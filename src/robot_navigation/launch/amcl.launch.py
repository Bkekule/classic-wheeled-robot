#!/usr/bin/env python3
"""
@brief Localization and navigation launch file.

Launches the full Nav2 stack for map-based localization and autonomous navigation:
  1. Map Server — serves the pre-built occupancy grid
  2. AMCL — adaptive Monte Carlo localization
  3. Nav2 Navigation — planner, controller, costmaps, behavior tree navigator

Usage:
    ros2 launch robot_navigation amcl.launch.py map_name:=my_world.yaml

    # With a custom Nav2 params file:
    ros2 launch robot_navigation amcl.launch.py \
        map_name:=my_world.yaml \
        params_file:=/path/to/custom_nav2_params.yaml

The map_name argument is just the filename (e.g. my_world.yaml); the file is
expected to live in the maps/ subfolder of this package.
"""

import os
import sys

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, OpaqueFunction
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node


def _validate_map_file(context, *_args, **_kwargs):
    """Exit early if the specified map YAML file does not exist."""
    pkg_dir = get_package_share_directory('robot_navigation')
    map_name = context.launch_configurations['map_name']
    map_path = os.path.join(pkg_dir, 'maps', map_name)

    if not os.path.isfile(map_path):
        sys.exit(
            f'\n[ERROR] Map file not found: {map_path}\n'
            f'       Run generate_map.launch.py first to create it.\n'
        )


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch map_server, AMCL, and the Nav2 navigation stack.

    @return LaunchDescription with full localization and navigation.
    """
    pkg_dir = get_package_share_directory('robot_navigation')
    nav2_bringup_dir = get_package_share_directory('nav2_bringup')

    default_params_file = os.path.join(pkg_dir, 'config', 'nav2_params.yaml')

    # ─── Launch arguments ─────────────────────────────────────────────────────

    declare_map_name = DeclareLaunchArgument(
        'map_name',
        description='Map YAML filename (expected in the maps/ subfolder, e.g. my_world.yaml)',
    )

    declare_use_sim_time = DeclareLaunchArgument(
        'use_sim_time',
        default_value='true',
        description='Use simulation clock',
    )

    declare_params_file = DeclareLaunchArgument(
        'params_file',
        default_value=default_params_file,
        description='Full path to the Nav2 parameters YAML file',
    )

    # ─── Validate map file exists ─────────────────────────────────────────────

    validate_map = OpaqueFunction(function=_validate_map_file)

    # ─── Map Server ───────────────────────────────────────────────────────────

    map_yaml_path = PathJoinSubstitution([pkg_dir, 'maps', LaunchConfiguration('map_name')])

    map_server_node = Node(
        package='nav2_map_server',
        executable='map_server',
        name='map_server',
        output='screen',
        parameters=[
            LaunchConfiguration('params_file'),
            {'yaml_filename': map_yaml_path},
        ],
    )

    # ─── AMCL ─────────────────────────────────────────────────────────────────

    amcl_node = Node(
        package='nav2_amcl',
        executable='amcl',
        name='amcl',
        output='screen',
        parameters=[LaunchConfiguration('params_file')],
    )

    # ─── Nav2 Navigation Stack ────────────────────────────────────────────────

    nav2_navigation = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(nav2_bringup_dir, 'launch', 'navigation_launch.py')
        ),
        launch_arguments=[
            ('use_sim_time', LaunchConfiguration('use_sim_time')),
            ('params_file', LaunchConfiguration('params_file')),
        ],
    )

    # ─── Lifecycle Manager (map_server + amcl) ────────────────────────────────

    lifecycle_manager_node = Node(
        package='nav2_lifecycle_manager',
        executable='lifecycle_manager',
        name='lifecycle_manager_localization',
        output='screen',
        parameters=[
            LaunchConfiguration('params_file'),
            {'node_names': ['map_server', 'amcl']},
        ],
    )

    return LaunchDescription(
        [
            declare_map_name,
            declare_use_sim_time,
            declare_params_file,
            validate_map,
            map_server_node,
            amcl_node,
            lifecycle_manager_node,
            nav2_navigation,
        ]
    )
