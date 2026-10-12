# Header-only helpers (JSON field validation, user-info cache codec, send length limit).
add_executable(test_chat_util test_chat_util.cpp)
target_include_directories(test_chat_util PRIVATE
  "${CMAKE_SOURCE_DIR}/ChatServer"
  "${MYSQL_SHIM}"
  ${Protobuf_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_chat_util PRIVATE
  GTest::gtest
  GTest::gtest_main
  JsonCpp::JsonCpp
  Threads::Threads
)
add_test(NAME chat_util COMMAND test_chat_util)

# RedisConPool is header-only but calls into hiredis; it is exercised against a refused port.
add_executable(test_chat_redis_pool test_chat_redis_pool.cpp)
target_include_directories(test_chat_redis_pool PRIVATE
  "${CMAKE_SOURCE_DIR}/ChatServer"
  "${MYSQL_SHIM}"
  ${Protobuf_INCLUDE_DIRS}
  ${HIREDIS_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
)
target_link_libraries(test_chat_redis_pool PRIVATE
  GTest::gtest
  GTest::gtest_main
  ${HIREDIS_LIBRARIES}
  Threads::Threads
)
add_test(NAME chat_redis_pool COMMAND test_chat_redis_pool)
