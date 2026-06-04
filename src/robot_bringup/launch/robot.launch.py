#!/usr/bin/env python3
"""
@brief Main robot launch for real or mock hardware with RViz.

Wraps robot_core.launch.py with a standalone ros2_control_node and RViz.
The hardware_plugin argument selects the ros2_control hardware interface:

    # Mock hardware (visualization only):
    ros2 launch robot_bringup robot.launch.py

    # Real hardware:
    ros2 launch robot_bringup robot.launch.py hardware_plugin:=my_robot_hw/RealSystem

For Gazebo simulation, use gazebo.launch.py instead (the CM lives inside Gazebo).
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch the robot with a standalone controller_manager and RViz.

    @return LaunchDescription with robot_core, ros2_control_node, and RViz.
    """
    pkg_bringup_dir = get_package_share_directory('robot_bringup')
    pkg_description_dir = get_package_share_directory('robot_description')

    urdf_file = PathJoinSubstitution([pkg_description_dir, 'urdf', 'robot.urdf.xacro'])
    controllers_yaml = PathJoinSubstitution([pkg_description_dir, 'config', 'controllers.yaml'])

    declare_hardware_plugin = DeclareLaunchArgument(
        'hardware_plugin',
        default_value='mock_components/GenericSystem',
        description='ros2_control hardware plugin class name',
    )

    declare_use_sim_time = DeclareLaunchArgument(
        'use_sim_time',
        default_value='false',
        description='Use simulation clock',
    )

    # ─── Shared core (RSP + spawners + ball_chaser) ───────────────────────────

    core = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([pkg_bringup_dir, 'launch', 'robot_core.launch.py'])
        ),
        launch_arguments=[
            ('hardware_plugin', LaunchConfiguration('hardware_plugin')),
            ('use_sim_time', LaunchConfiguration('use_sim_time')),
        ],
    )

    # ─── Standalone controller_manager ────────────────────────────────────────
    # Required for mock/real hardware where no simulator plugin creates the CM.

    robot_description_content = ParameterValue(
        Command(
            [
                'xacro ',
                urdf_file,
                ' hardware_plugin:=',
                LaunchConfiguration('hardware_plugin'),
            ]
        ),
        value_type=str,
    )

    controller_manager_node = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[
            {'robot_description': robot_description_content},
            controllers_yaml,
        ],
        output='screen',
    )

    # ─── RViz ─────────────────────────────────────────────────────────────────

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        name='rviz2',
        arguments=['-d', PathJoinSubstitution([pkg_bringup_dir, 'launch', 'display.rviz'])],
        parameters=[{'use_sim_time': LaunchConfiguration('use_sim_time')}],
    )

    return LaunchDescription(
        [
            declare_hardware_plugin,
            declare_use_sim_time,
            core,
            controller_manager_node,
            rviz_node,
        ]
    )
