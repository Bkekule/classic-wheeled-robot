#!/bin/bash
## @brief Entrypoint script that sources the ROS2 workspace before executing the command.
set -e

# Source ROS2 and workspace
source /opt/ros/jazzy/setup.bash
source /ros2_ws/install/setup.bash

exec "$@"
