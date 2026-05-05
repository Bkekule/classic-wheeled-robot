#!/usr/bin/env python3

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import EnvironmentVariable, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node


_GZ_NOISE_RE = (
    r'descriptor(_database)?\.cc'
    r'|DynamicFactory\(\)'
    r'|absl::InitializeLog'
)
_GZ_QUIET_PREFIX = f"zsh -c 'exec \"$@\" 2> >(grep -vE \"{_GZ_NOISE_RE}\" >&2)' --"


def generate_launch_description() -> LaunchDescription:
    """Launch Gazebo Harmonic with the robot."""
    pkg_bringup_dir = get_package_share_directory('my_robot_bringup')

    declare_world = DeclareLaunchArgument(
        'world',
        default_value='empty',
        description='World SDF filename (without .sdf) inside $GZ_SIM_WORLD_PATH/',
    )

    world_sdf_path = PathJoinSubstitution(
        [EnvironmentVariable('GZ_SIM_WORLD_PATH'), [LaunchConfiguration('world'), '.sdf']]
    )

    core = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([pkg_bringup_dir, 'launch', 'robot_core.launch.py'])
        ),
        launch_arguments=[
            ('hardware_plugin', 'gz_ros2_control/GazeboSimSystem'),
            ('use_sim_time', 'true'),
        ],
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

    spawn_robot_service = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=['-topic', 'robot_description', '-name', 'my_robot'],
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
            core,
            gzserver_node,
            spawn_robot_service,
            ros_gz_bridge_node,
        ]
    )
