#pragma once

#include <vector>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include "../models/task.hpp"

inline std::vector<Task> loadTasksFromDB(const std::string& connStr) {
    std::vector<Task> tasks;

    try {
        pqxx::connection conn(connStr);
        pqxx::work       txn(conn);

        // 使用別名，全部轉成小寫
        auto result = txn.exec(R"(
            SELECT 
                "id" AS id,
                "description" AS description,
                "mood" AS mood,
                "suggestedTime" AS suggestedtime,
                "createdAt" AS createdat
            FROM "Task"
        )");

        for (const auto& row : result) {
            Task t;
            t.id          = row["id"].as<int>();
            t.description = row["description"].as<std::string>();

            auto moodStr = row["mood"].as<std::string>();
            t.mood = nlohmann::json::parse(moodStr).get<std::vector<std::string>>();

            t.suggestedTime = row["suggestedtime"].as<int>();
            t.createdAt     = row["createdat"].as<std::string>();

            tasks.push_back(t);
        }

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "[loadTasksFromDB] Error: " << e.what() << std::endl;
    }

    return tasks;
}
