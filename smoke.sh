#!/usr/bin/env bash
set -euo pipefail
base=${1:-http://localhost:8080}

echo '[GET] /ping'
curl -sS "$base/ping"; echo; echo

echo '[GET] /api/tags'
curl -sS "$base/api/tags" | python3 -m json.tool || true; echo

echo '[POST] /api/suggest'
curl -sS -X POST "$base/api/suggest" -H 'Content-Type: application/json' \
  -d '{"tagCodes":["context/desk","focus/high"],"time":20,"limit":5}' | python3 -m json.tool || true; echo

echo '[POST] /api/suggestions/buffer'
curl -sS -X POST "$base/api/suggestions/buffer" -H 'Content-Type: application/json' \
  -d '{"description":"散步 10 分鐘","suggestedTime":10,"tagCodes":["context/outdoor","energy/med"]}' \
  | python3 -m json.tool || true; echo

echo '[POST] /api/events adopt'
curl -sS -X POST "$base/api/events" -H 'Content-Type: application/json' \
  -d '{"taskId":1,"event":"adopt","tagCodes":["context/desk","focus/high"]}'; echo
