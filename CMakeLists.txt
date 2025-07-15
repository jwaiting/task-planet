cmake_minimum_required(VERSION 3.10)
project(task_planet)

set(CMAKE_CXX_STANDARD 17)

# Crow is header-only, so you just need to include the path
include_directories(include)

add_executable(task_planet src/main.cpp)
