#!/usr/bin/env bash
# Run once on a fresh Ubuntu host (e.g. ubuntu-carla) to install all
# prerequisites, then: docker compose run --rm dev
set -euo pipefail

# ── helpers ────────────────────────────────────────────────────────────────
info()  { echo "[INFO]  $*"; }
warn()  { echo "[WARN]  $*"; }
die()   { echo "[ERROR] $*" >&2; exit 1; }

require_ubuntu() {
  [[ "$(uname -s)" == "Linux" ]] || die "This script is for Linux only."
  command -v apt-get &>/dev/null || die "apt-get not found — not an Ubuntu/Debian host?"
}

# ── 1. base apt hygiene ────────────────────────────────────────────────────
# The Open Robotics signing key (F42ED6FBAB17C654) periodically expires on
# Focal installs. Replace the legacy apt-key entry with a modern signed-by
# keyring so apt-get update succeeds.
ROS_KEYRING=/usr/share/keyrings/ros-archive-keyring.gpg
ROS_LIST=/etc/apt/sources.list.d/ros-latest.list
if [[ -f "$ROS_LIST" ]]; then
  info "Refreshing Open Robotics apt key..."
  sudo apt-key del F42ED6FBAB17C654 2>/dev/null || true
  curl -fsSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.asc \
    | sudo gpg --dearmor -o "$ROS_KEYRING"
  # Rewrite the source list to use the signed-by keyring
  sudo sed -i \
    "s|deb http|deb [signed-by=${ROS_KEYRING}] http|g" \
    "$ROS_LIST"
  info "Open Robotics key refreshed."
fi

info "Updating apt..."
sudo apt-get update -y
sudo apt-get autoremove -y

# ── 2. docker ─────────────────────────────────────────────────────────────
if command -v docker &>/dev/null; then
  info "Docker already installed ($(docker --version)), skipping."
else
  info "Installing docker.io..."
  sudo apt-get install -y --fix-missing docker.io
fi

# docker compose v2 (the 'docker compose' plugin, not legacy docker-compose)
if docker compose version &>/dev/null 2>&1; then
  info "Docker Compose v2 already installed, skipping."
else
  info "Installing docker-compose-v2..."
  sudo apt-get install -y --fix-missing docker-compose-v2
fi

# ── 3. current user → docker group ────────────────────────────────────────
if groups "$USER" | grep -qw docker; then
  info "User '$USER' already in docker group, skipping."
else
  info "Adding '$USER' to docker group..."
  sudo usermod -aG docker "$USER"
  warn "Group membership takes effect in a new shell session."
  warn "After this script finishes, run: newgrp docker  (or log out/in)."
fi

# ── 4. nvidia-container-toolkit ────────────────────────────────────────────
if dpkg -l nvidia-container-toolkit &>/dev/null 2>&1; then
  info "nvidia-container-toolkit already installed, skipping."
else
  info "Adding NVIDIA container toolkit repository..."
  curl -fsSL https://nvidia.github.io/libnvidia-container/gpgkey \
    | sudo gpg --dearmor -o /usr/share/keyrings/nvidia-container-toolkit-keyring.gpg

  curl -s -L https://nvidia.github.io/libnvidia-container/stable/deb/nvidia-container-toolkit.list \
    | sed 's#deb https://#deb [signed-by=/usr/share/keyrings/nvidia-container-toolkit-keyring.gpg] https://#g' \
    | sudo tee /etc/apt/sources.list.d/nvidia-container-toolkit.list > /dev/null

  sudo apt-get update -y
  info "Installing nvidia-container-toolkit..."
  sudo apt-get install -y nvidia-container-toolkit
fi

# ── 5. configure docker nvidia runtime ─────────────────────────────────────
DOCKER_DAEMON=/etc/docker/daemon.json
if sudo grep -q '"nvidia"' "$DOCKER_DAEMON" 2>/dev/null; then
  info "NVIDIA docker runtime already configured, skipping."
else
  info "Configuring NVIDIA runtime for Docker..."
  sudo nvidia-ctk runtime configure --runtime=docker
fi

# ── 6. restart docker ──────────────────────────────────────────────────────
info "Restarting Docker daemon..."
sudo systemctl restart docker

# ── 7. smoke test ──────────────────────────────────────────────────────────
info "Smoke-testing GPU access inside Docker..."
if docker run --rm --gpus all ubuntu nvidia-smi &>/dev/null; then
  info "GPU smoke test passed."
else
  warn "GPU smoke test failed. Check that NVIDIA drivers are installed on the host (nvidia-smi should work outside Docker)."
fi

# ── done ───────────────────────────────────────────────────────────────────
echo ""
echo "Setup complete."
echo ""
echo "If you were just added to the docker group, run:"
echo "  newgrp docker"
echo ""
echo "Then start the dev container:"
echo "  docker compose run --rm dev"
