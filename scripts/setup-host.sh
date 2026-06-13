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
ADDED_TO_DOCKER_GROUP=false
if groups "$USER" | grep -qw docker; then
  info "User '$USER' already in docker group, skipping."
else
  info "Adding '$USER' to docker group..."
  sudo usermod -aG docker "$USER"
  ADDED_TO_DOCKER_GROUP=true
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

# ── 6. vscode ──────────────────────────────────────────────────────────────
VSCODE_KEYRING=/usr/share/keyrings/microsoft-archive-keyring.gpg
if command -v code &>/dev/null; then
  info "VS Code already installed ($(code --version | head -1)), skipping."
else
  info "Adding Microsoft apt repository..."
  curl -fsSL https://packages.microsoft.com/keys/microsoft.asc \
    | sudo gpg --dearmor -o "$VSCODE_KEYRING"
  echo "deb [arch=amd64 signed-by=${VSCODE_KEYRING}] https://packages.microsoft.com/repos/code stable main" \
    | sudo tee /etc/apt/sources.list.d/vscode.list > /dev/null
  sudo apt-get update -y
  info "Installing VS Code..."
  sudo apt-get install -y code
fi

# ── 8. restart docker ──────────────────────────────────────────────────────
info "Restarting Docker daemon..."
sudo systemctl restart docker

# ── 9. smoke test + final instructions ────────────────────────────────────
echo ""
if [[ "$ADDED_TO_DOCKER_GROUP" == true ]]; then
  # Group membership isn't active yet in this shell, so apply it now and
  # run the smoke test inside the new group context rather than skipping it.
  info "Applying docker group membership and running smoke test..."
  if sg docker -c "docker run --rm --gpus all ubuntu nvidia-smi" &>/dev/null; then
    info "GPU smoke test passed."
  else
    warn "GPU smoke test failed. Verify that NVIDIA drivers are installed: run 'nvidia-smi' on the host."
  fi
else
  info "Smoke-testing GPU access inside Docker..."
  if docker run --rm --gpus all ubuntu nvidia-smi &>/dev/null; then
    info "GPU smoke test passed."
  else
    warn "GPU smoke test failed. Verify that NVIDIA drivers are installed: run 'nvidia-smi' on the host."
  fi
fi

# ── done ───────────────────────────────────────────────────────────────────
echo ""
echo "Setup complete. Run the following to start the dev container:"
echo ""
echo "  newgrp docker"
echo "  docker compose run --rm dev"
