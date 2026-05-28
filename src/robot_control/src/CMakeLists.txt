find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(std_msgs REQUIRED)
find_package(diagnostic_msgs REQUIRED)
find_package(custom_interfaces REQUIRED)

# --- ball_chaser_utils: pure domain logic, no ROS dependency ---
add_library(ball_chaser_utils STATIC
  ball_chaser/image_utils.cpp
  ball_chaser/state_machine.cpp
)
target_include_directories(ball_chaser_utils PUBLIC
  $<BUILD_INTERFACE:${ROBOT_CONTROL_DIR}/include/ball_chaser>
  $<INSTALL_INTERFACE:include/ball_chaser>
)

# --- ball_chaser_node: unified node library ---
add_library(ball_chaser_node OBJECT
  ball_chaser/ball_chaser_node.cpp
  ball_chaser/ball_chaser_perception.cpp
  ball_chaser/ball_chaser_actuation.cpp
)
target_include_directories(ball_chaser_node PUBLIC
  $<BUILD_INTERFACE:${ROBOT_CONTROL_DIR}/include/ball_chaser>
  $<INSTALL_INTERFACE:include/ball_chaser>
)
target_link_libraries(ball_chaser_node ball_chaser_utils)
ament_target_dependencies(ball_chaser_node
  rclcpp
  geometry_msgs
  sensor_msgs
  std_msgs
  diagnostic_msgs
  custom_interfaces
)

# --- ball_chaser_main executable ---
add_executable(ball_chaser_main
  ball_chaser/ball_chaser_main.cpp
)
target_link_libraries(ball_chaser_main ball_chaser_node)

install(TARGETS ball_chaser_main
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY ${ROBOT_CONTROL_DIR}/include/
  DESTINATION include
)
