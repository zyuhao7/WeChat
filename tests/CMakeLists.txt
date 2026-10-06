# Unit tests for the C++ backends. Run with: ctest --test-dir build --output-on-failure
find_package(GTest REQUIRED)

# The services generate their protobuf sources into the build tree inside the top-level
# foreach; regenerate here instead of sharing those, so a test target never perturbs the
# service build and can be built on its own.
set(TEST_GEN_DIR "${CMAKE_BINARY_DIR}/gen/tests/GateServer")
file(MAKE_DIRECTORY "${TEST_GEN_DIR}")

add_custom_command(
  OUTPUT "${TEST_GEN_DIR}/message.pb.cc" "${TEST_GEN_DIR}/message.pb.h"
         "${TEST_GEN_DIR}/message.grpc.pb.cc" "${TEST_GEN_DIR}/message.grpc.pb.h"
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
          --proto_path="${CMAKE_SOURCE_DIR}/GateServer"
          --cpp_out="${TEST_GEN_DIR}"
          --grpc_out="${TEST_GEN_DIR}"
          --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
          "${CMAKE_SOURCE_DIR}/GateServer/message.proto"
  DEPENDS "${CMAKE_SOURCE_DIR}/GateServer/message.proto"
  COMMENT "protoc tests/GateServer/message.proto"
)

add_executable(test_gate_proto
  test_gate_proto.cpp
  "${TEST_GEN_DIR}/message.pb.cc"
  "${TEST_GEN_DIR}/message.grpc.pb.cc"
)
target_include_directories(test_gate_proto PRIVATE
  "${TEST_GEN_DIR}"
  ${Protobuf_INCLUDE_DIRS}
)
target_link_libraries(test_gate_proto PRIVATE
  GTest::gtest
  GTest::gtest_main
  gRPC::grpc++
  ${Protobuf_LIBRARIES}
  Threads::Threads
)

add_test(NAME gate_proto COMMAND test_gate_proto)
