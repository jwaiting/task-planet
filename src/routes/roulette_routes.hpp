#pragma once
#include <random>

#include "../models/task.hpp"
#include "crow_all.h"

void registerRouletteRoutes(crow::SimpleApp& app, std::vector<Task>& taskPool) {
    CROW_ROUTE(app, "/roulette")([&taskPool]() {
        if (taskPool.empty())
            return crow::response(404, "No tasks available");

        std::mt19937                    rng(std::random_device{}());
        std::uniform_int_distribution<> dist(0, taskPool.size() - 1);
        const Task&                     task = taskPool[dist(rng)];

        crow::json::wvalue result;
        result["description"] = task.description;
        result["mood"]        = task.moodTag;
        result["minTime"]     = task.minTime;

        return crow::response(result);
    });
}
