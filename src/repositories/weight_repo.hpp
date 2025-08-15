#pragma once
#include <pqxx/pqxx>
#include <vector>
#include <utility>

class WeightRepo
{
   public:
    explicit WeightRepo(pqxx::connection& c) : c_(c) {}

    // 採用事件：alpha += 1（snake_case）
    void reinforce(int taskId, const std::vector<int>& tagIds) {
        pqxx::work tx(c_);
        for (int tagId : tagIds) {
            tx.exec_prepared("tasktag_upsert_adopt", taskId, tagId);
        }
        tx.commit();
        std::cout << "[DEBUG] reinforce committed taskId=" << taskId
                  << " count=" << tagIds.size() << std::endl;
    }

    // 設定 base_weight（snake_case）
    void set_base_weights(int                                        taskId,
                          const std::vector<std::pair<int, double>>& tagWeights) {
        pqxx::work tx(c_);
        for (auto const& tw : tagWeights) {
            int    tagId = tw.first;
            double base  = tw.second;
            tx.exec_params(
                R"(INSERT INTO task_tag_weight (task_id, tag_id, base_weight, alpha, beta)
           VALUES ($1,$2,$3,1.0,9.0)
           ON CONFLICT (task_id, tag_id)
           DO UPDATE SET base_weight = EXCLUDED.base_weight, updated_at = now())",
                taskId,
                tagId,
                base);
        }
        tx.commit();
    }

   private:
    pqxx::connection& c_;
};
