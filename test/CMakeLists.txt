cmake_minimum_required(VERSION 3.14)
project(logic_test)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(${PROJECT_NAME} logic_test.cpp)

target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../logic/cpp/build)
target_link_libraries(${PROJECT_NAME} GTest::gtest_main logic)


include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
