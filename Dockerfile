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
# No ros2_control source patch needed here — the standalone ros2_control_node
# receives parameters via the Node() parameters list, not --params-file CLI
# injection, so determine_controller_node_options() has nothing to forward.
##

FROM ros:jazzy-ros-base

# Install ros2_control stack, RViz, and build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    ros-jazzy-ros2-control \
    ros-jazzy-ros2-controllers \
    ros-jazzy-xacro \
    ros-jazzy-robot-state-publisher \
    ros-jazzy-rviz2 \
    python3-colcon-common-extensions \
    python3-rosdep \
    && rm -rf /var/lib/apt/lists/*

# ─── Build robot workspace ────────────────────────────────────────────────────
WORKDIR /ros2_ws
COPY src/ src/

RUN rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y \
    --skip-keys "rapidcheck pytest irobot_create_description irobot_create_msgs slam_toolbox nav2_bringup nav2_simple_commander joint_state_publisher gz_ros2_control ros_gz_sim" \
    || true

RUN . /opt/ros/jazzy/setup.sh && \
    colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release \
    --packages-up-to robot_bringup robot_control

COPY docker-entrypoint.sh /docker-entrypoint.sh
RUN chmod +x /docker-entrypoint.sh
ENTRYPOINT ["/docker-entrypoint.sh"]
CMD ["ros2", "launch", "robot_bringup", "robot.launch.py"]
