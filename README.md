# classic_wheeled_robot

Developed on MacOS Tahoe, tested on Ubuntu 26.04. A modular ROS 2 robot description for a differential-drive mobile robot with Lidar and camera sensors.

## Packages

- **`robot_description`**: URDF xacro files, ros2_control hardware interface, and shared properties
- **`robot_bringup`**: Launch files and controller configuration for RViz, Gazebo, and shared core bringup
- **`robot_control`**: Autonomous behaviour nodes (ball chaser, action clients/servers)
- **`robot_world`**: SDF world files and models for Gazebo simulation
- **`custom_interfaces`**: Custom ROS 2 message and service definitions

## Getting Started

Everything runs inside Docker — no system ROS installation needed.

### 1. Install Docker

```bash
sudo apt-get update && sudo apt-get install -y docker.io docker-compose
sudo usermod -aG docker $USER
newgrp docker
```

### 2. Clone the repo

```bash
git clone <repo-url>
cd classic-wheeled-robot
```

### 3. Run

```bash
docker-compose up robot                        # mock hardware + RViz
docker-compose --profile gazebo up gazebo      # Gazebo sim
docker-compose --profile test run --rm test    # tests
```

To open a shell inside a running container:

```bash
docker exec -it classic-wheeled-robot_gazebo_1 /docker-entrypoint.sh bash
```

The container name follows the pattern `{project}_{service}_{replica}` — replace `gazebo` with the service name and `1` with the replica number shown in `docker ps`.

## Contributing

Run `pre-commit` at least once before making changes to install the git hooks, so formatting, XML/YAML validation, and linting run automatically on each commit.

## Robot Specification

- **Footprint**: `robot_footprint` link at ground level for navigation
- **Chassis**: 0.4 × 0.2 × 0.1 m box, 15 kg, frame at wheel-axle height
- **Driven wheels**: cylinder (r = 0.1 m, length 0.05 m), 2 kg each, continuous joints, velocity-controlled
- **Casters**: passive spheres (r = 0.05 m), fixed joints at the front and back of the chassis
- **Sensors**: Lidar on top, camera on front (with `camera_optical_frame`)

Dimensional values are centralised in [robot_properties.xacro](robot_description/urdf/common/robot_properties.xacro).

## CI

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) runs pre-commit, the integration test suite, and a full `colcon build` against the same RoboStack environment defined in [ci/pixi.toml](ci/pixi.toml).
