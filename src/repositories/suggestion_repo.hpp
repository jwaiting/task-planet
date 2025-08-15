#pragma once
#include <pqxx/pqxx>
#include <vector>
#include <string>

class SuggestionRepo
{
   public:
    explicit SuggestionRepo(pqxx::connection& c) : c_(c) {}

    int insert(const std::string& description,
               int                suggested_time,
               const std::string& status = "pending",
               int                votes  = 1) {
        pqxx::work tx(c_);
        auto       r = tx.exec_prepared(
            "sugg_insert", description, suggested_time, status, votes);
        int id = r[0]["id"].as<int>();
        tx.commit();
        return id;
    }

    void insert_tags(int                     suggestionId,
                     const std::vector<int>& tagIds,
                     double                  baseWeight = 0.6,
                     double                  alpha      = 1.0,
                     double                  beta       = 9.0) {
        pqxx::work tx(c_);
        for (int tagId : tagIds) {
            tx.exec_prepared(
                "sugg_tag_insert", suggestionId, tagId, baseWeight, alpha, beta);
        }
        tx.commit();
    }

    void upsert_alias(int suggestionId, int taskId, double similarity) {
        pqxx::work tx(c_);
        tx.exec_prepared("alias_upsert", suggestionId, taskId, similarity);
        tx.commit();
    }

   private:
    pqxx::connection& c_;
};
