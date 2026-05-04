# my_robot_description

A modular ROS2 robot description package for a differential-drive mobile robot with Lidar and camera sensors.

## Packages

- **`my_robot_description`**: URDF xacro files, meshes, and configuration
- **`my_robot_bringup`**: Launch files for RViz visualization and Gazebo Harmonic simulation

## Requirements

- ROS2 Jazzy
- Gazebo Harmonic
- `xacro`, `robot_state_publisher`, `joint_state_publisher`, `rviz2`

## Build

```bash
cd ~/workspace
colcon build --packages-select my_robot_description my_robot_bringup
```

## Run

### Visualize in RViz
```bash
ros2 launch my_robot_bringup display.launch.py
```

### Simulate in Gazebo Harmonic
```bash
ros2 launch my_robot_bringup gazebo.launch.py
```

## URDF Structure

```
urdf/
├── my_robot.urdf.xacro          (top-level, includes all sub-files)
├── base/
│   ├── base.xacro               (chassis + caster wheels)
│   └── base.gazebo.xacro        (DiffDrive plugin, materials)
├── wheels/
│   ├── wheel.xacro              (reusable macro for driven wheels)
│   └── wheel.gazebo.xacro       (friction params)
└── sensors/
    ├── lidar.xacro              (lidar + gpu_lidar plugin)
    └── camera.xacro             (camera + camera sensor plugin)
```

## Robot Specification

- **Base**: 0.4 × 0.2 × 0.1 m box, 15 kg
- **Wheels**: Cylinder (radius 0.1 m, length 0.05 m), mass 2 kg each, continuous joints
- **Casters**: Passive support spheres (radius 0.05 m), fixed joints at front and back
- **Sensors**: Lidar on top, camera on front

## Testing

```bash
colcon test --packages-select my_robot_description
```
