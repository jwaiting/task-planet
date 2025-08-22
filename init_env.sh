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

Options:
  --env <path>        Path to .env file (default: ./.env)
  --iss <issuer>      AUTH_ISS value (default: ${ISS_DEFAULT})
  --aud <audience>    AUTH_AUD value (default: ${AUD_DEFAULT})
  --force-secret      Regenerate AUTH_JWT_SECRET even if it exists
EOF
}

# --- parse args ---
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

# --- helpers ---
random_secret() {
  if ! command -v openssl >/dev/null 2>&1; then
    echo "ERROR: openssl not found. Please install it first." >&2
    exit 1
  fi
  # 64 bytes, base64; remove newlines just in case
  openssl rand -base64 64 | tr -d '\n'
}

# update or insert a KEY=VALUE (preserve other content & comments)
upsert_env() {
  local key="$1"
  local value="$2"
  if grep -qE "^[[:space:]]*${key}=" "$ENV_PATH"; then
    # Replace the whole line (key=existing) with key=new_value
    # Use sed -i compatible with BSD (macOS)
    local esc_value
    esc_value="$(printf '%s' "$value" | sed 's/[\/&]/\\&/g')"
    sed -i '' -E "s|^[[:space:]]*${key}=.*|${key}=${esc_value}|g" "$ENV_PATH"
  else
    printf '%s=%s\n' "$key" "$value" >> "$ENV_PATH"
  fi
}

# get current value (empty string if missing)
get_env() {
  local key="$1"
  if [[ -f "$ENV_PATH" ]]; then
    # take last assignment if multiple
    grep -E "^[[:space:]]*${key}=" "$ENV_PATH" | tail -n 1 | cut -d= -f2- || true
  fi
}

# --- ensure file exists ---
if [[ ! -f "$ENV_PATH" ]]; then
  echo "# Auto-created by init_env.sh" > "$ENV_PATH"
  echo "" >> "$ENV_PATH"
fi

# --- JWT SECRET ---
CUR_SECRET="$(get_env AUTH_JWT_SECRET || true)"
if [[ -z "$CUR_SECRET" || "$FORCE_SECRET" == "true" ]]; then
  NEW_SECRET="$(random_secret)"
  upsert_env "AUTH_JWT_SECRET" "$NEW_SECRET"
  SECRET_MSG="AUTH_JWT_SECRET set $( [[ "$FORCE_SECRET" == "true" ]] && echo '(forced)' || echo '(new)' )"
else
  SECRET_MSG="AUTH_JWT_SECRET kept (existing)"
fi

# --- ISS ---
CUR_ISS="$(get_env AUTH_ISS || true)"
TARGET_ISS="${OVERRIDE_ISS:-${CUR_ISS:-$ISS_DEFAULT}}"
upsert_env "AUTH_ISS" "$TARGET_ISS"

# --- AUD ---
CUR_AUD="$(get_env AUTH_AUD || true)"
TARGET_AUD="${OVERRIDE_AUD:-${CUR_AUD:-$AUD_DEFAULT}}"
upsert_env "AUTH_AUD" "$TARGET_AUD"

# --- nice section header (ensure present once) ---
if ! grep -q "^# ==== Auth (JWT) ====" "$ENV_PATH"; then
  # Insert header before the first AUTH_* line, or append if not found
  if grep -nE "^[[:space:]]*AUTH_(JWT_SECRET|ISS|AUD)=" "$ENV_PATH" >/dev/null; then
    line_no="$(grep -nE "^[[:space:]]*AUTH_(JWT_SECRET|ISS|AUD)=" "$ENV_PATH" | head -n1 | cut -d: -f1)"
    tmp="$(mktemp)"
    awk -v ln="$line_no" 'NR==ln{print "# ==== Auth (JWT) ===="} {print}' "$ENV_PATH" > "$tmp"
    mv "$tmp" "$ENV_PATH"
  else
    echo -e "\n# ==== Auth (JWT) ====" >> "$ENV_PATH"
  fi
fi

echo "✅ Updated $ENV_PATH"
echo "   - $SECRET_MSG"
echo "   - AUTH_ISS = $TARGET_ISS"
echo "   - AUTH_AUD = $TARGET_AUD"

# --- Git ignore hint ---
if [[ ! -f ".gitignore" ]] || ! grep -qE '(^|/)\.env$' .gitignore; then
  echo "⚠️  Reminder: add '.env' to .gitignore to avoid committing secrets."
fi
