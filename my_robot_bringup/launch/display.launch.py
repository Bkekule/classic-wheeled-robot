#!/usr/bin/env python3

import os

import xacro
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
    """Launch RViz visualization of the robot."""

    pkg_description_dir = get_package_share_directory("my_robot_description")
    pkg_bringup_dir = get_package_share_directory("my_robot_bringup")

    urdf_file = os.path.join(pkg_description_dir, "urdf", "my_robot.urdf.xacro")

    # Declare arguments
    declare_use_sim_time = DeclareLaunchArgument(
        "use_sim_time",
        default_value="false",
        description="Use simulation clock"
    )

    # Expand xacro to URDF in-process so any errors surface here
    robot_description_content = xacro.process_file(urdf_file).toxml()

    # Robot state publisher
    robot_state_publisher_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[
            {
                "robot_description": robot_description_content,
                "use_sim_time": LaunchConfiguration("use_sim_time"),
            }
        ],
    )

    # Publishes default (zero) joint states so RSP can compute wheel transforms
    joint_state_publisher_node = Node(
        package='joint_state_publisher',
        executable='joint_state_publisher',
        parameters=[{'use_sim_time': LaunchConfiguration('use_sim_time')}],
    )

    # RViz with default config
    rviz_config_file = PathJoinSubstitution(
        [pkg_bringup_dir, "launch", "display.rviz"]
    )

    rviz_node = Node(
        package="rviz2",
        executable="rviz2",
        name="rviz2",
        arguments=["-d", rviz_config_file],
        parameters=[
            {"use_sim_time": LaunchConfiguration("use_sim_time")}
        ],
    )

    return LaunchDescription(
        [
            declare_use_sim_time,
            robot_state_publisher_node,
            joint_state_publisher_node,
            rviz_node,
        ]
    )
