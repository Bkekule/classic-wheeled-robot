#!/usr/bin/env python3

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import (
  Command,
  EnvironmentVariable,
  LaunchConfiguration,
  PathJoinSubstitution
)
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


_GZ_NOISE_RE = (
    r'descriptor(_database)?\.cc'
    r'|DynamicFactory\(\)'
    r'|absl::InitializeLog'
)
_GZ_QUIET_PREFIX = f"zsh -c 'exec \"$@\" 2> >(grep -vE \"{_GZ_NOISE_RE}\" >&2)' --"


def generate_launch_description() -> LaunchDescription:
    """
    Launch Gazebo Harmonic with the robot.

    Controller manager is provided by GazeboSimROS2ControlPlugin loaded inside
    Gazebo — no standalone ros2_control_node is started here.
    """
    pkg_description_dir = get_package_share_directory('robot_description')

    declare_world = DeclareLaunchArgument(
        'world',
        default_value='empty',
        description='World SDF filename (without .sdf) inside $GZ_SIM_WORLD_PATH/',
    )

    world_sdf_path = PathJoinSubstitution(
        [EnvironmentVariable('GZ_SIM_WORLD_PATH'), [LaunchConfiguration('world'), '.sdf']]
    )

    robot_description_content = ParameterValue(
        Command([
            'xacro ',
            PathJoinSubstitution([pkg_description_dir, 'urdf', 'robot.urdf.xacro']),
            ' hardware_plugin:=gz_ros2_control/GazeboSimSystem',
        ]),
        value_type=str,
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{
            'robot_description': robot_description_content,
            'use_sim_time': True,
        }],
    )

    gzserver_node = ExecuteProcess(
        cmd=['zsh', '-c',
             ['exec gz sim -s -v4 ', world_sdf_path, f" 2> >(grep -vE '{_GZ_NOISE_RE}' >&2)"]],
        additional_env={
            'GZ_SIM_SYSTEM_PLUGIN_PATH': '',
            'GZ_SIM_RESOURCE_PATH': '',
        },
        output='screen',
    )

    gzgui_node = ExecuteProcess(
        cmd=['zsh', '-c', 'exec gz sim -g -v4 '],
        output='screen',
    )

    spawn_robot_service = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=['-topic', 'robot_description', '-name', 'robot'],
        prefix=_GZ_QUIET_PREFIX,
        output='screen',
    )

    # ROS-Gazebo bridge for sensors only.
    # /cmd_vel and /odom are handled by diff_drive_controller in ROS space.
    ros_gz_bridge_node = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            '/camera/image_raw@sensor_msgs/msg/Image[gz.msgs.Image',
        ],
        prefix=_GZ_QUIET_PREFIX,
        output='screen',
    )

    return LaunchDescription(
        [
            declare_world,
            robot_state_publisher_node,
            gzserver_node,
            gzgui_node,
            spawn_robot_service,
            ros_gz_bridge_node,
        ]
    )
