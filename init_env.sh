#!/bin/bash
set -euo pipefail

ENV_PATH=".env"
ISS_DEFAULT="taskplanet-api"
AUD_DEFAULT="taskplanet-web"
FORCE_SECRET="false"
OVERRIDE_ISS=""
OVERRIDE_AUD=""

usage() {
  cat <<EOF
Usage: $0 [--env <path>] [--iss <issuer>] [--aud <audience>] [--force-secret]
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --env) ENV_PATH="$2"; shift 2;;
    --iss) OVERRIDE_ISS="$2"; shift 2;;
    --aud) OVERRIDE_AUD="$2"; shift 2;;
    --force-secret) FORCE_SECRET="true"; shift;;
    -h|--help) usage; exit 0;;
    *) echo "Unknown arg: $1"; usage; exit 1;;
  esac
done

random_secret() { openssl rand -base64 64 | tr -d '\n'; }

upsert_env() {
  local key="$1" value="$2"
  if grep -qE "^[[:space:]]*${key}=" "$ENV_PATH"; then
    local esc_value; esc_value="$(printf '%s' "$value" | sed 's/[\/&]/\\&/g')"
    sed -i '' -E "s|^[[:space:]]*${key}=.*|${key}=${esc_value}|g" "$ENV_PATH"
  else
    printf '%s=%s\n' "$key" "$value" >> "$ENV_PATH"
  fi
}

get_env() {
  local key="$1"
  if [[ -f "$ENV_PATH" ]]; then
    grep -E "^[[:space:]]*${key}=" "$ENV_PATH" | tail -n 1 | cut -d= -f2- || true
  fi
}

# 在第一個符合 keys_regex 的鍵前插入 header（若尚未存在）
ensure_section() {
  local header="$1" keys_regex="$2"
  grep -qF "$header" "$ENV_PATH" && return 0
  local line_no
  line_no="$(grep -nE "^[[:space:]]*(${keys_regex})=" "$ENV_PATH" | head -n1 | cut -d: -f1 || true)"
  if [[ -n "$line_no" ]]; then
    local tmp; tmp="$(mktemp)"
    awk -v ln="$line_no" -v hdr="$header" 'NR==ln{print hdr} {print}' "$ENV_PATH" > "$tmp" && mv "$tmp" "$ENV_PATH"
  else
    # 若找不到鍵，才追加到檔尾（避免空標題）
    echo -e "\n$header" >> "$ENV_PATH"
  fi
}

[[ -f "$ENV_PATH" ]] || { echo "# Auto-created by init_env.sh" > "$ENV_PATH"; echo "" >> "$ENV_PATH"; }

# ---- 基本鍵 ----
upsert_env "DB_SCHEMA" "public"

# JWT
CUR_SECRET="$(get_env AUTH_JWT_SECRET || true)"
if [[ -z "$CUR_SECRET" || "$FORCE_SECRET" == "true" ]]; then
  upsert_env "AUTH_JWT_SECRET" "$(random_secret)"
fi
upsert_env "AUTH_ISS" "${OVERRIDE_ISS:-$(get_env AUTH_ISS || echo "$ISS_DEFAULT")}"
upsert_env "AUTH_AUD" "${OVERRIDE_AUD:-$(get_env AUTH_AUD || echo "$AUD_DEFAULT")}"

# CORS / Rate Limits（只有不存在才設預設值）
[[ -n "$(get_env ALLOW_ORIGINS || true)" ]] || upsert_env "ALLOW_ORIGINS" "*"
[[ -n "$(get_env VOTE_PER_MIN_PER_TOKEN || true)" ]] || upsert_env "VOTE_PER_MIN_PER_TOKEN" "30"
[[ -n "$(get_env VOTE_PER_MIN_PER_IP || true)" ]] || upsert_env "VOTE_PER_MIN_PER_IP" "120"
[[ -n "$(get_env SUGGEST_PER_DAY_PER_TOKEN || true)" ]] || upsert_env "SUGGEST_PER_DAY_PER_TOKEN" "20"
[[ -n "$(get_env GUEST_ISSUE_PER_MIN_PER_IP || true)" ]] || upsert_env "GUEST_ISSUE_PER_MIN_PER_IP" "10"

# ---- 插入區塊標題到正確位置 ----
ensure_section "# ==== Auth (JWT) ====" "AUTH_(JWT_SECRET|ISS|AUD)"
ensure_section "# ==== CORS ====" "ALLOW_ORIGINS"
ensure_section "# ==== Rate Limits ====" "VOTE_PER_MIN_PER_TOKEN|VOTE_PER_MIN_PER_IP|SUGGEST_PER_DAY_PER_TOKEN|GUEST_ISSUE_PER_MIN_PER_IP"

echo "✅ $ENV_PATH updated."
