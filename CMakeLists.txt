cmake_minimum_required(VERSION 3.10)
project(task_planet LANGUAGES CXX)

# ---- C++ standard ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Build type default ----
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- Dev login flag (default OFF) ----
option(TP_ENABLE_DEV_LOGIN "Enable /api/auth/dev-login endpoint" OFF)

# 若開了 dev-login 卻還是 Release，直接擋下（雙保險，程式碼內也有 #error）
if(TP_ENABLE_DEV_LOGIN AND CMAKE_BUILD_TYPE MATCHES "^[Rr]elease$")
  message(FATAL_ERROR "TP_ENABLE_DEV_LOGIN must NOT be enabled in Release builds.")
endif()

# ---- Dependencies ----
find_package(Threads REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)   # brew 提供的 cmake config
find_package(OpenSSL QUIET)                   # optional（未來 JWT）
find_package(jwt-cpp CONFIG QUIET)            # optional（未來 JWT）

# If jwt-cpp not found via package manager, fetch it (header-only)
if(NOT TARGET jwt-cpp::jwt-cpp)
  message(STATUS "jwt-cpp not found via find_package; fetching from upstream...")
  include(FetchContent)
  FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.0
  )
  FetchContent_MakeAvailable(jwt-cpp)
endif()

# ---- Homebrew / local include & lib hints ----
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
  jwt-cpp::jwt-cpp            # <- 一律連結（確保 include path 有效）
)

if(OpenSSL_FOUND)
  target_link_libraries(task_planet PRIVATE OpenSSL::SSL OpenSSL::Crypto)
endif()

# ---- Compile options ----
target_compile_options(task_planet PRIVATE
  -Wall -Wextra -Wpedantic
  -Wno-c++20-extensions
  -Wno-deprecated-declarations
)

# ---- Dev login compile definition ----
if(TP_ENABLE_DEV_LOGIN)
  target_compile_definitions(task_planet PRIVATE TP_ENABLE_DEV_LOGIN=1)
  message(STATUS ">>> Dev login route ENABLED (TP_ENABLE_DEV_LOGIN=ON)")
else()
  message(STATUS ">>> Dev login route DISABLED (TP_ENABLE_DEV_LOGIN=OFF)")
endif()

# ---- Runtime search path (macOS 常見動態庫位置) ----
if(APPLE)
  set_target_properties(task_planet PROPERTIES
    BUILD_RPATH "/opt/homebrew/lib;/usr/local/lib"
    INSTALL_RPATH "/opt/homebrew/lib;/usr/local/lib"
  )
endif()
