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

# ChatServer's own protobuf, regenerated for the same reason as above.
set(CHAT_GEN_DIR "${CMAKE_BINARY_DIR}/gen/tests/ChatServer")
file(MAKE_DIRECTORY "${CHAT_GEN_DIR}")

add_custom_command(
  OUTPUT "${CHAT_GEN_DIR}/message.pb.cc" "${CHAT_GEN_DIR}/message.pb.h"
         "${CHAT_GEN_DIR}/message.grpc.pb.cc" "${CHAT_GEN_DIR}/message.grpc.pb.h"
  COMMAND ${Protobuf_PROTOC_EXECUTABLE}
          --proto_path="${CMAKE_SOURCE_DIR}/ChatServer"
          --cpp_out="${CHAT_GEN_DIR}"
          --grpc_out="${CHAT_GEN_DIR}"
          --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
          "${CMAKE_SOURCE_DIR}/ChatServer/message.proto"
  DEPENDS "${CMAKE_SOURCE_DIR}/ChatServer/message.proto"
  COMMENT "protoc tests/ChatServer/message.proto"
)

# Every ChatServer translation unit except the one holding main().
file(GLOB CHAT_SERVER_SOURCES "${CMAKE_SOURCE_DIR}/ChatServer/*.cpp")
list(REMOVE_ITEM CHAT_SERVER_SOURCES "${CMAKE_SOURCE_DIR}/ChatServer/ChatServer.cpp")

add_executable(test_chat_service
  test_chat_service.cpp
  ${CHAT_SERVER_SOURCES}
  "${CHAT_GEN_DIR}/message.pb.cc"
  "${CHAT_GEN_DIR}/message.grpc.pb.cc"
)
target_include_directories(test_chat_service PRIVATE
  "${CHAT_GEN_DIR}"
  "${CMAKE_SOURCE_DIR}/ChatServer"
  "${MYSQL_SHIM}"
  ${Protobuf_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_chat_service PRIVATE
  GTest::gtest
  GTest::gtest_main
  ${COMMON_LIBS}
)

add_test(NAME chat_service COMMAND test_chat_service)

# UrlCodec is deliberately dependency-free, so this target needs neither protobuf nor gRPC.
add_executable(test_url_codec
  test_url_codec.cpp
  "${CMAKE_SOURCE_DIR}/GateServer/UrlCodec.cpp"
)
target_include_directories(test_url_codec PRIVATE "${CMAKE_SOURCE_DIR}/GateServer")
target_link_libraries(test_url_codec PRIVATE
  GTest::gtest
  GTest::gtest_main
  Threads::Threads
)

add_test(NAME url_codec COMMAND test_url_codec)

# MsgHeader.h is a header-only boundary check; const.h pulls in the backend headers, so
# this needs the include paths but none of the service libraries at link time.
add_executable(test_msg_header test_msg_header.cpp)
target_include_directories(test_msg_header PRIVATE
  "${CMAKE_SOURCE_DIR}/ChatServer"
  "${MYSQL_SHIM}"
  ${Protobuf_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_msg_header PRIVATE
  GTest::gtest
  GTest::gtest_main
  JsonCpp::JsonCpp
  Threads::Threads
)

add_test(NAME msg_header COMMAND test_msg_header)

# MsgNode.cpp is boost/asio-only, so like msg_header this needs the include paths but no
# service library at link time.
add_executable(test_msg_node test_msg_node.cpp "${CMAKE_SOURCE_DIR}/ChatServer/MsgNode.cpp")
target_include_directories(test_msg_node PRIVATE
  "${CMAKE_SOURCE_DIR}/ChatServer"
  "${MYSQL_SHIM}"
  ${Protobuf_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_msg_node PRIVATE
  GTest::gtest
  GTest::gtest_main
  JsonCpp::JsonCpp
  Threads::Threads
)

add_test(NAME msg_node COMMAND test_msg_node)



