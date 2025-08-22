#!/usr/bin/env bash
set -euo pipefail

# 可覆寫：BUILD_DIR, BUILD_TYPE, GENERATOR, RUN_ARGS
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
GENERATOR="${GENERATOR:-}"   # 例：-G Ninja

# 取 CPU 核心數
get_jobs() {
  if command -v nproc >/dev/null 2>&1; then
    nproc
  elif [[ "$OSTYPE" == "darwin"* ]]; then
    sysctl -n hw.ncpu
  else
    echo 4
  fi
}

# 配置 & 編譯
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" ${GENERATOR:+-G "$GENERATOR"}
cmake --build "$BUILD_DIR" -j"$(get_jobs)"

# 執行（binary 位置為 $BUILD_DIR/task_planet）
echo "[INFO] starting server: $BUILD_DIR/task_planet"
"./$BUILD_DIR/task_planet" ${RUN_ARGS:-}
