if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_uncrustify_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_image_utils
    unit/ball_chaser/test_image_utils.cpp
  )
  target_link_libraries(test_image_utils ball_chaser_utils)

  ament_add_gtest(test_drive_bot
    integration/ball_chaser/test_drive_bot.cpp
  )
  target_link_libraries(test_drive_bot drive_bot_node)
endif()
