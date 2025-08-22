#!/usr/bin/env bash
set -euo pipefail

# 讀取 .env 裡的 PORT（若有）
PORT_FROM_ENV=""
if [[ -f ".env" ]]; then
  # 取最後一次賦值，忽略註解；支援形如 PORT=8080
  PORT_FROM_ENV="$(grep -E '^[[:space:]]*PORT=' .env | tail -n 1 | cut -d= -f2- | tr -d '[:space:]' || true)"
fi

# 允許外部傳 base；否則用 .env 的 PORT；最後預設 8080
DEFAULT_BASE="http://localhost:${PORT_FROM_ENV:-8080}"
base="${1:-$DEFAULT_BASE}"

have_python_pp() {
  command -v python3 >/dev/null 2>&1
}

pp() {
  if have_python_pp; then
    python3 -m json.tool || true
  else
    cat
  fi
}

req() {
  local method="$1"; shift
  local url="$1"; shift
  echo "[$method] $url"
  if [[ "$method" == "GET" ]]; then
    curl -sS -w "\n" "$url"
  else
    curl -sS -w "\n" -X "$method" "$url" "$@"
  fi
  echo
}

# ---- 開始測試 ----

# 1) /ping
resp="$(curl -sS -o - -w "%{http_code}" "$base/ping")"
body="${resp%[0-9][0-9][0-9]}"
code="${resp:${#body}}"
echo "[GET] /ping => $code"
echo "$body"
echo
if [[ "$code" != "200" ]] || [[ "$body" != "pong" ]]; then
  echo "❌ /ping failed (code=$code, body=$body)"
  exit 1
fi

# 2) /api/tags
echo '[GET] /api/tags'
curl -sS "$base/api/tags" | pp; echo

# 3) /api/suggest
echo '[POST] /api/suggest'
curl -sS -X POST "$base/api/suggest" -H 'Content-Type: application/json' \
  -d '{"tagCodes":["context/desk","focus/high"],"time":20,"limit":5}' | pp; echo

# 4) /api/suggestions/buffer
echo '[POST] /api/suggestions/buffer'
curl -sS -X POST "$base/api/suggestions/buffer" -H 'Content-Type: application/json' \
  -d '{"description":"散步 10 分鐘","suggestedTime":10,"tagCodes":["context/outdoor","energy/med"]}' | pp; echo

# 5) /api/events adopt
echo '[POST] /api/events adopt'
curl -sS -X POST "$base/api/events" -H 'Content-Type: application/json' \
  -d '{"taskId":1,"event":"adopt","tagCodes":["context/desk","focus/high"]}'; echo

echo "✅ Smoke finished against $base"
