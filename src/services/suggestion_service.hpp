#pragma once
#include <pqxx/pqxx>
#include <optional>
#include <string>
#include <vector>
#include "../repositories/task_repo.hpp"
#include "../repositories/suggestion_repo.hpp"
#include "../repositories/weight_repo.hpp"

struct SuggestionResult
{
    bool                  merged       = false;
    int                   suggestionId = 0;
    std::optional<int>    matchedTaskId;
    std::optional<double> similarity;
};

class SuggestionService
{
   public:
    explicit SuggestionService(pqxx::connection& c, double simThreshold = 0.87)
        : c_(c),
          taskRepo_(c),
          suggRepo_(c),
          weightRepo_(c),
          simThreshold_(simThreshold) {}

    SuggestionResult create_or_alias(const std::string&      description,
                                     int                     suggestedTime,
                                     const std::vector<int>& tagIds) {
        // 1) 找相似任務
        auto cands = taskRepo_.find_similar(description, simThreshold_, /*limit*/ 1);
        if (!cands.empty()) {
            // 命中：建立 merged 的 suggestion、寫 alias、強化該任務的權重
            int    taskId = cands.front().id;
            double sim    = cands.front().similarity;

            int suggId =
                suggRepo_.insert(description, suggestedTime, "merged", /*votes*/ 1);
            suggRepo_.upsert_alias(suggId, taskId, sim);
            if (!tagIds.empty())
                weightRepo_.reinforce(taskId, tagIds);

            SuggestionResult r;
            r.merged        = true;
            r.suggestionId  = suggId;
            r.matchedTaskId = taskId;
            r.similarity    = sim;
            return r;
        }

        // 未命中：進 buffer 並附上 tags
        int suggId =
            suggRepo_.insert(description, suggestedTime, "pending", /*votes*/ 1);
        if (!tagIds.empty())
            suggRepo_.insert_tags(
                suggId, tagIds, /*baseWeight*/ 0.6, /*alpha*/ 1.0, /*beta*/ 9.0);

        SuggestionResult r;
        r.merged       = false;
        r.suggestionId = suggId;
        return r;
    }

   private:
    pqxx::connection& c_;
    TaskRepo          taskRepo_;
    SuggestionRepo    suggRepo_;
    WeightRepo        weightRepo_;
    double            simThreshold_;
};
