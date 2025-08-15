#pragma once
#include <pqxx/pqxx>

inline void register_prepared(pqxx::connection& c) {
    auto prepare_once = [&](const char* name, const char* sql) {
        try {
            c.prepare(name, sql);
        }
        catch (const pqxx::usage_error&) { /* already prepared on this conn */
        }
    };

    // 相似檢索（pg_trgm）
    prepare_once("suggest_similar",
                 R"(SELECT id,
              similarity(LOWER(description), LOWER($1)) AS sim
       FROM   tasks
       WHERE  similarity(LOWER(description), LOWER($1)) > $2
       ORDER  BY sim DESC
       LIMIT  $3)");

    // alias upsert
    prepare_once(
        "alias_upsert",
        R"(INSERT INTO suggestion_alias ("suggestion_id","task_id", similarity)
       VALUES ($1,$2,$3)
       ON CONFLICT ("suggestion_id")
       DO UPDATE SET "task_id"=$2, similarity=$3, matched_at=now())");

    // 採用事件：task_tag_weight upsert（snake_case）
    prepare_once(
        "tasktag_upsert_adopt",
        R"(INSERT INTO task_tag_weight (task_id, tag_id, base_weight, alpha, beta)
       VALUES ($1,$2,0.5,1.0,9.0)
       ON CONFLICT (task_id, tag_id)
       DO UPDATE SET alpha = task_tag_weight.alpha + 1.0,
                     updated_at = now())");

    // 新增 suggestion（snake_case）
    prepare_once(
        "sugg_insert",
        R"(INSERT INTO task_suggestion_buffer (description, suggested_time, status, votes)
       VALUES ($1,$2,$3,$4)
       RETURNING id)");

    // suggestion_tag_weight（snake_case）
    prepare_once(
        "sugg_tag_insert",
        R"(INSERT INTO suggestion_tag_weight ("suggestion_id","tag_id", base_weight, alpha, beta)
       VALUES ($1,$2,$3,$4,$5)
       ON CONFLICT DO NOTHING)");

    // 推薦查詢（snake_case join）
    prepare_once("recommend_query",
                 R"(WITH picked AS (
         SELECT UNNEST($1::int[]) AS tag_id
       ),
       fits AS (
         SELECT t.id AS task_id,
                AVG(0.4*ttw.base_weight + 0.6*(ttw.alpha/NULLIF(ttw.alpha+ttw.beta,0))) AS tag_fit
         FROM   tasks t
         JOIN   task_tag_weight ttw ON ttw.task_id = t.id
         JOIN   picked p           ON p.tag_id     = ttw.tag_id
         GROUP  BY t.id
       )
       SELECT t.id,
              t.description,
              t.suggested_time,
              COALESCE(f.tag_fit, 0.1)          AS tag_fit,
              COALESCE(ts.score_quality, 0.0)   AS score_quality,
              COALESCE(ts.score_popularity, 0.0)AS score_popularity,
              t.created_at
       FROM   tasks t
       LEFT   JOIN fits       f  ON f.task_id  = t.id
       LEFT   JOIN task_stats ts ON ts.task_id = t.id
       ORDER  BY t.created_at DESC
       LIMIT  $2)");
}
