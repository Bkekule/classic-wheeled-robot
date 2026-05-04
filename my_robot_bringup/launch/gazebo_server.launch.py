#!/usr/bin/env python3

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import PathJoinSubstitution, Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description() -> LaunchDescription:
    """Launch Gazebo Harmonic with the robot."""

    pkg_description_dir = get_package_share_directory("my_robot_description")
    get_package_share_directory("my_robot_bringup")

    urdf_file = PathJoinSubstitution(
        [pkg_description_dir, "urdf", "my_robot.urdf.xacro"]
    )

    # Declare arguments
    declare_world = DeclareLaunchArgument(
        "world",
        default_value="empty",
        description="Gazebo world to load"
    )

    # Expand xacro to URDF
    robot_description_content = ParameterValue(
        Command(["xacro ", urdf_file]),
        value_type=str
    )

    # Robot state publisher
    robot_state_publisher_node = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[
            {
                "robot_description": robot_description_content,
                "use_sim_time": True,
            }
        ],
    )

    # Drop gz-msgs descriptor double-registration spam (cosmetic, harmless,
    # comes from every process linked against gz-msgs at startup).
    gz_noise_re = (
        r"descriptor(_database)?\.cc"
        r"|DynamicFactory\(\)"
        r"|absl::InitializeLog"
    )
    gz_quiet_prefix = (
        f"zsh -c 'exec \"$@\" 2> >(grep -vE \"{gz_noise_re}\" >&2)' --"
    )

    # Gazebo server
    gzserver_node = ExecuteProcess(
        cmd=["zsh", "-c",
             f"exec gz sim -s -v4 2> >(grep -vE \"{gz_noise_re}\" >&2)"],
        additional_env={
            "GZ_SIM_SYSTEM_PLUGIN_PATH": "",
            "GZ_SIM_RESOURCE_PATH": "",
        },
        output="screen",
    )

    # Create a service caller to spawn the robot
    # This will be called after Gazebo is up
    spawn_robot_service = Node(
        package="ros_gz_sim",
        executable="create",
        arguments=[
            "-topic", "robot_description",
            "-name", "my_robot",
        ],
        prefix=gz_quiet_prefix,
        output="screen",
    )

    # ROS-Gazebo bridge for sensors only.
    # /cmd_vel and /odom are handled entirely by diff_drive_controller in ROS space
    # and do not need bridging.
    ros_gz_bridge_node = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        arguments=[
            "/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan",
            "/camera/image_raw@sensor_msgs/msg/Image[gz.msgs.Image",
        ],
        prefix=gz_quiet_prefix,
        output="screen",
    )

    return LaunchDescription(
        [
            declare_world,
            robot_state_publisher_node,
            gzserver_node,
            spawn_robot_service,
            ros_gz_bridge_node,
        ]
    )
