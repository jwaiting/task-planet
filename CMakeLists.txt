cmake_minimum_required(VERSION 3.10)
project(task_planet CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(nlohmann_json REQUIRED)
find_package(Threads REQUIRED)

# include & link 路徑（保留原本 /usr/local，以適配本機安裝的 pqxx/pq）
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_executable(task_planet
  src/app/server.cpp
  # 之後如果把 repo/service 拆成 .cpp 再加這裡
)

target_include_directories(task_planet PRIVATE
  include
  src
  src/app
  src/db
  src/repositories
  src/services
  src/controllers
  src/domain
  src/dto
  # src/models
  # src/utils
  # src/routes
)

target_link_libraries(task_planet PRIVATE
  pqxx
  pq
  nlohmann_json::nlohmann_json
  Threads::Threads
)

# 推薦的編譯旗標
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

target_compile_options(task_planet PRIVATE
  -Wall -Wextra -Wpedantic
  -Wno-c++20-extensions           # ← 壓掉 libpqxx 的 [[unlikely]] 警告
  # 如果你的環境有把所有警告視為錯誤，也可加：
  # -Wno-error=c++20-extensions
  -Wno-deprecated-declarations
)
