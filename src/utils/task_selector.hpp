// utils/task_selector.hpp
inline std::vector<Task> selectTasks(const std::vector<Task>& pool, const std::string& mood, int time) {
    std::vector<Task> filtered;

    for (const auto& task : pool) {
        if ((mood == "any" || task.moodTag == mood) && task.minTime <= time) {
            filtered.push_back(task);
        }
    }

    // 隨機打亂
    std::shuffle(filtered.begin(), filtered.end(), std::mt19937{std::random_device{}()});

    // 最多只回傳三個任務
    if (filtered.size() > 3) filtered.resize(3);

    return filtered;
}
