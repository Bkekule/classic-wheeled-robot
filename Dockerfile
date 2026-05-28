FROM ros:jazzy-ros-base

# Install build tools
RUN apt-get update && apt-get install -y --no-install-recommends \
    python3-colcon-common-extensions \
    python3-rosdep \
    && rm -rf /var/lib/apt/lists/*

# Create workspace
WORKDIR /ros2_ws
COPY src/ src/

# Install dependencies via rosdep
RUN rosdep update && \
    rosdep install --from-paths src --ignore-src -r -y

# Build
RUN . /opt/ros/jazzy/setup.sh && \
    colcon build --cmake-args -DCMAKE_BUILD_TYPE=Release

# Default entrypoint runs tests
CMD ["/bin/bash", "-c", ". /opt/ros/jazzy/setup.sh && . install/setup.sh && colcon test && colcon test-result --verbose"]
