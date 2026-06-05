#!/usr/bin/env python3
"""
@brief Data recording launch for capturing key system topics to a rosbag.

Records camera, velocity, odometry, scan, state, and diagnostics topics
for behavioral regression testing and post-mortem debugging.
"""

from datetime import datetime

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration


## @brief ROS 2 topics recorded for behavioral regression testing and debugging.  # noqa: E266
TOPICS = [
    '/camera/rgb/image_raw',  # Raw camera frames for replay and vision debugging
    '/cmd_vel',  # Velocity commands sent to the robot
    '/odom',  # Odometry from diff_drive_controller
    '/scan',  # Lidar scan data
    '/ball_chaser/state',  # Behavior state machine transitions (Idle/Tracking/Lost)
    '/diagnostics',  # Node health and detection statistics
]


def generate_launch_description() -> LaunchDescription:
    """
    @brief Launch ros2 bag record for key system topics.

    @return LaunchDescription with bag recording process.
    """
    default_bag_path = f'rosbag2_{datetime.now().strftime("%Y_%m_%d-%H_%M_%S")}'

    declare_bag_path = DeclareLaunchArgument(
        'bag_path',
        default_value=default_bag_path,
        description='Output directory for the recorded bag',
    )

    record_process = ExecuteProcess(
        cmd=['ros2', 'bag', 'record', '--output', LaunchConfiguration('bag_path')] + TOPICS,
        output='screen',
    )

    return LaunchDescription(
        [
            declare_bag_path,
            record_process,
        ]
    )
