# TaskPlanet Backend

Crow (C++) backend for TaskPlanet: mood/context-driven task suggestions with a suggestion-buffer and lightweight learning.

---

## Overview

* **Stack**: C++17, [Crow](https://github.com/CrowCpp/Crow) (HTTP), libpqxx (PostgreSQL), CMake
* **DB**: PostgreSQL (snake\_case schema), `pg_trgm` extension, Prisma for migrations
* **Config**: `.env` via dotenv-cpp
* **Key features**:

  * `/api/tags` – fetch active tag catalog
  * `/api/suggest` – recommend tasks based on tags + time
  * `/api/suggestions/buffer` – submit user suggestions (auto-merge if similar)
  * `/api/events` – feedback (`adopt`/`skip`/`impression`) to update weights
  * CORS enabled; unified JSON error shape `{ error, hint }`

---

## Prerequisites

* **PostgreSQL** 13+ (with `pg_trgm` extension available)
* **CMake** 3.10+
* **Compiler**: clang++/g++ with C++17
* **libpq / libpqxx** installed and discoverable (e.g., `/usr/local/include`, `/usr/local/lib`)

> Note: Prisma is used only to manage migrations. The C++ service connects directly to Postgres via libpq DSN.

---

## Environment variables (`.env`)

```
DB_NAME=taskplanet_tagfit
DB_USER=taskuser
DB_PASSWORD=taskpass
DB_HOST=localhost
DB_PORT=5432
DB_SCHEMA=public   # optional, defaults to public
PORT=8080          # optional, defaults to 8080
```

> The backend **does not** use Prisma-style URLs with `?schema=`. Schema is set via `DB_SCHEMA` and applied as `SET search_path TO <schema>, public` per connection.

---

## Install & Run

### Quick start

```bash
# Build & run (Release by default)
./build_and_run.sh

# In another terminal: quick smoke tests
./smoke.sh
```

### Manual build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/task_planet
```

---

## Database & Migrations

* Enable `pg_trgm` and create supporting indexes/triggers via Prisma migration:

  * `migrations/<ts>_infra_trgm_indexes_triggers/migration.sql`
* Apply:

```bash
npx prisma migrate dev
```

* If `CREATE EXTENSION pg_trgm` needs superuser, run once with a superuser:

```bash
psql -h localhost -U <superuser> -d taskplanet_tagfit -c 'CREATE EXTENSION IF NOT EXISTS pg_trgm;'
```

---

## API

Base URL: `http://localhost:${PORT:-8080}`

### Health

`GET /ping` → `pong`

### Tags

`GET /api/tags`

```json
{
  "tags": [
    {"id": 1, "code": "context/desk", "label": "在桌前", "group": "context"},
    ...
  ]
}
```

### Suggest tasks

`POST /api/suggest`

```json
{
  "tagCodes": ["context/desk", "focus/high"],
  "time": 20,
  "limit": 10
}
```

Response:

```json
{
  "tasks": [
    {
      "id": 12,
      "description": "寫下三件感恩小事",
      "suggestedTime": 10,
      "tagFit": 0.73,
      "timeFit": 0.86,
      "finalScore": 0.78
    }
  ]
}
```

> Also accepts `tags: number[]` (tag IDs). `excludeTaskIds` is **planned** but not implemented yet.

### Submit suggestion

`POST /api/suggestions/buffer`

```json
{
  "description": "散步 10 分鐘",
  "suggestedTime": 10,
  "tagCodes": ["context/outdoor", "energy/med"]
}
```

Response (merge hit):

```json
{ "merged": true, "suggestionId": 44, "matchedTaskId": 5, "similarity": 0.91 }
```

Response (queued):

```json
{ "merged": false, "suggestionId": 45 }
```

### Feedback events

`POST /api/events`

```json
{ "taskId": 5, "event": "adopt", "tagCodes": ["context/desk", "focus/high"] }
```

* `adopt`: reinforce `(task, tag)` (alpha += 1)
* `skip`/`impression`: light negative signal (beta += 1)

Errors:

* JSON error envelope: `{ "error": "...", "hint": "..." }`

---

## Development

* **Prepared statements** are registered once per pooled connection (in pool initializer). **Do not** re-register in controllers.
* **SQL style**: snake\_case tables and columns throughout.
* **CORS** is enabled via middleware.

### Scripts

* `build_and_run.sh` – configurable via `BUILD_DIR`, `BUILD_TYPE`, `GENERATOR`
* `smoke.sh` – basic end-to-end API checks

---

## Project structure

```
src/
  app/
    server.cpp            # entrypoint
    middleware.hpp        # CORS
    routes.hpp            # (optional) central route mounting
  config/
    config.hpp            # dotenv + env access + DB DSN + schema + port
  db/
    pool.hpp              # pqxx connection pool
    prepared.hpp          # prepared SQL (snake_case)
  repositories/
    task_repo.hpp
    suggestion_repo.hpp
    weight_repo.hpp
    tag_repo.hpp
    stats_repo.hpp        # (todo)
  services/
    recommend_service.hpp
    suggestion_service.hpp
    event_service.hpp
  controllers/
    suggest_controller.hpp
    suggestions_controller.hpp
    events_controller.hpp
    tags_controller.hpp
    admin_controller.hpp  # (todo)
  domain/
    task.hpp              # (todo) domain structs
    types.hpp             # (todo) enums/aliases
  dto/
    request.hpp           # (todo) inbound shape helpers
    response.hpp          # (todo) error helpers
```

---

## Troubleshooting

* **`invalid URI query parameter: "schema"`**: Don’t use Prisma-style URLs. Use DSN via `.env` (DB\_\* vars). Service sets `search_path` from `DB_SCHEMA`.
* **`function similarity(text, text) does not exist`**: Install `pg_trgm` in the database.
* **`prepared statement "..." already exists`**: Ensure `register_prepared` is only called from the pool initializer (not in controllers).
* **`syntax error near "{"` with ANY(...)**: Pass array via parameterized query (`$1::text[]` / `$1::int[]`).

---

## Roadmap (short)

* `/api/suggest`: support `excludeTaskIds`
* `/api/suggestions/buffer`: return `matchedTask` fields on merge
* Admin endpoints for reviewing the buffer
* Daily maintenance job: decay `alpha/beta`; refresh `task_stats`

---

## License

MIT (see `LICENSE`).

---

## Local development – quick commands

```bash
# Build & run (Release) – defaults are configurable via env vars
./build_and_run.sh

# Build & run (Debug)
BUILD_TYPE=Debug ./build_and_run.sh

# Use Ninja instead of Make
GENERATOR=Ninja ./build_and_run.sh

# Smoke tests (can pass base URL)
./smoke.sh            # default http://localhost:8080
./smoke.sh http://127.0.0.1:9090
```

If you don’t have `jq`, use Python to pretty-print:

```bash
curl -sS <url> | python3 -m json.tool
```

---

## API field reference

### `GET /api/tags`

**Response**

* `tags[]` – array of objects

  * `id` (number)
  * `code` (string, e.g., `context/desk`)
  * `label` (string, localized)
  * `group` (string)

### `POST /api/suggest`

**Request body**

* `tagCodes[]` *(string, optional)* – tag codes; resolved to IDs server-side
* `tags[]` *(number, optional)* – tag IDs; alternative to `tagCodes`
* `time` *(number, required)* – minutes the user has
* `limit` *(number, optional, default 20)*

**Response**

* `tasks[]`

  * `id` (number)
  * `description` (string)
  * `suggestedTime` (number)
  * `tagFit` (float, 0..1)
  * `timeFit` (float, 0..1)
  * `finalScore` (float, 0..1)

> Planned: `excludeTaskIds: number[]` to avoid repeats.

### `POST /api/suggestions/buffer`

**Request body**

* `description` *(string, required)*
* `suggestedTime` *(number, required)*
* `tagCodes[]` *(string, optional)* or `tags[]` *(number, optional)*

**Response**

* If merged (similar task exists): `{ merged: true, suggestionId, matchedTaskId, similarity }`
* If queued: `{ merged: false, suggestionId }`

> Planned: include `matchedTask: { id, description }` when merged.

### `POST /api/events`

**Request body**

* `taskId` *(number, required)*
* `event` *(string, required)* – one of `adopt`, `skip`, `impression`
* `tagCodes[]` *(string, optional)* or `tags[]` *(number, optional)*

**Semantics**

* `adopt` → reinforce `(task, tag)` (alpha += 1)
* `skip`/`impression` → light negative signal (beta += 1)

**Response**

* `"ok"`

### Error envelope

* `{ "error": string, "hint": string }`

---

## Example requests

```bash
# Tags
curl -sS http://localhost:8080/api/tags | python3 -m json.tool

# Suggest with tag codes
curl -sS -X POST http://localhost:8080/api/suggest \
  -H 'Content-Type: application/json' \
  -d '{"tagCodes":["context/desk","focus/high"],"time":20,"limit":5}' \
  | python3 -m json.tool

# Submit suggestion
curl -sS -X POST http://localhost:8080/api/suggestions/buffer \
  -H 'Content-Type: application/json' \
  -d '{"description":"散步 10 分鐘","suggestedTime":10,"tagCodes":["context/outdoor","energy/med"]}' \
  | python3 -m json.tool

# Adopt event
curl -sS -X POST http://localhost:8080/api/events \
  -H 'Content-Type: application/json' \
  -d '{"taskId":1,"event":"adopt","tagCodes":["context/desk","focus/high"]}'
```

---

## Admin (planned)

* `GET /admin/suggestions?status=pending&limit=50` – list suggestions with votes & best match
* `POST /admin/suggestions/{id}/approve` – create task, copy weights, mark approved
* `POST /admin/suggestions/{id}/merge` – merge into task `{ taskId }`, mark merged
* `POST /admin/suggestions/{id}/reject` – mark rejected

> Protect admin endpoints with `ADMIN_TOKEN` header. Keep in a `.env` var; check in middleware.

---

## Deployment notes

* Run the service behind a reverse proxy (nginx/caddy) for TLS and routing.
* Set env vars via your process manager (systemd, Docker, or container runtime).
* Ensure database has `pg_trgm` installed once by a privileged role.

**systemd unit (example)**

```ini
[Unit]
Description=TaskPlanet Backend
After=network.target

[Service]
Environment=DB_NAME=taskplanet_tagfit
Environment=DB_USER=taskuser
Environment=DB_PASSWORD=taskpass
Environment=DB_HOST=127.0.0.1
Environment=DB_PORT=5432
Environment=DB_SCHEMA=public
Environment=PORT=8080
WorkingDirectory=/opt/taskplanet
ExecStart=/opt/taskplanet/build/task_planet
Restart=on-failure

[Install]
WantedBy=multi-user.target
```

---

## Maintenance job (daily)

Use cron or a runner to decay alpha/beta and refresh `task_stats`.

```bash
#!/usr/bin/env bash
set -euo pipefail
psql "host=$DB_HOST dbname=$DB_NAME user=$DB_USER password=$DB_PASSWORD" <<'SQL'
-- Decay
UPDATE task_tag_weight
SET alpha = alpha * 0.995,
    beta  = beta  * 0.995,
    updated_at = now();

-- Refresh quality/popularity (simple baseline)
UPDATE task_stats ts
SET score_quality = COALESCE(tt.alpha / NULLIF(tt.alpha + tt.beta,0), 0),
    score_popularity = COALESCE(score_popularity, 0),
    updated_at = now()
FROM (
  SELECT task_id, SUM(alpha) AS alpha, SUM(beta) AS beta
  FROM task_tag_weight
  GROUP BY task_id
) tt
WHERE ts.task_id = tt.task_id;
SQL
```

Crontab (runs at 03:15 daily):

```
15 3 * * * DB_HOST=localhost DB_NAME=taskplanet_tagfit DB_USER=taskuser DB_PASSWORD=taskpass \
  /opt/taskplanet/scripts/maintain.sh >> /var/log/taskplanet/maintain.log 2>&1
```

---

## Security

* Limit request body size in controllers for POST endpoints (e.g., reject >64KB).
* Validate required fields; return JSON error envelope.
* CORS: in production, narrow `Access-Control-Allow-Origin` to your front-end domain(s).
* Rate-limiting (planned): simple IP/token-based limits on write endpoints.

---

## Performance tips

* `pg_trgm` GIN indexes are in place; keep `ANALYZE`/`VACUUM` running as per Postgres defaults.
* Consider raising `work_mem` for complex queries if needed.
* Batch writes (e.g., tag weight updates) within a single transaction (already done).

---

## Conventions

* **Commit messages**: Conventional Commits (e.g., `feat:`, `fix:`, `chore:`) in **English**.
* **Code style**: header-only repos/services okay for now; split into `.cpp` when compile times grow.
* **SQL**: always parameterize dynamic values; prefer `$1::int[]`/`$1::text[]` for arrays.

---

## Troubleshooting (expanded)

* `invalid URI query parameter: "schema"` → do not use Prisma-style URL; set `DB_SCHEMA` + DSN.
* `function similarity(text, text) does not exist` → run `CREATE EXTENSION IF NOT EXISTS pg_trgm;` in the DB.
* `prepared statement "..." already exists` → ensure we only register in the pool initializer.
* `syntax error near "{"` in `ANY({...})` → pass arrays as params (`$1::text[]`).
* `permission denied for schema public` → check DB user grants; apply migrations with a role that owns the schema.

---

## Roadmap (near-term)

* `/api/suggest`: add `excludeTaskIds` to reduce duplicates.
* `/api/suggestions/buffer`: include `matchedTask` details on merge.
* Admin review endpoints & basic UI.
* Event logging table (optional) for analytics.
* Observability: latency logs and a lightweight metrics endpoint.
