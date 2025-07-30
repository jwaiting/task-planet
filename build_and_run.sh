#!/bin/bash
set -e  # 有錯誤就中斷

# 1. 建立或清空 build 目錄
if [ ! -d "build" ]; then
  mkdir build
fi

cd build

# 2. 執行 CMake 與 Make
cmake ..
make -j$(nproc)

# 3. 回到專案根目錄並執行
cd ..
./build/task_planet
