#!/usr/bin/env python3
"""
@brief Gazebo Harmonic simulation launch using gz_ros2_control.

Starts the Gazebo server and GUI, includes robot_core for controller_manager
and robot_state_publisher, spawns the robot, bridges sensor topics via
ros_gz_bridge, and launches the ball chaser node and RViz.
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch Gazebo Harmonic with the robot using gz_ros2_control.

    Uses robot_core.launch.py to start robot_state_publisher, controller_manager,
    and controller spawners with the GazeboSimSystem hardware plugin. The
    ros_gz_bridge only bridges sensor topics (/clock, /scan, /camera/*) since
    /cmd_vel, /odom, and /joint_states are handled natively by ros2_control.

    @return LaunchDescription with Gazebo server, GUI, robot_core, spawn,
    ros_gz_bridge, ball_chaser, and RViz.
    """
    pkg_bringup_dir = get_package_share_directory('robot_bringup')

    declare_world = DeclareLaunchArgument(
        'world',
        default_value='my_apartment.sdf',
        description='World SDF filename, resolved via GZ_SIM_RESOURCE_PATH',
    )

    gazebo_server = ExecuteProcess(
        cmd=['gz', 'sim', '-s', '-r', '-v', '3', LaunchConfiguration('world')],
        output='screen',
    )

    gazebo_gui = ExecuteProcess(
        cmd=['gz', 'sim', '-g', '-v', '3'],
        output='screen',
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

    gz_spawn_entity = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=['-topic', 'robot_description', '-name', 'robot'],
        output='screen',
    )

    ros_gz_bridge_node = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
            '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            '/camera/rgb/image_raw@sensor_msgs/msg/Image[gz.msgs.Image',
            '/camera/rgb/camera_info@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo',
        ],
        output='screen',
    )

    ball_chaser_node = Node(
        package='robot_control',
        executable='ball_chaser_main',
        output='screen',
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        arguments=[
            '-d',
            PathJoinSubstitution(
                [
                    FindPackageShare('robot_bringup'),
                    'launch',
                    'display.rviz',
                ]
            ),
        ],
        parameters=[{'use_sim_time': True}],
    )

    return LaunchDescription(
        [
            declare_world,
            gazebo_server,
            gazebo_gui,
            core,
            gz_spawn_entity,
            ros_gz_bridge_node,
            ball_chaser_node,
            rviz_node,
        ]
    )
