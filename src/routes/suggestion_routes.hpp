#pragma once
#include "crow_all.h"
#include "../models/task.hpp"
#include "../utils/task_selector.hpp"
#include <vector>

void registerSuggestionRoutes(crow::SimpleApp& app, std::vector<Task>& taskPool) {
    CROW_ROUTE(app, "/suggestions").methods("POST"_method)(
        [&taskPool](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body)
                return crow::response(400, "Invalid JSON");

            std::string mood = body["mood"].s();
            int time = body["time"].i();

            auto suggestions = selectTasks(taskPool, mood, time);

            crow::json::wvalue result;
            for (size_t i = 0; i < suggestions.size(); ++i) {
                result[i]["description"] = suggestions[i].description;
                result[i]["mood"] = suggestions[i].moodTag;
                result[i]["minTime"] = suggestions[i].minTime;
            }

            return crow::response(result);
        }
    );
}
