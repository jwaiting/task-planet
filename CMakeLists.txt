cmake_minimum_required(VERSION 3.10)
project(task_planet)

set(CMAKE_CXX_STANDARD 17)

# Crow is header-only, so you just need to include the path
include_directories(include)
include_directories(src/routes)
include_directories(src/models)
include_directories(src/utils)

file(GLOB ROUTES src/routes/*.hpp)
file(GLOB MODELS src/models/*.hpp)
file(GLOB UTILS src/utils/*.hpp)

add_executable(task_planet src/main.cpp ${ROUTES} ${MODELS} ${UTILS})
