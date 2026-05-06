#!/usr/bin/env python3

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import ExecuteProcess, RegisterEventHandler
from launch.event_handlers import OnProcessStart
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node, SetParameter
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackagePrefix


def generate_launch_description() -> LaunchDescription:
    """
    Core robot bringup shared by display and gazebo launches.

    Expected arguments (callers must forward or declare them):
        hardware_plugin  — xacro hardware_plugin value
        use_sim_time     — bool string, 'true' or 'false'
    """
    pkg_description_dir = get_package_share_directory('robot_description')

    urdf_file = PathJoinSubstitution(
        [pkg_description_dir, 'urdf', 'robot.urdf.xacro']
    )
    controllers_yaml = PathJoinSubstitution(
        [pkg_description_dir, 'config', 'controllers.yaml']
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
                'update_rate': 100,
                'diff_drive_controller': {'type': 'diff_drive_controller/DiffDriveController'},
                'joint_state_broadcaster': {'type': 'joint_state_broadcaster/JointStateBroadcaster'},
            }
        ],
        output='screen',
    )

    spawner_bin = PathJoinSubstitution(
        [FindPackagePrefix('controller_manager'), 'lib', 'controller_manager', 'spawner']
    )

    joint_state_broadcaster_spawner = ExecuteProcess(
        cmd=[spawner_bin, 'joint_state_broadcaster'],
        output='screen',
    )

    diff_drive_controller_spawner = ExecuteProcess(
        cmd=[spawner_bin, 'diff_drive_controller', '--param-file', controllers_yaml],
        output='screen',
    )

    spawn_controllers = RegisterEventHandler(
        OnProcessStart(
            target_action=controller_manager_node,
            on_start=[joint_state_broadcaster_spawner, diff_drive_controller_spawner],
        )
    )

    return LaunchDescription(
        [
            SetParameter('use_sim_time', LaunchConfiguration('use_sim_time')),
            robot_state_publisher_node,
            controller_manager_node,
            spawn_controllers,
        ]
    )
