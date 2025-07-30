cmake_minimum_required(VERSION 3.10)
project(task_planet)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# include paths
include_directories(include)
include_directories(src/models)
include_directories(src/utils)
include_directories(src/routes)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

# nlohmann-json
find_package(nlohmann_json REQUIRED)

# source files
file(GLOB ROUTES src/routes/*.hpp)
file(GLOB MODELS src/models/*.hpp)
file(GLOB UTILS src/utils/*.hpp)

add_executable(task_planet src/main.cpp ${ROUTES} ${MODELS} ${UTILS})

target_link_libraries(task_planet
  pqxx
  pq
  nlohmann_json::nlohmann_json
)
