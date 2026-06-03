#!/bin/bash
## @brief Entrypoint script that sources all workspaces before executing the command.
set -e

source /opt/ros/jazzy/setup.bash
source /ros2_control_ws/install/setup.bash
source /ros2_ws/install/setup.bash

exec "$@"
