cmake_minimum_required(VERSION 3.14)
project(logic)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} STATIC logic.cpp)
