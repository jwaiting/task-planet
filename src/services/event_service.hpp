#pragma once
#include <pqxx/pqxx>
#include <string>
#include <vector>
#include "../repositories/weight_repo.hpp"

class EventService
{
   public:
    explicit EventService(pqxx::connection& c) : c_(c), weightRepo_(c) {}

    void handle_event(int                     taskId,
                      const std::string&      event,
                      const std::vector<int>& tagIds) {
        if (taskId <= 0 || event.empty())
            return;

        if (event == "adopt") {
            if (!tagIds.empty())
                weightRepo_.reinforce(taskId, tagIds);
            std::cout << "[DEBUG] adopt taskId=" << taskId << " tags=";
            for (size_t i = 0; i < tagIds.size(); ++i) {
                if (i)
                    std::cout << ',';
                std::cout << tagIds[i];
            }
            std::cout << " at " << std::time(nullptr) << std::endl;
            return;
        }

        if (event == "skip" || event == "impression") {
            pqxx::work tx(c_);
            for (int tagId : tagIds) {
                tx.exec_params(
                    R"(INSERT INTO task_tag_weight (task_id, tag_id, base_weight, alpha, beta)
             VALUES ($1,$2,0.5,1.0,9.0)
             ON CONFLICT (task_id, tag_id)
             DO UPDATE SET beta = task_tag_weight.beta + 1.0,
                           updated_at = now())",
                    taskId,
                    tagId);
            }
            tx.commit();
        }
    }

   private:
    pqxx::connection& c_;
    WeightRepo        weightRepo_;
};
