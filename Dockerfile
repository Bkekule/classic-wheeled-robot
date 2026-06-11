##
# @brief Unified robot Dockerfile (ROS 2 Jazzy + Gazebo Harmonic).
#
# Based on osrf/ros:jazzy-simulation which includes desktop + Gazebo Harmonic.
# Supports all modes: robot.launch.py (mock/real HW), gazebo.launch.py, turtlebot4.
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
##

FROM osrf/ros:jazzy-simulation

# ─── Install additional dependencies ──────────────────────────────────────────
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-jazzy-ros2-control \
    ros-jazzy-ros2-controllers \
    ros-jazzy-gz-ros2-control \
    ros-jazzy-turtlebot4-simulator \
    ros-jazzy-example-interfaces \
    python3-colcon-common-extensions \
    git \
    && rm -rf /var/lib/apt/lists/*

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

# ─── Build pgm map creator from source ───────────────────────────────────
WORKDIR /pgm_map_creator_ws/src
RUN git clone https://github.com/Bkekule/pgm_map_creator.git

WORKDIR /pgm_map_creator_ws
RUN apt-get update && apt-get install -y --no-install-recommends \
    libprotobuf-dev \
    protobuf-compiler \
    && rm -rf /var/lib/apt/lists/*

RUN . /opt/ros/jazzy/setup.sh && \
    . /ros2_control_ws/install/setup.sh && \
    colcon build --packages-select pgm_map_creator

# ─── Build robot workspace ────────────────────────────────────────────────────
WORKDIR /ros2_ws
COPY src/ src/

RUN rosdep install --from-paths src --ignore-src -r -y \
    --skip-keys "rapidcheck pytest ros2_control" || true

RUN . /opt/ros/jazzy/setup.sh && \
    . /ros2_control_ws/install/setup.sh && \
    colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release \
        --packages-up-to robot_bringup robot_control robot_world

COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh
ENTRYPOINT ["/docker-entrypoint.sh"]
