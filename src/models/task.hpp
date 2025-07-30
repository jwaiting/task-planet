#pragma once
#include <string>
#include <vector>

struct Task
{
    int                      id = 0;
    std::string              description;
    std::vector<std::string> mood;
    int                      suggestedTime = 0;
    std::string              createdAt;
};