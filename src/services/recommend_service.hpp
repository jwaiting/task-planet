#pragma once
#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include "../repositories/task_repo.hpp"

struct RecommendItem
{
    int         id;
    std::string description;
    int         suggestedTime;
    double      tagFit;
    double      timeFit;
    double      scoreQuality;
    double      scorePopularity;
    double      finalScore;
};

class RecommendService
{
   public:
    explicit RecommendService(pqxx::connection& c) : c_(c), tasks_(c) {}

    std::vector<RecommendItem> recommend(const std::vector<int>& tagIds,
                                         int                     timeMinutes,
                                         int                     limit) {
        auto                       rows = tasks_.recommend_rows(tagIds, limit);
        std::vector<RecommendItem> out;
        out.reserve(rows.size());
        for (auto const& row : rows) {
            RecommendItem it;
            it.id              = row["id"].as<int>();
            it.description     = row["description"].as<std::string>();
            it.suggestedTime   = row["suggested_time"].as<int>();
            it.tagFit          = row["tag_fit"].as<double>();
            it.timeFit         = time_fit(timeMinutes, it.suggestedTime);
            it.scoreQuality    = row["score_quality"].as<double>();
            it.scorePopularity = row["score_popularity"].as<double>();
            it.finalScore      = 0.55 * it.tagFit + 0.25 * it.timeFit +
                            0.12 * it.scoreQuality + 0.08 * it.scorePopularity;
            out.push_back(std::move(it));
        }
        std::sort(out.begin(), out.end(), [](auto const& a, auto const& b) {
            return a.finalScore > b.finalScore;
        });
        return out;
    }

   private:
    pqxx::connection& c_;
    TaskRepo          tasks_;

    static double time_fit(int userMin, int sugMin) {
        const double eps = 1e-6;
        if (userMin <= 0 || sugMin <= 0)
            return 0.8;
        return std::exp(-std::fabs(std::log((userMin + eps) / (sugMin + eps))));
    }
};
