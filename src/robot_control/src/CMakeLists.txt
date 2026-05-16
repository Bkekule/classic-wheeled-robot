find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(custom_interfaces REQUIRED)

# --- ball_chaser_utils: pure domain logic, no ROS dependency ---
add_library(ball_chaser_utils STATIC
  ball_chaser/image_utils.cpp
)
target_include_directories(ball_chaser_utils PUBLIC
  $<BUILD_INTERFACE:${ROBOT_CONTROL_DIR}/include/ball_chaser>
  $<INSTALL_INTERFACE:include/ball_chaser>
)

# --- drive_bot node library (linked by both executable and tests) ---
add_library(drive_bot_node OBJECT
  servers/ball_chaser/drive_bot_node.cpp
)
target_include_directories(drive_bot_node PUBLIC
  $<BUILD_INTERFACE:${ROBOT_CONTROL_DIR}/include/servers/ball_chaser>
  $<INSTALL_INTERFACE:include/servers/ball_chaser>
)
ament_target_dependencies(drive_bot_node
  rclcpp
  geometry_msgs
  custom_interfaces
)

# --- process_image node library ---
add_library(process_image_node OBJECT
  clients/ball_chaser/process_image_node.cpp
)
target_include_directories(process_image_node PUBLIC
  $<BUILD_INTERFACE:${ROBOT_CONTROL_DIR}/include/clients/ball_chaser>
  $<INSTALL_INTERFACE:include/clients/ball_chaser>
)
target_link_libraries(process_image_node ball_chaser_utils)
ament_target_dependencies(process_image_node
  rclcpp
  sensor_msgs
  custom_interfaces
)

# --- process_image_main executable ---
add_executable(process_image_main
  clients/ball_chaser/process_image_main.cpp
)
target_link_libraries(process_image_main process_image_node)

install(TARGETS process_image_main
  DESTINATION lib/${PROJECT_NAME}
)

# --- drive_bot_main executable ---
add_executable(drive_bot_main
  servers/ball_chaser/drive_bot_main.cpp
)
target_link_libraries(drive_bot_main drive_bot_node)

install(TARGETS drive_bot_main
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY ${ROBOT_CONTROL_DIR}/include/
  DESTINATION include
)
