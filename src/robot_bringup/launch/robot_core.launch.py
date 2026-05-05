#!/usr/bin/env python3

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import RegisterEventHandler
from launch.event_handlers import OnProcessStart
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description() -> LaunchDescription:
    """
    Core robot bringup shared by display and gazebo launches.

    Expected arguments (callers must forward or declare them):
        hardware_plugin  — xacro hardware_plugin value
        use_sim_time     — bool string, 'true' or 'false'
    """
    pkg_description_dir = get_package_share_directory('robot_description')
    pkg_bringup_dir = get_package_share_directory('robot_bringup')

    urdf_file = PathJoinSubstitution(
        [pkg_description_dir, 'urdf', 'my_robot.urdf.xacro']
    )
    controllers_yaml = PathJoinSubstitution(
        [pkg_bringup_dir, 'config', 'controllers.yaml']
    )

    robot_description_content = ParameterValue(
        Command([
            'xacro ',
            urdf_file,
            ' hardware_plugin:=',
            LaunchConfiguration('hardware_plugin')
        ]),
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

    controller_manager_node = Node(
        package='controller_manager',
        executable='ros2_control_node',
        parameters=[
            {
                'robot_description': robot_description_content,
                'use_sim_time': LaunchConfiguration('use_sim_time'),
            },
            controllers_yaml,
        ],
        output='screen',
    )

    joint_state_broadcaster_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['joint_state_broadcaster'],
    )

    diff_drive_controller_spawner = Node(
        package='controller_manager',
        executable='spawner',
        arguments=['diff_drive_controller'],
    )

    spawn_controllers = RegisterEventHandler(
        OnProcessStart(
            target_action=controller_manager_node,
            on_start=[joint_state_broadcaster_spawner, diff_drive_controller_spawner],
        )
    )

    return LaunchDescription(
        [
            robot_state_publisher_node,
            controller_manager_node,
            spawn_controllers,
        ]
    )
