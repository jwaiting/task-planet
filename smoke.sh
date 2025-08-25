#!/usr/bin/env bash
set -euo pipefail

# 讀取 .env 裡的 PORT（若有）
PORT_FROM_ENV=""
if [[ -f ".env" ]]; then
  PORT_FROM_ENV="$(grep -E '^[[:space:]]*PORT=' .env | tail -n 1 | cut -d= -f2- | tr -d '[:space:]' || true)"
fi

# 允許外部傳 base；否則用 .env 的 PORT；最後預設 8080
DEFAULT_BASE="http://localhost:${PORT_FROM_ENV:-8080}"
base="${1:-$DEFAULT_BASE}"

have_python_pp() { command -v python3 >/dev/null 2>&1; }
pp() { if have_python_pp; then python3 -m json.tool || true; else cat; fi; }

curl_json() { curl -sS -H 'Content-Type: application/json' "$@"; }

# 回傳 "BODY<SEP>CODE"（SEP 是最後一行前的 \n）
req_with_code() {
  local method="$1"; shift
  local url="$1"; shift
  curl -sS -o - -w $'\n%{http_code}' -X "$method" "$url" "$@"
}

echo "=== Smoke against: $base ==="

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
curl_json -X POST "$base/api/suggest" \
  -d '{"tagCodes":["context/desk","focus/high"],"time":20,"limit":5}' | pp; echo

# 4) /api/suggestions/buffer
echo '[POST] /api/suggestions/buffer'
curl_json -X POST "$base/api/suggestions/buffer" \
  -d '{"description":"散步 10 分鐘","suggestedTime":10,"tagCodes":["context/outdoor","energy/med"]}' | pp; echo

# 5) JWT 測試：嘗試 dev-login 取 token（Debug+DEV_LOGIN=1 時才會存在）
echo '[POST] /api/auth/dev-login (optional)'
dev_login_resp="$(req_with_code POST "$base/api/auth/dev-login")" || true
dev_login_body="${dev_login_resp%$'\n'*}"
dev_login_code="${dev_login_resp##*$'\n'}"
echo "dev-login http_code=$dev_login_code"
if [[ "$dev_login_code" == "200" ]]; then
  # 解析 token（優先 jq，沒有就用 sed）
  if command -v jq >/dev/null 2>&1; then
    TOKEN="$(printf '%s' "$dev_login_body" | jq -r .token 2>/dev/null || true)"
  else
    TOKEN="$(printf '%s' "$dev_login_body" | sed -n 's/.*"token"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')"
  fi
  if [[ -z "${TOKEN:-}" || "${TOKEN:-null}" == "null" ]]; then
    echo "❌ dev-login returned 200 but token is empty"
    exit 1
  fi
  echo "✅ got token"

  echo '[POST] /api/events (with token, expect 200)'
  events_ok="$(req_with_code POST "$base/api/events" \
      -H "Authorization: Bearer $TOKEN" \
      -H 'Content-Type: application/json' \
      -d '{"taskId":1,"event":"adopt","tagCodes":["context/desk","focus/high"]}')" || true
  events_ok_body="${events_ok%$'\n'*}"
  events_ok_code="${events_ok##*$'\n'}"
  echo "code=$events_ok_code"
  echo "$events_ok_body"
  echo
  if [[ "$events_ok_code" != "200" ]]; then
    echo "❌ /api/events expected 200 with token, got $events_ok_code"
    exit 1
  fi
else
  echo "dev-login not available (this is expected in Release)."
  echo '[POST] /api/events (without token, expect 401/403)'
  events_noauth="$(req_with_code POST "$base/api/events" \
      -H 'Content-Type: application/json' \
      -d '{"taskId":1,"event":"adopt","tagCodes":["context/desk","focus/high"]}')" || true
  events_noauth_body="${events_noauth%$'\n'*}"
  events_noauth_code="${events_noauth##*$'\n'}"
  echo "code=$events_noauth_code"
  echo "$events_noauth_body"
  echo
  case "$events_noauth_code" in
    401|403) echo "✅ /api/events correctly rejected without token";;
    *) echo "❌ /api/events should be 401/403 without token, got $events_noauth_code"; exit 1;;
  esac
fi

echo "✅ Smoke finished against $base"
