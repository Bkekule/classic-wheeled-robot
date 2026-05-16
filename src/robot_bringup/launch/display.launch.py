#!/usr/bin/env python3
"""
@brief Display launch for visualizing the robot in RViz with mock hardware.

Includes robot_core launch with mock_components/GenericSystem and starts RViz.
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch the robot with mock hardware and RViz (no simulator).

    @return LaunchDescription with robot_core and RViz node.
    """
    pkg_bringup_dir = get_package_share_directory('robot_bringup')

    declare_use_sim_time = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation clock',
    )

    core = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([pkg_bringup_dir, 'launch', 'robot_core.launch.py'])
        ),
        launch_arguments=[
            ('hardware_plugin', 'mock_components/GenericSystem'),
            ('use_sim_time', LaunchConfiguration('use_sim_time')),
        ],
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', PathJoinSubstitution([pkg_bringup_dir, 'launch', 'display.rviz'])],
        parameters=[{'use_sim_time': LaunchConfiguration('use_sim_time')}],
    )

    return LaunchDescription(
        [
            declare_use_sim_time,
            core,
            rviz_node,
        ]
    )
