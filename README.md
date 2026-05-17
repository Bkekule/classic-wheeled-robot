# classic_wheeled_robot

A modular ROS 2 robot description for a differential-drive mobile robot with Lidar and camera sensors,
driven through `ros2_control` so the same launch graph runs on mock hardware, in Gazebo Harmonic, or
on real hardware.

## Packages

- **`robot_description`**: URDF xacro files, ros2_control hardware interface, and shared properties
- **`robot_bringup`**: Launch files and controller configuration for RViz, Gazebo, and shared core bringup
- **`robot_control`**: Autonomous behaviour nodes (ball chaser, action clients/servers)
- **`robot_world`**: SDF world files and models for Gazebo simulation
- **`custom_interfaces`**: Custom ROS 2 message and service definitions

## Getting Started

Everything runs through [pixi](https://pixi.sh) + RoboStack. No system ROS installation is needed.

### 1. Install pixi

```bash
curl -fsSL https://pixi.sh/install.sh | bash
```

Then add the pixi binary to your PATH as directed by the installer (restart your shell or source your profile).

### 2. Clone and open in VSCode

```bash
git clone <repo-url>
cd classic-wheeled-robot
code .
```

When VSCode opens, accept the prompt to **Install Recommended Extensions** (defined in [.vscode/extensions.json](.vscode/extensions.json)).

### 3. Create the robostack environment folder

```bash
mkdir robostack
cp ci/pixi.toml robostack/pixi.toml
```

This folder is where pixi manages the ROS 2 Jazzy + Gazebo environment. The VSCode tasks all `cd robostack` before running pixi commands, so this layout is required.

### 4. Run tasks from the VSCode Command Palette

Open the Command Palette (`Cmd+Shift+P` / `Ctrl+Shift+P`), choose **Tasks: Run Task**, and select from:

| Task | What it does |
|---|---|
| **Build and Launch RViz** | Builds the workspace and launches the robot in RViz with mock hardware |
| **Build and Launch Gazebo** | Builds the workspace and launches the robot in Gazebo Harmonic |
| **Run Tests** | Builds and runs the full integration test suite |
| **Run Pre-commit** | Runs all pre-commit hooks across the repo |

Pixi installs all dependencies automatically on first run — this will take a few minutes the first time.

### If you plan to contribute

Run the **Run Pre-commit** task at least once before making changes. This installs the pre-commit hooks so formatting, XML/YAML validation, and linting run automatically on each commit.

## Robot Specification

- **Footprint**: `robot_footprint` link at ground level for navigation
- **Chassis**: 0.4 × 0.2 × 0.1 m box, 15 kg, frame at wheel-axle height
- **Driven wheels**: cylinder (r = 0.1 m, length 0.05 m), 2 kg each, continuous joints, velocity-controlled
- **Casters**: passive spheres (r = 0.05 m), fixed joints at the front and back of the chassis
- **Sensors**: Lidar on top, camera on front (with `camera_optical_frame`)

Dimensional values are centralised in [robot_properties.xacro](robot_description/urdf/common/robot_properties.xacro).

## CI

CI ([.github/workflows/ci.yml](.github/workflows/ci.yml)) runs pre-commit, the integration test suite, and a full `colcon build` against the same RoboStack environment defined in [ci/pixi.toml](ci/pixi.toml).
