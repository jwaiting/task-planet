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
