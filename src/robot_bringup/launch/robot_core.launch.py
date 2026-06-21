#!/usr/bin/env python3
"""
@brief Shared robot core launch: RSP, controller spawners, and application nodes.

This is the hardware-agnostic core included by both gazebo.launch.py and
robot.launch.py. It does NOT start a controller_manager — that's the caller's
responsibility (Gazebo creates it via gz_ros2_control, robot.launch.py
launches ros2_control_node).

Expected arguments (callers must declare and forward):
    hardware_plugin  — xacro hardware_plugin value
    use_sim_time     — bool string, 'true' or 'false'
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, TimerAction
from launch.conditions import IfCondition
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description() -> LaunchDescription:
    """
    @brief Shared robot bringup core.

    Launches:
    - robot_state_publisher (with xacro hardware_plugin substitution)
    - Controller spawners (joint_state_broadcaster, diff_drive_controller)
    - ball_chaser application node

    The controller spawners use ExecuteProcess + TimerAction to avoid
    --ros-args injection issues and give the CM time to initialize.

    @return LaunchDescription.
    """
    pkg_description_dir = get_package_share_directory('robot_description')

    urdf_file = PathJoinSubstitution([pkg_description_dir, 'urdf', 'robot.urdf.xacro'])

    # ─── Robot state publisher ────────────────────────────────────────────────

    robot_description_content = ParameterValue(
        Command(
            ['xacro ', urdf_file, ' hardware_plugin:=', LaunchConfiguration('hardware_plugin')]
        ),
        value_type=str,
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[
            {
                'robot_description': robot_description_content,
                'use_sim_time': LaunchConfiguration('use_sim_time'),
            }
        ],
    )

    # ─── Controller spawners ──────────────────────────────────────────────────
    # --param-file passes controller-specific parameters (wheel names, etc.)
    # to each controller via the CM's parameter service.

    diff_drive_controller_yaml = PathJoinSubstitution(
        [pkg_description_dir, 'config', 'diff_drive_controller.yaml']
    )

    joint_state_broadcaster_spawner = TimerAction(
        period=8.0,
        actions=[
            ExecuteProcess(
                cmd=[
                    'ros2',
                    'run',
                    'controller_manager',
                    'spawner',
                    'joint_state_broadcaster',
                    '--controller-manager-timeout',
                    '30',
                ],
                output='screen',
            ),
        ],
    )

    diff_drive_controller_spawner = TimerAction(
        period=8.0,
        actions=[
            ExecuteProcess(
                cmd=[
                    'ros2',
                    'run',
                    'controller_manager',
                    'spawner',
                    'diff_drive_controller',
                    '--param-file',
                    diff_drive_controller_yaml,
                    '--controller-manager-timeout',
                    '30',
                ],
                output='screen',
            ),
        ],
    )

    # ─── Application nodes ────────────────────────────────────────────────────

    ball_chaser_node = Node(
        package='robot_control',
        executable='ball_chaser_main',
        output='screen',
        parameters=[{'use_sim_time': LaunchConfiguration('use_sim_time')}],
        remappings=[('/cmd_vel', '/diff_drive_controller/cmd_vel')],
        condition=IfCondition(LaunchConfiguration('launch_ball_chaser')),
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                'hardware_plugin',
                description='ros2_control hardware plugin class name',
            ),
            DeclareLaunchArgument(
                'use_sim_time',
                default_value='false',
                description='Use simulation clock',
            ),
            DeclareLaunchArgument(
                'launch_ball_chaser',
                default_value='true',
                description='Whether to launch the ball_chaser node',
            ),
            robot_state_publisher_node,
            joint_state_broadcaster_spawner,
            diff_drive_controller_spawner,
            ball_chaser_node,
        ]
    )
