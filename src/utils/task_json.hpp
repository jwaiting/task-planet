#pragma once

#include <nlohmann/json.hpp>
#include "../models/task.hpp"

using nlohmann::json;

// Task → JSON
inline void to_json(json& j, const Task& t) {
    j = json{{"id", t.id},
             {"description", t.description},
             {"mood", t.mood},
             {"suggestedTime", t.suggestedTime},
             {"createdAt", t.createdAt}};
}

// JSON → Task
inline void from_json(const json& j, Task& t) {
    j.at("description").get_to(t.description);
    j.at("mood").get_to(t.mood);
    j.at("suggestedTime").get_to(t.suggestedTime);
}
