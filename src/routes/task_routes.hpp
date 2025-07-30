#pragma once
#include <vector>
#include <nlohmann/json.hpp>

#include "../models/task.hpp"
#include "../utils/task_json.hpp" // 有 to_json() / from_json()
#include "crow_all.h"

using nlohmann::json;

void registerTaskRoutes(crow::SimpleApp& app, std::vector<Task>& taskPool) {
    // GET /tasks - 回傳全部任務
    CROW_ROUTE(app, "/tasks")([&taskPool]() {
        json result = json::array();
        for (const auto& task : taskPool) result.push_back(task); // 自動觸發 to_json
        return crow::response(result.dump());
    });

    // POST /task - 新增任務到 memory
    CROW_ROUTE(app, "/task")
        .methods("POST"_method)([&taskPool](const crow::request& req) {
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

            Task newTask;
            newTask.description   = body["description"];
            newTask.mood          = body["mood"].get<std::vector<std::string>>();
            newTask.suggestedTime = body["suggestedTime"];
            newTask.createdAt     = "local"; // 或 std::format 現在時間（暫定）

            taskPool.push_back(newTask);
            return crow::response(201, "Task added");
        });
}
