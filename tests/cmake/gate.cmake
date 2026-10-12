add_executable(test_gate_http_parse
  test_gate_http_parse.cpp
  ${CMAKE_SOURCE_DIR}/GateServer/HttpParse.cpp
  ${CMAKE_SOURCE_DIR}/GateServer/UrlCodec.cpp
)
target_include_directories(test_gate_http_parse PRIVATE "${CMAKE_SOURCE_DIR}/GateServer")
target_link_libraries(test_gate_http_parse PRIVATE
  GTest::gtest
  GTest::gtest_main
  JsonCpp::JsonCpp
)
add_test(NAME gate_http_parse COMMAND test_gate_http_parse)
