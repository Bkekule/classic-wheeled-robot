#!/usr/bin/env python3
"""
@brief Launch file to generate a 2-D occupancy-grid PGM map from a Gazebo world.

This launch file assumes the `pgm_map_creator` package is installed and available
in the current ROS 2 workspace. It delegates to:

    pgm_map_creator / generate_map.launch.py

and writes the resulting .pgm file plus a companion .yaml metadata file into the
`maps/` folder that lives alongside this `launch/` folder.

Usage example:
    ros2 launch robot_localization generate_map.launch.py \
        world_name:=my_world.sdf \
        xmin:=-15 xmax:=15 ymin:=-15 ymax:=15 \
        scan_height:=5 resolution:=0.01

Optional threshold overrides:
    ros2 launch robot_localization generate_map.launch.py \
        world_name:=my_world.sdf \
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


def _write_yaml(context, *_args, **_kwargs):
    """OpaqueFunction callback that writes the map YAML metadata file after generation."""
    world_name = context.launch_configurations['world_name']
    resolution = context.launch_configurations['resolution']
    xmin = context.launch_configurations['xmin']
    ymin = context.launch_configurations['ymin']
    occupied_thresh = context.launch_configurations['occupied_thresh']
    free_thresh = context.launch_configurations['free_thresh']
    negate = context.launch_configurations['negate']

    map_basename = Path(world_name).stem  # e.g. "my_world" from "my_world.sdf"

    # maps/ lives next to launch/ inside the source tree
    maps_dir = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'maps'
    )
    os.makedirs(maps_dir, exist_ok=True)

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


def generate_launch_description() -> LaunchDescription:
    """
    @brief Generate a PGM map from a Gazebo .sdf world file.

    @return LaunchDescription that invokes pgm_map_creator and writes a YAML sidecar.
    """
    # Resolve the maps directory (sibling of launch/)
    maps_dir = os.path.join(
        os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'maps'
    )

    pgm_map_creator_dir = get_package_share_directory('pgm_map_creator')

    # ─── Launch arguments ─────────────────────────────────────────────────────

    declare_world_name = DeclareLaunchArgument(
        'world_name',
        description='Filename of the Gazebo .sdf world (e.g. my_world.sdf)',
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

    # ─── Include pgm_map_creator ──────────────────────────────────────────────

    generate_map = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([pgm_map_creator_dir, 'launch', 'generate_map.launch.py'])
        ),
        launch_arguments=[
            ('world_name', LaunchConfiguration('world_name')),
            ('output_path', maps_dir),
            ('xmin', LaunchConfiguration('xmin')),
            ('xmax', LaunchConfiguration('xmax')),
            ('ymin', LaunchConfiguration('ymin')),
            ('ymax', LaunchConfiguration('ymax')),
            ('scan_height', LaunchConfiguration('scan_height')),
            ('resolution', LaunchConfiguration('resolution')),
        ],
    )

    # ─── Write companion YAML after map generation ────────────────────────────

    write_yaml = OpaqueFunction(function=_write_yaml)

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
            generate_map,
            write_yaml,
        ]
    )
