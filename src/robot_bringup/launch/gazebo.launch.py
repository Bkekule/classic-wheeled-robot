#!/usr/bin/env python3

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import (
    Command,
    EnvironmentVariable,
    LaunchConfiguration,
    PathJoinSubstitution,
)
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description() -> LaunchDescription:
    """
    Launch Gazebo Harmonic with the robot using native Gazebo plugins.

    Drives the robot via gz-sim-diff-drive and publishes joint states via
    gz-sim-joint-state-publisher, avoiding gz_ros2_control entirely.
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

    gazebo_server = ExecuteProcess(
        cmd=['gz', 'sim', '-s', '-r', '-v', '3', world_sdf_path],
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
            '/camera/image_raw@sensor_msgs/msg/Image[gz.msgs.Image',
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
        ],
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
            rviz_node,
        ]
    )
