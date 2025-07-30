#pragma once
#include <vector>
#include <nlohmann/json.hpp>

#include "../models/task.hpp"
#include "../utils/task_selector.hpp"
#include "../utils/task_json.hpp" // ✅ 若有 to_json(Task)

#include "crow_all.h"
using nlohmann::json;

void registerSuggestionRoutes(crow::SimpleApp& app, std::vector<Task>& taskPool) {
    CROW_ROUTE(app, "/suggestions")
        .methods("POST"_method)([&taskPool](const crow::request& req) {
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

            auto suggestions = selectTasks(taskPool, mood, time);

            json result = json::array();
            for (const auto& task : suggestions) {
                result.push_back(task); // 觸發 to_json(task)
            }

            return crow::response(result.dump());
        });
}
