#pragma once
#include <vector>
#include <random>
#include <algorithm>
#include "../models/task.hpp"

inline std::vector<Task> selectTasks(const std::vector<Task>& pool,
                                     const std::string&       mood,
                                     int                      time) {
    std::vector<Task> filtered;

    for (const auto& task : pool) {
        // 檢查是否 mood 陣列中有包含傳入的 mood
        bool moodMatch =
            (mood == "any") ||
            (std::find(task.mood.begin(), task.mood.end(), mood) != task.mood.end());

        if (moodMatch && task.suggestedTime <= time) {
            filtered.push_back(task);
        }
    }

    // 隨機打亂順序
    std::shuffle(
        filtered.begin(), filtered.end(), std::mt19937{std::random_device{}()});

    // 最多只回傳三個任務
    if (filtered.size() > 3)
        filtered.resize(3);

    return filtered;
}
