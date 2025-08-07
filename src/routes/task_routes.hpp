#pragma once
#include <vector>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>
#include "crow_all.h"

#include "../models/task.hpp"
#include "../utils/task_json.hpp"
#include "../utils/db_task.hpp" // 用來存取 DB

using nlohmann::json;

void registerTaskRoutes(crow::SimpleApp& app, pqxx::connection& conn) {
    // GET /tasks - 從 DB 抓取全部任務
    CROW_ROUTE(app, "/api/tasks")
    ([&conn]() {
        auto tasks  = loadTasksFromDB(conn);
        json result = json::array();
        for (const auto& task : tasks) result.push_back(task);
        return crow::response(result.dump());
    });

    // POST /task - 新增任務到 DB
    CROW_ROUTE(app, "/api/task")
        .methods("POST"_method)([&conn](const crow::request& req) {
            json body;
            try {
                body = json::parse(req.body);
            }
            catch (...) {
                return crow::response(400, "Invalid JSON");
            }

            if (!body.contains("description") || !body.contains("mood") ||
                !body.contains("suggestedTime")) {
                return crow::response(400, "Missing fields");
            }

            try {
                pqxx::work txn(conn);
                txn.exec_params(
                    R"(INSERT INTO "Task" (description, mood, "suggestedTime") VALUES ($1, $2, $3))",
                    body["description"].get<std::string>(),
                    body["mood"].dump(),
                    body["suggestedTime"].get<int>());
                txn.commit();
            }
            catch (const std::exception& e) {
                return crow::response(500, std::string("DB Error: ") + e.what());
            }

            return crow::response(201, "Task added");
        });
}
