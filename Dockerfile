##
# @brief Robot launch Dockerfile (mock or real hardware, no Gazebo).
#
# Runs robot.launch.py with a standalone ros2_control_node. The hardware_plugin
# argument selects mock or real hardware:
#
#   docker compose up                                          # mock (default)
#   docker compose run robot ros2 launch robot_bringup \
#       robot.launch.py hardware_plugin:=my_hw/RealSystem     # real
#
# Includes the same ros2_control source patch as Dockerfile.gazebo — ROS 2
# launch injects --params-file via --ros-args when using Node(parameters=[...]),
# and determine_controller_node_options() forwards those to controllers.
##

FROM ros:jazzy-ros-base

# Install ros2_control stack, RViz, and build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-jazzy-ros2-controllers \
    ros-jazzy-xacro \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-rviz2 \
    ros-jazzy-example-interfaces \
    python3-colcon-common-extensions \
    python3-rosdep \
    git \
    && rm -rf /var/lib/apt/lists/*

# ─── Build patched ros2_control from source ───────────────────────────────────
# Filter --params-file in determine_controller_node_options() to prevent
# forwarding to controllers (same patch as Dockerfile.gazebo).
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

# ─── Build robot workspace ────────────────────────────────────────────────────
WORKDIR /ros2_ws
COPY src/ src/

RUN rosdep install --from-paths src --ignore-src -r -y \
    --skip-keys "rapidcheck pytest irobot_create_description irobot_create_msgs slam_toolbox nav2_bringup nav2_simple_commander joint_state_publisher gz_ros2_control ros_gz_sim ros2_control" \
    || true

RUN . /opt/ros/jazzy/setup.sh && \
    . /ros2_control_ws/install/setup.sh && \
    colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release \
    --packages-up-to robot_bringup robot_control

COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["ros2", "launch", "robot_bringup", "robot.launch.py"]
