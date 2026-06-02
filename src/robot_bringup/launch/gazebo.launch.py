#!/usr/bin/env python3
"""
@brief Gazebo Harmonic simulation launch using gz_ros2_control.

Starts the Gazebo server and GUI, publishes the robot description, spawns the
robot entity (which activates gz_ros2_control inside Gazebo), spawns controllers,
bridges sensor topics, and launches the ball chaser node and RViz.

Note: The controller_manager runs INSIDE Gazebo via the gz_ros2_control plugin.
We do NOT launch a standalone ros2_control_node here — only the spawners.
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, TimerAction
from launch.substitutions import Command, LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue
from launch_ros.substitutions import FindPackageShare


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch Gazebo Harmonic with gz_ros2_control managing the diff-drive.

    The gz_ros2_control plugin (declared in robot.urdf.xacro) creates the
    controller_manager inside Gazebo's process. We only need to:
    1. Start Gazebo
    2. Publish robot_description (robot_state_publisher)
    3. Spawn the robot entity into Gazebo
    4. Spawn controllers (they connect to gz_ros2_control's controller_manager)
    5. Bridge sensor topics that ros2_control doesn't handle

    @return LaunchDescription.
    """
    pkg_description_dir = get_package_share_directory('robot_description')

    urdf_file = PathJoinSubstitution([pkg_description_dir, 'urdf', 'robot.urdf.xacro'])
    diff_drive_controller_yaml = PathJoinSubstitution(
        [pkg_description_dir, 'config', 'diff_drive_controller.yaml']
    )

    declare_world = DeclareLaunchArgument(
        'world',
        default_value='my_apartment.sdf',
        description='World SDF filename, resolved via GZ_SIM_RESOURCE_PATH',
    )

    # ─── Gazebo server and GUI ────────────────────────────────────────────────

    gazebo_server = ExecuteProcess(
        cmd=['gz', 'sim', '-s', '-r', '-v', '3', LaunchConfiguration('world')],
        output='screen',
    )

    gazebo_gui = ExecuteProcess(
        cmd=['gz', 'sim', '-g', '-v', '3'],
        output='screen',
    )

    # ─── Robot state publisher ────────────────────────────────────────────────

    robot_description_content = ParameterValue(
        Command(['xacro ', urdf_file, ' hardware_plugin:=gz_ros2_control/GazeboSimSystem']),
        value_type=str,
    )

    robot_state_publisher_node = Node(
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[
            {
                'robot_description': robot_description_content,
                'use_sim_time': True,
            }
        ],
    )

    # ─── Spawn robot into Gazebo ──────────────────────────────────────────────

    gz_spawn_entity = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=['-topic', 'robot_description', '-name', 'robot'],
        output='screen',
    )

    # ─── Controller spawners ──────────────────────────────────────────────────
    # These connect to the controller_manager created by gz_ros2_control inside
    # Gazebo. We delay them to give Gazebo time to spawn the entity and
    # initialize the hardware interface.

    joint_state_broadcaster_spawner = TimerAction(
        period=5.0,
        actions=[
            Node(
                package='controller_manager',
                executable='spawner',
                arguments=[
                    'joint_state_broadcaster',
                    '--controller-manager-timeout',
                    '30',
                ],
                output='screen',
            ),
        ],
    )

    diff_drive_controller_spawner = TimerAction(
        period=5.0,
        actions=[
            Node(
                package='controller_manager',
                executable='spawner',
                arguments=[
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

    # ─── ROS-Gz bridge (sensors only) ────────────────────────────────────────

    ros_gz_bridge_node = Node(
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
            '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
            '/camera/rgb/image_raw@sensor_msgs/msg/Image[gz.msgs.Image',
            '/camera/rgb/camera_info@sensor_msgs/msg/CameraInfo[gz.msgs.CameraInfo',
        ],
        output='screen',
    )

    # ─── Application nodes ────────────────────────────────────────────────────

    ball_chaser_node = Node(
        package='robot_control',
        executable='ball_chaser_main',
        output='screen',
        parameters=[{'use_sim_time': True}],
    )

    rviz_node = Node(
        package='rviz2',
        executable='rviz2',
        arguments=[
            '-d',
            PathJoinSubstitution([FindPackageShare('robot_bringup'), 'launch', 'display.rviz']),
        ],
        parameters=[{'use_sim_time': True}],
    )

    return LaunchDescription(
        [
            declare_world,
            gazebo_server,
            gazebo_gui,
            robot_state_publisher_node,
            gz_spawn_entity,
            joint_state_broadcaster_spawner,
            diff_drive_controller_spawner,
            ros_gz_bridge_node,
            ball_chaser_node,
            rviz_node,
        ]
    )
