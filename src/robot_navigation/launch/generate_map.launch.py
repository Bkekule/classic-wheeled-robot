#!/usr/bin/env python3
"""
@brief Launch file to generate a 2-D occupancy-grid PGM map from a Gazebo world.

This launch file assumes the `pgm_map_creator` package is installed and available
in the current ROS 2 workspace. It delegates to:

    pgm_map_creator / generate_map.launch.py

and writes the resulting .pgm file plus a companion .yaml metadata file into the
`maps/` folder at the installed package share path. The maps/ directory is created
at runtime if it does not already exist.

Usage example:
    ros2 launch robot_navigation generate_map.launch.py \
        world_name:=my_apartment.sdf \
        xmin:=-15 xmax:=15 ymin:=-15 ymax:=15 \
        scan_height:=5 resolution:=0.01

Optional threshold overrides:
    ros2 launch robot_navigation generate_map.launch.py \
        world_name:=my_apartment.sdf \
        occupied_thresh:=0.65 free_thresh:=0.196 negate:=0
"""

import os
from pathlib import Path

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import (
    DeclareLaunchArgument,
    IncludeLaunchDescription,
    OpaqueFunction,
)
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution


def _generate_map_and_yaml(context, *_args, **_kwargs):
    """
    OpaqueFunction that:
    1. Ensures the maps/ directory exists at runtime.
    2. Returns the IncludeLaunchDescription for pgm_map_creator with the correct
       output_path (full file path prefix, not just the directory).
    3. Writes the companion .yaml metadata file.
    """
    world_name = context.launch_configurations['world_name']
    resolution = context.launch_configurations['resolution']
    xmin = context.launch_configurations['xmin']
    xmax = context.launch_configurations['xmax']
    ymin = context.launch_configurations['ymin']
    ymax = context.launch_configurations['ymax']
    scan_height = context.launch_configurations['scan_height']
    occupied_thresh = context.launch_configurations['occupied_thresh']
    free_thresh = context.launch_configurations['free_thresh']
    negate = context.launch_configurations['negate']

    map_basename = Path(world_name).stem  # e.g. "my_apartment" from "my_apartment.sdf"

    # Resolve maps/ directory inside the installed package share
    pkg_dir = get_package_share_directory('robot_navigation')
    maps_dir = os.path.join(pkg_dir, 'maps')
    os.makedirs(maps_dir, exist_ok=True)

    # pgm_map_creator treats output_path as a file path prefix:
    #   output_path=/path/to/maps/my_apartment → writes /path/to/maps/my_apartment.pgm
    output_path = os.path.join(maps_dir, map_basename)

    pgm_map_creator_dir = get_package_share_directory('pgm_map_creator')

    # ─── Include pgm_map_creator ──────────────────────────────────────────────

    generate_map = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(pgm_map_creator_dir, 'launch', 'generate_map.launch.py')
        ),
        launch_arguments=[
            ('world_name', world_name),
            ('output_path', output_path),
            ('xmin', xmin),
            ('xmax', xmax),
            ('ymin', ymin),
            ('ymax', ymax),
            ('scan_height', scan_height),
            ('resolution', resolution),
        ],
    )

    # ─── Write companion YAML metadata ────────────────────────────────────────

    yaml_path = os.path.join(maps_dir, f'{map_basename}.yaml')
    yaml_content = (
        f'image: {map_basename}.pgm\n'
        f'resolution: {resolution}\n'
        f'origin: [{xmin}, {ymin}, 0.0]\n'
        f'occupied_thresh: {occupied_thresh}\n'
        f'free_thresh: {free_thresh}\n'
        f'negate: {negate}\n'
    )

    with open(yaml_path, 'w') as f:
        f.write(yaml_content)

    return [generate_map]


def generate_launch_description() -> LaunchDescription:
    """
    @brief Generate a PGM map from a Gazebo .sdf world file.

    @return LaunchDescription that invokes pgm_map_creator and writes a YAML sidecar.
    """
    # ─── Launch arguments ─────────────────────────────────────────────────────

    declare_world_name = DeclareLaunchArgument(
        'world_name',
        description='Filename of the Gazebo .sdf world (e.g. my_apartment.sdf)',
    )

    declare_xmin = DeclareLaunchArgument(
        'xmin', default_value='-15', description='Minimum X bound of the map area'
    )

    declare_xmax = DeclareLaunchArgument(
        'xmax', default_value='15', description='Maximum X bound of the map area'
    )

    declare_ymin = DeclareLaunchArgument(
        'ymin', default_value='-15', description='Minimum Y bound of the map area'
    )

    declare_ymax = DeclareLaunchArgument(
        'ymax', default_value='15', description='Maximum Y bound of the map area'
    )

    declare_scan_height = DeclareLaunchArgument(
        'scan_height', default_value='5', description='Height at which to scan the world'
    )

    declare_resolution = DeclareLaunchArgument(
        'resolution', default_value='0.01', description='Map resolution in meters/pixel'
    )

    declare_occupied_thresh = DeclareLaunchArgument(
        'occupied_thresh',
        default_value='0.65',
        description='Threshold above which a cell is considered occupied',
    )

    declare_free_thresh = DeclareLaunchArgument(
        'free_thresh',
        default_value='0.196',
        description='Threshold below which a cell is considered free',
    )

    declare_negate = DeclareLaunchArgument(
        'negate',
        default_value='0',
        description='Whether to negate the image colors (0 or 1)',
    )

    # ─── Generate map + write YAML (resolved at runtime) ─────────────────────

    generate_and_write = OpaqueFunction(function=_generate_map_and_yaml)

    return LaunchDescription(
        [
            declare_world_name,
            declare_xmin,
            declare_xmax,
            declare_ymin,
            declare_ymax,
            declare_scan_height,
            declare_resolution,
            declare_occupied_thresh,
            declare_free_thresh,
            declare_negate,
            generate_and_write,
        ]
    )
