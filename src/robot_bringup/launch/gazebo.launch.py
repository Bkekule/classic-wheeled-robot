#!/usr/bin/env python3
"""
@brief Gazebo Harmonic simulation launch using native Gazebo plugins.

Starts the Gazebo server and GUI, spawns the robot, bridges ROS-Gz topics,
and launches the ball chasing nodes and RViz.
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import (
    Command,
    LaunchConfiguration,
    PathJoinSubstitution,
)
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch Gazebo Harmonic with the robot using native Gazebo plugins.

    Drives the robot via gz-sim-diff-drive and publishes joint states via
    gz-sim-joint-state-publisher, avoiding gz_ros2_control entirely.

    @return LaunchDescription with Gazebo server, GUI, robot nodes, ROS-Gz bridge, and RViz.
    """
    pkg_description_dir = get_package_share_directory('robot_description')

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

    robot_description_content = ParameterValue(
        Command(
            [
                'xacro ',
                PathJoinSubstitution([pkg_description_dir, 'urdf', 'robot.gazebo.urdf.xacro']),
            ]
        ),
        value_type=str,
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[
            {
                'robot_description': robot_description_content,
                'use_sim_time': True,
            }
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
            '/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
            '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
            '/joint_states@sensor_msgs/msg/JointState[gz.msgs.Model',
            '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            '/camera/rgb/image_raw@sensor_msgs/msg/Image[gz.msgs.Image',
            '/camera/rgb/camera_info@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo',
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
        ],
        output='screen',
    )

    drive_bot_node = Node(
        package='robot_control',
        executable='drive_bot_main',
        output='screen',
    )

    process_image_node = Node(
        package='robot_control',
        executable='process_image_main',
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
            robot_state_publisher_node,
            gz_spawn_entity,
            ros_gz_bridge_node,
            drive_bot_node,
            process_image_node,
            rviz_node,
        ]
    )
