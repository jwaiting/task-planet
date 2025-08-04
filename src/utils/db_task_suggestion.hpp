#pragma once
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include "../models/task_suggestion.hpp"
#include "./db_helpers.hpp"

namespace DBTaskSuggestion {

    inline std::vector<TaskSuggestion> fetchAll(pqxx::connection& conn) {
        pqxx::work txn(conn);
        auto       result = txn.exec(pqxx::zview(R"SQL(
            SELECT 
                "id" AS id,
                "description" AS description,
                "mood" AS mood,
                "suggestedTime" AS suggested_time,
                "status" AS status,
                "votes" AS votes,
                "createdAt" AS created_at
            FROM "TaskSuggestionBuffer"
        )SQL"));
        txn.commit();

        std::vector<TaskSuggestion> tasks;
        for (const auto& row : result) {
            TaskSuggestion t;
            t.id          = row["id"].as<int>();
            t.description = row["description"].as<std::string>();
            t.mood        = nlohmann::json::parse(row["mood"].as<std::string>())
                         .get<std::vector<std::string>>();
            t.suggestedTime = row["suggested_time"].as<int>();
            t.status        = stringToStatus(row["status"].as<std::string>());
            t.votes         = row["votes"].as<int>();
            t.createdAt     = parseTimestamp(row["created_at"].as<std::string>());
            tasks.push_back(std::move(t));
        }
        return tasks;
    }

    inline void insert(pqxx::connection&     conn,
                       const std::string&    description,
                       const nlohmann::json& mood,
                       int                   suggestedTime) {
        pqxx::work txn(conn);
        // ✅ 用雙引號確保駝峰命名
        txn.exec_params(
            R"SQL(
                INSERT INTO "TaskSuggestionBuffer" ("description", "mood", "suggestedTime")
                VALUES ($1, $2, $3)
            )SQL",
            description,
            mood.dump(),
            suggestedTime);
        txn.commit();
    }

    inline void approve(pqxx::connection& conn, int id) {
        pqxx::work txn(conn);

        auto result = txn.exec_params(
            R"SQL(
            UPDATE "TaskSuggestionBuffer"
            SET "status" = 'approved'
            WHERE "id" = $1 AND "status" = 'pending'
        )SQL",
            id);

        if (result.affected_rows() == 0) {
            // 可能是 ID 不存在或狀態不是 pending
            throw std::runtime_error("NOT_FOUND_OR_ALREADY_PROCESSED");
        }

        txn.commit();
    }

    inline void reject(pqxx::connection& conn, int id) {
        pqxx::work txn(conn);

        auto result = txn.exec_params(
            R"SQL(
            UPDATE "TaskSuggestionBuffer"
            SET "status" = 'rejected'
            WHERE "id" = $1 AND "status" = 'pending'
        )SQL",
            id);

        if (result.affected_rows() == 0) {
            throw std::runtime_error("NOT_FOUND_OR_ALREADY_PROCESSED");
        }

        txn.commit();
    }

} // namespace DBTaskSuggestion
