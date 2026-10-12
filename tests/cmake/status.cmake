# ServerPicker is dependency-free, so this target needs neither protobuf, gRPC nor Redis.
add_executable(test_status_server_picker
  test_status_server_picker.cpp
  "${CMAKE_SOURCE_DIR}/StatusServer/ServerPicker.cpp"
)
target_include_directories(test_status_server_picker PRIVATE "${CMAKE_SOURCE_DIR}/StatusServer")
target_link_libraries(test_status_server_picker PRIVATE
  GTest::gtest
  GTest::gtest_main
  Threads::Threads
)

add_test(NAME status_server_picker COMMAND test_status_server_picker)
