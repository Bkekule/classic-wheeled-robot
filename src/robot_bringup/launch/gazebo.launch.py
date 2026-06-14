#!/usr/bin/env python3
"""
@brief Gazebo Harmonic simulation launch using gz_ros2_control.

Wraps robot_core.launch.py with Gazebo-specific additions:
- Gazebo server and GUI
- Entity spawning into the sim world
- ROS-Gz bridge for sensor topics
- RViz visualization

The controller_manager runs INSIDE Gazebo via the gz_ros2_control plugin
(declared in robot.urdf.xacro). We do NOT launch a standalone ros2_control_node.
"""

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch Gazebo Harmonic with gz_ros2_control managing the diff-drive.

    1. Include robot_core (RSP + spawners + ball_chaser)
    2. Start Gazebo server and GUI
    3. Spawn the robot entity into Gazebo
    4. Bridge sensor topics (clock, lidar, camera)
    5. Launch RViz

    @return LaunchDescription.
    """
    pkg_bringup_dir = get_package_share_directory('robot_bringup')

    declare_world = DeclareLaunchArgument(
        'world',
        default_value='my_apartment.sdf',
        description='World SDF filename, resolved via GZ_SIM_RESOURCE_PATH',
    )

    declare_launch_ball_chaser = DeclareLaunchArgument(
        'launch_ball_chaser',
        default_value='true',
        description='Whether to launch the ball_chaser node',
    )

    # ─── Shared core (RSP + spawners + ball_chaser) ───────────────────────────

    core = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution([pkg_bringup_dir, 'launch', 'robot_core.launch.py'])
        ),
        launch_arguments=[
            ('hardware_plugin', 'gz_ros2_control/GazeboSimSystem'),
            ('use_sim_time', 'true'),
            ('launch_ball_chaser', LaunchConfiguration('launch_ball_chaser')),
        ],
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

    # ─── Spawn robot into Gazebo ──────────────────────────────────────────────

    gz_spawn_entity = Node(
        package='ros_gz_sim',
        executable='create',
        arguments=['-topic', 'robot_description', '-name', 'robot'],
        output='screen',
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

    # ─── RViz ─────────────────────────────────────────────────────────────────

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
            declare_launch_ball_chaser,
            core,
            gazebo_server,
            gazebo_gui,
            gz_spawn_entity,
            ros_gz_bridge_node,
            rviz_node,
        ]
    )
