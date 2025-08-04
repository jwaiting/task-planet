#pragma once

#include <vector>
#include <pqxx/pqxx>
#include <nlohmann/json.hpp>
#include "../models/task.hpp"

// 讀取 Task 資料
inline std::vector<Task> loadTasksFromDB(pqxx::connection& conn) {
    std::vector<Task> tasks;

    try {
        pqxx::work txn(conn);

        // 因為沒有參數，所以直接用 exec()
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

            tasks.push_back(std::move(t));
        }

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "[loadTasksFromDB] Error: " << e.what() << std::endl;
    }

    return tasks;
}

// 新增任務到 Task table
inline void insertTaskToDB(pqxx::connection&               conn,
                           const std::string&              description,
                           const std::vector<std::string>& mood,
                           int                             suggestedTime) {
    try {
        pqxx::work txn(conn);

        nlohmann::json moodJson = mood;

        txn.exec_params(
            R"(
        INSERT INTO "Task" ("description", "mood", "suggestedTime")
        VALUES ($1, $2, $3)
    )",
            description,
            moodJson.dump(),
            suggestedTime);

        txn.commit();
    }
    catch (const std::exception& e) {
        std::cerr << "[insertTaskToDB] Error: " << e.what() << std::endl;
    }
}
