##
# @brief Unified robot Dockerfile (ROS 2 Jazzy).
#
# Supports two modes via the GAZEBO build arg:
#   GAZEBO=false (default) — robot.launch.py with standalone ros2_control_node
#   GAZEBO=true            — gazebo.launch.py with gz_ros2_control plugin
#
# ─── ros2_control patch ───────────────────────────────────────────────────────
#
# Builds ros2_control from source with a patch to
# determine_controller_node_options() that filters out --params-file arguments
# before forwarding CM node options to individual controllers.
#
# Without this patch, the CM passes its own parameter files to each controller
# node, causing "parameter not declared" or parse errors.
#
# See docker/ros2_control_params_file_patch.py for patch details.
#
# ─── turtlebot4 ───────────────────────────────────────────────────────────────
#
# Builds turtlebot4 from source so that turtlebot4 launch files and messages
# are available for integration (e.g. ros2 launch turtlebot4_gz_bringup).
##

FROM ros:jazzy-ros-base

ARG GAZEBO=false

# ─── Install base dependencies ────────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-jazzy-ros2-control \
    ros-jazzy-ros2-controllers \
    ros-jazzy-ros2-control-cmake \
    ros-jazzy-xacro \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-rviz2 \
    ros-jazzy-example-interfaces \
    python3-colcon-common-extensions \
    python3-rosdep \
    git \
    && rm -rf /var/lib/apt/lists/*

# ─── Install Gazebo packages (only when GAZEBO=true) ──────────────────────────
RUN if [ "$GAZEBO" = "true" ]; then \
        apt-get update && apt-get install -y --no-install-recommends \
            ros-jazzy-gz-ros2-control \
            ros-jazzy-ros-gz \
        && rm -rf /var/lib/apt/lists/*; \
    fi

# ─── Build patched ros2_control from source ───────────────────────────────────
WORKDIR /ros2_control_ws
COPY docker/ros2_control_params_file_patch.py /tmp/ros2_control_patch.py
RUN git clone --branch jazzy --depth 1 \
    https://github.com/ros-controls/ros2_control.git src/ros2_control && \
    python3 /tmp/ros2_control_patch.py \
    src/ros2_control/controller_manager/src/controller_manager.cpp && \
    . /opt/ros/jazzy/setup.sh && \
    rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y \
    --skip-keys "ros2controlcli" || true && \
    colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release

# ─── Build turtlebot4 from source ────────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-jazzy-irobot-create-description \
    ros-jazzy-irobot-create-msgs \
    ros-jazzy-turtlebot4-simulator \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /turtlebot4_ws
RUN git clone --branch jazzy --depth 1 \
    https://github.com/turtlebot/turtlebot4.git src/turtlebot4 && \
    . /opt/ros/jazzy/setup.sh && \
    . /ros2_control_ws/install/setup.sh && \
    rosdep install --from-paths src --ignore-src -r -y || true && \
    colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release

# ─── Build robot workspace ────────────────────────────────────────────────────
WORKDIR /ros2_ws
COPY src/ src/

RUN rosdep install --from-paths src --ignore-src -r -y \
    --skip-keys "rapidcheck pytest irobot_create_description irobot_create_msgs slam_toolbox nav2_bringup nav2_simple_commander joint_state_publisher gz_ros2_control ros_gz_sim ros2_control turtlebot4_msgs turtlebot4_description turtlebot4_navigation turtlebot4_node" \
    || true

RUN . /opt/ros/jazzy/setup.sh && \
    . /ros2_control_ws/install/setup.sh && \
    . /turtlebot4_ws/install/setup.sh && \
    if [ "$GAZEBO" = "true" ]; then \
        colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release \
            --packages-up-to robot_bringup robot_control robot_world; \
    else \
        colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release \
            --packages-up-to robot_bringup robot_control; \
    fi

COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["ros2", "launch", "robot_bringup", "robot.launch.py"]
