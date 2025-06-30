cmake_minimum_required(VERSION 3.14)
project(sim_builder)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC builder.cpp)

#target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/..z/gpcc/build)
#target_link_libraries(${PROJECT_NAME} gpcc)
