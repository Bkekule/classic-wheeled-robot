# classic_wheeled_robot

Developed on MacOS Tahoe, tested on Ubuntu 26.04. A modular ROS 2 robot description for a differential-drive mobile robot with Lidar and camera sensors.

## Packages

- **`robot_description`**: URDF xacro files, ros2_control hardware interface, and shared properties
- **`robot_bringup`**: Launch files and controller configuration for RViz, Gazebo, and shared core bringup
- **`robot_control`**: Autonomous behaviour nodes (ball chaser, action clients/servers)
- **`robot_world`**: SDF world files and models for Gazebo simulation
- **`custom_interfaces`**: Custom ROS 2 message and service definitions

## Getting Started

Everything runs through [pixi](https://pixi.sh) for a controlled, self-sufficient environment — no system ROS installation needed.

### 1. Install pixi

```bash
curl -fsSL https://pixi.sh/install.sh | bash
```

Then export the pixi binary to your PATH:

```bash
export PATH="$HOME/.pixi/bin:$PATH"
```

### 2. Clone the repo

```bash
git clone <repo-url>
cd classic-wheeled-robot
```

### 3. Create the robostack environment folder

```bash
mkdir robostack
cp ci/pixi.toml robostack/pixi.toml
```

This is where pixi manages the ROS 2 Jazzy + Gazebo environment.

### 4. Run tasks

Choose your preferred workflow:

**Command line**

```bash
cd robostack
pixi run -e jazzy <task-name>
```

Available tasks:

| Task | What it does |
|---|---|
| `gz-launch` | Builds and launches the robot in Gazebo Harmonic |
| `rviz-launch` | Builds and launches the robot in RViz with mock hardware and eventual  true hardware |
| `run-tests` | Builds and runs the full integration test suite |
| `pre-commit` | Runs all pre-commit hooks across the repo |

Pixi installs all dependencies automatically on first run — this will take a few minutes the first time. Details of what each of these tasks are running can be seen [here](ci/pixi.toml#L38)

**VSCode**

Open the Command Palette (`Ctrl+Shift+P`), choose **Tasks: Run Task**, and select the task you want to run.

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
