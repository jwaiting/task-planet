#pragma once
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "../models/task_suggestion.hpp"
#include "../models/task.hpp"

// Helper: mood serialize/deserialize
inline std::string serializeMood(const std::vector<std::string>& mood) {
    nlohmann::json j = mood;
    return j.dump();
}

inline std::vector<std::string> deserializeMood(const std::string& moodJson) {
    auto j = nlohmann::json::parse(moodJson);
    return j.get<std::vector<std::string>>();
}

// Helper: status <-> string
inline std::string statusToString(TaskStatus status) {
    switch (status) {
        case TaskStatus::Approved:
            return "approved";
        case TaskStatus::Rejected:
            return "rejected";
        default:
            return "pending";
    }
}

inline TaskStatus stringToStatus(const std::string& s) {
    if (s == "approved")
        return TaskStatus::Approved;
    if (s == "rejected")
        return TaskStatus::Rejected;
    return TaskStatus::Pending;
}

// Helper: timestamp <-> time_point
inline std::chrono::system_clock::time_point parseTimestamp(const std::string& ts) {
    std::tm            tm = {};
    std::istringstream ss(ts);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

inline std::string formatTimestamp(const std::chrono::system_clock::time_point& tp) {
    std::time_t t  = std::chrono::system_clock::to_time_t(tp);
    std::tm     tm = *std::localtime(&t);
    char        buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}
