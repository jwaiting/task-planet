#!/usr/bin/env bash
set -euo pipefail

# 可覆寫：BUILD_DIR, BUILD_TYPE, GENERATOR, RUN_ARGS, DEV_LOGIN
BUILD_DIR="${BUILD_DIR:-build}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
GENERATOR="${GENERATOR:-}"   # 例：-G Ninja
DEV_LOGIN="${DEV_LOGIN:-0}"  # 1/true/ON 才會啟用 /api/auth/dev-login

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

# 轉換 DEV_LOGIN 為 CMake 參數
TP_FLAG="OFF"
case "$DEV_LOGIN" in
  1|true|TRUE|on|ON|yes|YES) TP_FLAG="ON" ;;
esac

# 安全提示
if [[ "${TP_FLAG}" == "ON" && "${BUILD_TYPE}" =~ ^[Rr]elease$ ]]; then
  echo "ERROR: DEV_LOGIN requested but BUILD_TYPE=Release. Refusing to configure."
  exit 1
fi

echo "[INFO] CMAKE_BUILD_TYPE=${BUILD_TYPE}, TP_ENABLE_DEV_LOGIN=${TP_FLAG}"

# 配置 & 編譯
cmake -S . -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DTP_ENABLE_DEV_LOGIN="${TP_FLAG}" \
  ${GENERATOR:+-G "$GENERATOR"}

cmake --build "$BUILD_DIR" -j"$(get_jobs)"

# 執行（binary 位置為 $BUILD_DIR/task_planet）
echo "[INFO] starting server: $BUILD_DIR/task_planet"
"./$BUILD_DIR/task_planet" ${RUN_ARGS:-}
