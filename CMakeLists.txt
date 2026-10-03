cmake_minimum_required(VERSION 3.16)
project(WeChatBackend CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(HIREDIS REQUIRED hiredis)

find_library(MYSQLCPPCONN mysqlcppconn REQUIRED)
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

# 源码里用 Windows/VS 的 <jdbc/xxx.h> 布局；在构建目录生成兼容 shim，
# 指向本机 MySQL Connector/C++ 的真实头文件，避免改动源码。
find_path(MYSQLCPPCONN_INC mysql_driver.h PATHS /usr/include /usr/local/include REQUIRED)
set(MYSQL_SHIM "${CMAKE_BINARY_DIR}/mysql_shim")
file(MAKE_DIRECTORY "${MYSQL_SHIM}/jdbc")
foreach(h mysql_driver.h mysql_connection.h mysql_error.h)
  if(EXISTS "${MYSQLCPPCONN_INC}/${h}")
    file(CREATE_LINK "${MYSQLCPPCONN_INC}/${h}" "${MYSQL_SHIM}/jdbc/${h}" SYMBOLIC)
  endif()
endforeach()
if(EXISTS "${MYSQLCPPCONN_INC}/cppconn")
  file(CREATE_LINK "${MYSQLCPPCONN_INC}/cppconn" "${MYSQL_SHIM}/jdbc/cppconn" SYMBOLIC)
endif()

set(COMMON_LIBS
  gRPC::grpc++
  ${Protobuf_LIBRARIES}
  JsonCpp::JsonCpp
  ${MYSQLCPPCONN}
  Boost::system
  Boost::filesystem
  ${HIREDIS_LIBRARIES}
  Threads::Threads
)

# 三个 C++ 后端服务各自独立可执行；proto 在构建期用本机 protoc 重新生成
foreach(svc GateServer StatusServer ChatServer)
  set(GEN_DIR "${CMAKE_BINARY_DIR}/gen/${svc}")
  file(MAKE_DIRECTORY "${GEN_DIR}")
  set(PROTO_FILE "${CMAKE_SOURCE_DIR}/${svc}/message.proto")

  add_custom_command(
    OUTPUT "${GEN_DIR}/message.pb.cc" "${GEN_DIR}/message.pb.h"
           "${GEN_DIR}/message.grpc.pb.cc" "${GEN_DIR}/message.grpc.pb.h"
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
            --proto_path="${CMAKE_SOURCE_DIR}/${svc}"
            --cpp_out="${GEN_DIR}"
            --grpc_out="${GEN_DIR}"
            --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
            "${PROTO_FILE}"
    DEPENDS "${PROTO_FILE}"
    COMMENT "protoc ${svc}/message.proto"
  )

  file(GLOB SVC_SOURCES "${CMAKE_SOURCE_DIR}/${svc}/*.cpp")

  add_executable(${svc}
    ${SVC_SOURCES}
    "${GEN_DIR}/message.pb.cc"
    "${GEN_DIR}/message.grpc.pb.cc"
  )
  target_include_directories(${svc} PRIVATE
    "${GEN_DIR}"
    "${CMAKE_SOURCE_DIR}/${svc}"
    "${MYSQL_SHIM}"
    ${Protobuf_INCLUDE_DIRS}
    ${HIREDIS_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS}
  )
  target_link_libraries(${svc} PRIVATE ${COMMON_LIBS})
endforeach()
