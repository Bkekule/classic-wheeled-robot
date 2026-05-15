cmake_minimum_required(VERSION 3.8...3.22)
project(robot_control)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(custom_interfaces REQUIRED)

# --- drive_bot node library (linked by both executable and tests) ---
add_library(drive_bot_node STATIC
  src/servers/ball_chaser/drive_bot_node.cpp
)
target_include_directories(drive_bot_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(drive_bot_node
  rclcpp
  geometry_msgs
  custom_interfaces
)

# --- process_image node library ---
add_library(process_image_node STATIC
  src/clients/ball_chaser/process_image_node.cpp
)
target_include_directories(process_image_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)
ament_target_dependencies(process_image_node
  rclcpp
  sensor_msgs
  custom_interfaces
)

# --- process_image_main executable ---
add_executable(process_image_main
  src/clients/ball_chaser/process_image_main.cpp
)
target_link_libraries(process_image_main process_image_node)

install(TARGETS process_image_main
  DESTINATION lib/${PROJECT_NAME}
)

# --- drive_bot_main executable ---
add_executable(drive_bot_main
  src/servers/ball_chaser/drive_bot_main.cpp
)
target_link_libraries(drive_bot_main drive_bot_node)

install(TARGETS drive_bot_main
  DESTINATION lib/${PROJECT_NAME}
)
install(DIRECTORY include/
  DESTINATION include
)

# --- tests ---
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_uncrustify_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_drive_bot
    test/ball_chaser/test_drive_bot.cpp
  )
  target_link_libraries(test_drive_bot drive_bot_node)
endif()

ament_package()
