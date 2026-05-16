cmake_minimum_required(VERSION 3.8...3.22)
project(robot_control)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(ENABLE_ASAN  "Enable AddressSanitizer (memory errors)"  OFF)
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer"         ON)
option(ENABLE_TSAN  "Enable ThreadSanitizer (race conditions)"  OFF)

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan are mutually exclusive — enable only one at a time.")
endif()

if(ENABLE_ASAN) # Limit to staging environments or production environments where timing and memory overhead is palatable
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN) # Just a tad slower. Can be used in production sparingly.
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

if(ENABLE_TSAN) # Only when investigating race conditions, as it has significant performance overhead
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

find_package(ament_cmake REQUIRED)
set(ROBOT_CONTROL_DIR ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(src)
add_subdirectory(tests)

ament_package()
