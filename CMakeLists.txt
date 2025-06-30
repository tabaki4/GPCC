cmake_minimum_required(VERSION 3.14)
project(GP)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_BUILD_TYPE Release)

add_subdirectory(${CMAKE_SOURCE_DIR}/logic)
add_subdirectory(${CMAKE_SOURCE_DIR}/gpcc)
add_subdirectory(${CMAKE_SOURCE_DIR}/sim_builder)

add_executable(${PROJECT_NAME} test.cpp)

#target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/logic/build)
#target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/gpcc/build)
#target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/sim_builder/build)
target_link_libraries(${PROJECT_NAME} logic gpcc sim_builder)