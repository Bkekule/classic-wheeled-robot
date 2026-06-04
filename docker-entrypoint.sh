#!/bin/bash
## @brief Entrypoint script that sources all workspaces before executing the command.
set -e

source /opt/ros/jazzy/setup.bash

# Source patched ros2_control overlay if present (Dockerfile.gazebo only)
if [ -f /ros2_control_ws/install/setup.bash ]; then
    source /ros2_control_ws/install/setup.bash
fi

source /ros2_ws/install/setup.bash

exec "$@"
