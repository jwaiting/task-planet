#pragma once
#include <random>
#include "../models/task.hpp"
#include "crow_all.h"
#include "../utils/task_json.hpp"

void registerRouletteRoutes(crow::SimpleApp& app, std::vector<Task>& taskPool) {
    CROW_ROUTE(app, "/roulette")([&taskPool]() {
        if (taskPool.empty())
            return crow::response(404, "No tasks available");

        std::mt19937                    rng(std::random_device{}());
        std::uniform_int_distribution<> dist(0, taskPool.size() - 1);
        const Task&                     task = taskPool[dist(rng)];

        return crow::response(json(task).dump());
    });
}
