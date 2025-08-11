#pragma once
#include "crow_all.h"
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include "../utils/db_task_suggestion.hpp"

inline void registerSuggestionBufferRoutes(crow::SimpleApp&  app,
                                           pqxx::connection& conn) {
    // 取得所有 buffer 任務
    CROW_ROUTE(app, "/api/suggestions/buffer").methods("GET"_method)([&conn]() {
        auto           tasks = DBTaskSuggestion::fetchAll(conn);
        nlohmann::json res   = nlohmann::json::array();

        for (const auto& t : tasks) {
            res.push_back({{"id", t.id},
                           {"description", t.description},
                           {"mood", t.mood},
                           {"suggestedTime", t.suggestedTime},
                           {"status", statusToString(t.status)},
                           {"votes", t.votes},
                           {"createdAt", formatTimestamp(t.createdAt)}});
        }
        return crow::response(res.dump());
    });

    // 依 mood 和時間篩選 buffer 任務
    CROW_ROUTE(app, "/api/suggestions/buffer/filter")
        .methods("GET"_method)([&conn](const crow::request& req) {
            std::string moodParam =
                req.url_params.get("mood") ? req.url_params.get("mood") : "";
            int timeParam = req.url_params.get("time")
                                ? std::stoi(req.url_params.get("time"))
                                : 9999;

            pqxx::work txn(conn);

            auto result = txn.exec_params(
                R"SQL(
            SELECT id, description, mood, "suggestedTime" as suggestedtime, votes
            FROM "TaskSuggestionBuffer"
            WHERE $1 = ANY (SELECT jsonb_array_elements_text(mood::jsonb))
              AND "suggestedTime" <= $2
            ORDER BY votes DESC
            LIMIT 10
        )SQL",
                moodParam,
                timeParam);

            nlohmann::json out = nlohmann::json::array();
            for (const auto& row : result) {
                out.push_back({{"id", row["id"].as<int>()},
                               {"description", row["description"].c_str()},
                               {"mood", nlohmann::json::parse(row["mood"].c_str())},
                               {"minTime", row["suggestedTime"].as<int>()},
                               {"voteCount", row["votes"].as<int>()}});
            }

            return crow::response(out.dump());
        });

    // 投票
    CROW_ROUTE(app, "/api/suggestions/buffer/<int>/vote")
        .methods("POST"_method)([&conn](int id) {
            try {
                pqxx::work txn(conn);
                auto       r = txn.exec_params(
                    R"SQL(
                UPDATE "TaskSuggestionBuffer"
                SET votes = votes + 1
                WHERE id = $1
                RETURNING votes
            )SQL",
                    id);
                if (r.affected_rows() == 0) {
                    txn.commit();
                    return crow::response(404, "Not found");
                }
                int newVotes = r[0]["votes"].as<int>();
                txn.commit();

                nlohmann::json resp;
                resp["votes"] = newVotes;
                return crow::response(resp.dump());
            }
            catch (const std::exception& e) {
                return crow::response(500, std::string("DB error: ") + e.what());
            }
        });

    // 新增 buffer 任務
    CROW_ROUTE(app, "/api/suggestions/buffer")
        .methods("POST"_method)([&conn](const crow::request& req) {
            try {
                auto body = nlohmann::json::parse(req.body);
                if (!body.contains("description") || !body.contains("mood") ||
                    !body.contains("suggestedTime")) {
                    return crow::response(400, "Missing required fields");
                }

                DBTaskSuggestion::insert(
                    conn, body["description"], body["mood"], body["suggestedTime"]);
                return crow::response(201);
            }
            catch (const std::exception& e) {
                return crow::response(400,
                                      std::string("Invalid request: ") + e.what());
            }
        });

    // 審核 (approve)
    CROW_ROUTE(app, "/api/suggestions/buffer/<int>/approve")
        .methods("POST"_method)([&conn](int id) {
            try {
                DBTaskSuggestion::approve(conn, id);
                return crow::response(200, "Task approved and moved to main pool");
            }
            catch (const std::runtime_error& e) {
                if (std::string(e.what()) == "NOT_FOUND_OR_ALREADY_PROCESSED") {
                    return crow::response(404,
                                          "Task not found or already processed");
                }
                return crow::response(500, "Internal server error");
            }
        });

    // 拒絕 (reject)
    CROW_ROUTE(app, "/api/suggestions/buffer/<int>/reject")
        .methods("POST"_method)([&conn](int id) {
            try {
                DBTaskSuggestion::reject(conn, id);
                return crow::response(200, "Task rejected");
            }
            catch (const std::runtime_error& e) {
                if (std::string(e.what()) == "NOT_FOUND_OR_ALREADY_PROCESSED") {
                    return crow::response(404,
                                          "Task not found or already processed");
                }
                return crow::response(500, "Internal server error");
            }
        });
}
