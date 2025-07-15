#pragma once
#include <vector>

#include "../models/task.hpp"
#include "crow_all.h"

void registerTaskRoutes(crow::SimpleApp& app, std::vector<Task>& taskPool) {
    CROW_ROUTE(app, "/tasks")([&taskPool]() {
        crow::json::wvalue result;
        int                i = 0;
        for (const auto& task : taskPool) {
            result[i]["description"] = task.description;
            result[i]["mood"]        = task.moodTag;
            result[i]["minTime"]     = task.minTime;
            ++i;
        }
        return result;
    });

    CROW_ROUTE(app, "/task")
        .methods("POST"_method)([&taskPool](const crow::request& req) {
            auto body = crow::json::load(req.body);
            if (!body)
                return crow::response(400, "Invalid JSON");

            Task newTask{body["description"].s(),
                         body["mood"].s(),
                         static_cast<int>(body["minTime"].i())};

            taskPool.push_back(newTask);
            return crow::response(201, "Task added");
        });
}
