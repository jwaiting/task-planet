#pragma once
#include <random>
#include <pqxx/pqxx>
#include "../models/task.hpp"
#include "../utils/db_task.hpp"
#include "crow_all.h"
#include "../utils/task_json.hpp"

void registerRouletteRoutes(crow::SimpleApp& app, pqxx::connection& conn) {
    CROW_ROUTE(app, "/api/roulette")([&conn]() {
        auto taskPool = loadTasksFromDB(conn);

        if (taskPool.empty())
            return crow::response(404, "No tasks available");

        std::mt19937                    rng(std::random_device{}());
        std::uniform_int_distribution<> dist(0, taskPool.size() - 1);
        const Task&                     task = taskPool[dist(rng)];

        return crow::response(json(task).dump());
    });
}
