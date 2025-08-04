#pragma once
#include <string>
#include <vector>
#include <chrono>

enum class TaskStatus
{
    Pending,
    Approved,
    Rejected
};

struct TaskSuggestion
{
    int                                   id = 0;
    std::string                           description;
    std::vector<std::string>              mood;
    int                                   suggestedTime = 0;
    TaskStatus                            status        = TaskStatus::Pending;
    int                                   votes         = 0;
    std::chrono::system_clock::time_point createdAt;
};
