# robot_description

A modular ROS 2 robot description for a differential-drive mobile robot with Lidar and camera sensors,
driven through `ros2_control` so the same launch graph runs on mock hardware, in Gazebo Harmonic, or
on real hardware.

## Packages

- **`robot_description`**: URDF xacro files, ros2_control hardware interface, and shared properties
- **`robot_bringup`**: Launch files and controller configuration for RViz, Gazebo, and shared core bringup

## Requirements

- ROS 2 Jazzy
- Gazebo Harmonic
- `ros2_control`, `ros2_controllers`, `gz_ros2_control`
- `ros_gz_sim`, `ros_gz_bridge`
- `xacro`, `robot_state_publisher`, `rviz2`

A reproducible dev environment is provided via [pixi](https://pixi.sh) + RoboStack — see
[ci/pixi.toml](ci/pixi.toml). CI uses the same environment.

## Build

```bash
cd ~/workspace
colcon build --packages-select robot_description robot_bringup
source install/setup.bash
```

## Run

### Visualize in RViz (mock hardware)

Spins up `ros2_control` with `mock_components/GenericSystem` so `/cmd_vel` drives the robot in RViz
without a simulator.

```bash
ros2 launch robot_bringup display.launch.py
```

### Simulate in Gazebo Harmonic

Spawns the robot in a Gazebo world, wires `gz_ros2_control` into the same controller graph, and
bridges sensor topics (`/scan`, `/camera/image_raw`) into ROS. `/cmd_vel` and `/odom` are handled in
ROS by `diff_drive_controller`.

```bash
ros2 launch robot_bringup gazebo_server.launch.py
# optional: world:=<name> selects $GZ_SIM_WORLD_PATH/<name>.sdf (default: empty)
```

Both launches include [robot_core.launch.py](robot_bringup/launch/robot_core.launch.py), which
brings up `robot_state_publisher`, `controller_manager`, the `joint_state_broadcaster`, and the
`diff_drive_controller`.

## Layout

```
robot_description/
├── urdf/
│   ├── my_robot.urdf.xacro             (top-level, includes everything)
│   ├── common/
│   │   └── robot_properties.xacro      (shared dimensions and masses)
│   ├── base/
│   │   ├── base.xacro                  (footprint, chassis, caster instances)
│   │   ├── base.gazebo.xacro           (materials)
│   │   └── base.ros2_control.xacro     (hardware interface, swappable plugin)
│   ├── wheels/
│   │   ├── wheel.xacro                 (driven-wheel macro, used L/R)
│   │   ├── wheel.gazebo.xacro          (friction params)
│   │   └── caster.xacro                (passive sphere macro, front/back)
│   └── sensors/
│       ├── lidar.xacro                 (link + joint)
│       ├── lidar.gazebo.xacro          (gpu_lidar plugin)
│       ├── camera.xacro                (link + optical frame)
│       └── camera.gazebo.xacro         (camera sensor plugin)
└── config/

robot_bringup/
├── launch/
│   ├── robot_core.launch.py            (shared: rsp + controller_manager + spawners)
│   ├── display.launch.py               (mock hardware + RViz)
│   ├── gazebo_server.launch.py         (gz sim + spawn + sensor bridge)
│   └── display.rviz
└── config/
    └── controllers.yaml                (diff_drive + joint_state_broadcaster)
```

The `hardware_plugin` xacro arg in [base.ros2_control.xacro](robot_description/urdf/base/base.ros2_control.xacro)
is what lets the same URDF target mock hardware, Gazebo (`gz_ros2_control/GazeboSimSystem`), or a
real-hardware plugin without forking the description.

## Robot Specification

- **Footprint**: `robot_footprint` link at ground level for navigation
- **Chassis**: 0.4 × 0.2 × 0.1 m box, 15 kg, frame at wheel-axle height
- **Driven wheels**: cylinder (r = 0.1 m, length 0.05 m), 2 kg each, continuous joints, velocity-controlled
- **Casters**: passive spheres (r = 0.05 m), fixed joints at the front and back of the chassis
- **Sensors**: Lidar on top, camera on front (with `camera_optical_frame`)

Dimensional values are centralised in [robot_properties.xacro](robot_description/urdf/common/robot_properties.xacro).

## Testing

Integration tests live in each package's `tests/integration/` folder:

- `robot_description` — xacro expansion, required links/joints, joint types
- `robot_bringup` — `controllers.yaml` geometry stays in sync with the URDF (wheel radius, separation)

```bash
colcon test --packages-select robot_description robot_bringup
colcon test-result --verbose
```

`pre-commit` runs formatting, XML/YAML validation, `actionlint`, and `zizmor` on every change; CI
([.github/workflows/ci.yml](.github/workflows/ci.yml)) runs pre-commit, the test suite, and a full
`colcon build` against the RoboStack environment.
