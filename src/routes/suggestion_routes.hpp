#pragma once
#include <vector>
#include <nlohmann/json.hpp>
#include <pqxx/pqxx>

#include "../models/task.hpp"
#include "../utils/task_selector.hpp"
#include "../utils/task_json.hpp"
#include "../utils/db_task.hpp" // ✅ 改用 DB 取任務

#include "crow_all.h"
using nlohmann::json;

void registerSuggestionRoutes(crow::SimpleApp& app, pqxx::connection& conn) {
    CROW_ROUTE(app, "/suggestions")
        .methods("POST"_method)([&conn](const crow::request& req) {
            json body;
            try {
                body = json::parse(req.body);
            }
            catch (...) {
                return crow::response(400, "Invalid JSON");
            }

            if (!body.contains("mood") || !body.contains("time")) {
                return crow::response(400, "Missing mood or time field");
            }

            std::string mood = body["mood"];
            int         time = body["time"];

            // 從 DB 重新讀取最新的任務
            auto taskPool = loadTasksFromDB(conn);

            // 選擇符合 mood 和 time 的任務
            auto suggestions = selectTasks(taskPool, mood, time);

            json result = json::array();
            for (const auto& task : suggestions) {
                result.push_back(task); // 觸發 to_json(task)
            }

            return crow::response(result.dump());
        });
}
