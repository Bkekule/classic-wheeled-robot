if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_uncrustify_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  find_package(rapidcheck QUIET)

  # ─── Integration tests (no rapidcheck dependency) ───────────────────────────

  ament_add_gtest(test_ball_chaser_node
    integration/ball_chaser/test_ball_chaser_node.cpp
  )
  target_link_libraries(test_ball_chaser_node ball_chaser_node)

  ament_add_gtest(test_intent_service
    integration/ball_chaser/test_intent_service.cpp
  )
  target_link_libraries(test_intent_service ball_chaser_node)

  # ─── Unit + property tests (require rapidcheck) ─────────────────────────────

  if(rapidcheck_FOUND)
    ament_add_gtest(test_image_utils
      unit/ball_chaser/test_image_utils.cpp
    )
    target_link_libraries(test_image_utils ball_chaser_utils rapidcheck)

    ament_add_gtest(test_state_machine
      unit/ball_chaser/test_state_machine.cpp
    )
    target_link_libraries(test_state_machine ball_chaser_utils rapidcheck)
  else()
    message(STATUS "rapidcheck not found — skipping property-based unit tests")
  endif()

endif()
