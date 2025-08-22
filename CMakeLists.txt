cmake_minimum_required(VERSION 3.10)
project(task_planet LANGUAGES CXX)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Build type default ----
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Dependencies ----
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)   # brew 提供的 cmake config
find_package(OpenSSL QUIET)                   # optional（未來 JWT）
find_package(jwt-cpp CONFIG QUIET)            # optional（未來 JWT）

# ---- Homebrew / local include & lib hints ----
# 支援 Apple Silicon (/opt/homebrew) 與 Intel (/usr/local)
foreach(HINT_DIR /opt/homebrew /usr/local)
  if(EXISTS "${HINT_DIR}/include")
    include_directories("${HINT_DIR}/include")
  endif()
  if(EXISTS "${HINT_DIR}/lib")
    link_directories("${HINT_DIR}/lib")
  endif()
endforeach()

# ---- Sources ----
add_executable(task_planet
  src/main.cpp
  src/app/server.cpp
  # TODO: 未來有新的 .cpp 再加
)

# ---- Include paths for our code ----
target_include_directories(task_planet PRIVATE
  include
  src
  src/app
  src/config
  src/db
  src/repositories
  src/services
  src/controllers
  src/domain
  src/dto
)

# ---- Link libraries ----
target_link_libraries(task_planet PRIVATE
  pqxx
  pq
  nlohmann_json::nlohmann_json
  Threads::Threads
)

if(OpenSSL_FOUND)
  target_link_libraries(task_planet PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

if(TARGET jwt-cpp::jwt-cpp)
  target_link_libraries(task_planet PRIVATE jwt-cpp::jwt-cpp)
endif()

# ---- Compile options ----
target_compile_options(task_planet PRIVATE
  -Wall -Wextra -Wpedantic
  -Wno-c++20-extensions
  -Wno-deprecated-declarations
)

# ---- Runtime search path (macOS 常見動態庫位置) ----
if(APPLE)
  # 讓可執行檔在執行時能在這些路徑找到 dylib
  set_target_properties(task_planet PROPERTIES
    BUILD_RPATH "/opt/homebrew/lib;/usr/local/lib"
    INSTALL_RPATH "/opt/homebrew/lib;/usr/local/lib"
  )
endif()
