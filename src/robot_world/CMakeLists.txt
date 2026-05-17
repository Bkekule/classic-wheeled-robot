cmake_minimum_required(VERSION 3.8...3.22)
project(robot_world)

find_package(ament_cmake REQUIRED)

install(FILES my_apartment.sdf DESTINATION share/${PROJECT_NAME})
install(DIRECTORY models DESTINATION share/${PROJECT_NAME})

ament_package()
