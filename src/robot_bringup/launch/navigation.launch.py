#!/usr/bin/env python3
"""
@brief Full simulation + navigation launch.

Brings up the Gazebo simulation with the robot and layers Nav2 localization
and navigation on top. This is the all-in-one launch for autonomous navigation
in simulation.

Usage:
    ros2 launch robot_bringup navigation.launch.py map_name:=my_world.yaml

    # With a different world:
    ros2 launch robot_bringup navigation.launch.py \
        world:=office.sdf map_name:=office.yaml
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch Gazebo simulation with Nav2 localization and navigation.

    @return LaunchDescription combining gazebo.launch.py and amcl.launch.py.
    """
    pkg_bringup_dir = get_package_share_directory('robot_bringup')
    pkg_navigation_dir = get_package_share_directory('robot_navigation')

    # ─── Launch arguments ─────────────────────────────────────────────────────

    declare_world = DeclareLaunchArgument(
        'world',
        default_value='my_apartment.sdf',
        description='World SDF filename, resolved via GZ_SIM_RESOURCE_PATH',
    )

    declare_map_name = DeclareLaunchArgument(
        'map_name',
        description='Map YAML filename (expected in robot_navigation/maps/, e.g. my_world.yaml)',
    )

    declare_params_file = DeclareLaunchArgument(
        'params_file',
        default_value=PathJoinSubstitution([pkg_navigation_dir, 'config', 'nav2_params.yaml']),
        description='Full path to the Nav2 parameters YAML file',
    )

    # ─── Gazebo simulation ────────────────────────────────────────────────────

    gazebo = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([pkg_bringup_dir, 'launch', 'gazebo.launch.py'])
        ),
        launch_arguments=[
            ('world', LaunchConfiguration('world')),
            ('launch_ball_chaser', 'false'),
        ],
    )

    # ─── Nav2 localization + navigation ───────────────────────────────────────

    navigation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([pkg_navigation_dir, 'launch', 'amcl.launch.py'])
        ),
        launch_arguments=[
            ('map_name', LaunchConfiguration('map_name')),
            ('use_sim_time', 'true'),
            ('params_file', LaunchConfiguration('params_file')),
        ],
    )

    return LaunchDescription(
        [
            declare_world,
            declare_map_name,
            declare_params_file,
            gazebo,
            navigation,
        ]
    )
