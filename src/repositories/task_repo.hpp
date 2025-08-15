#pragma once
#include <pqxx/pqxx>
#include <vector>
#include <string>
#include <optional>

struct TaskCandidate
{
    int    id;
    double similarity;
};

struct TaskRow
{
    int         id;
    std::string description;
    int         suggested_time;
};

class TaskRepo
{
   public:
    explicit TaskRepo(pqxx::connection& c) : c_(c) {}

    std::vector<TaskCandidate> find_similar(const std::string& desc,
                                            double             threshold,
                                            int                limit = 3) {
        pqxx::work tx(c_);
        auto       r = tx.exec_prepared("suggest_similar", desc, threshold, limit);
        tx.commit();
        std::vector<TaskCandidate> out;
        out.reserve(r.size());
        for (auto const& row : r) {
            out.push_back({row["id"].as<int>(), row["sim"].as<double>()});
        }
        return out;
    }

    std::optional<TaskRow> get_by_id(int id) {
        pqxx::work tx(c_);
        auto       r = tx.exec_params(
            "SELECT id, description, suggested_time FROM tasks WHERE id=$1", id);
        tx.commit();
        if (r.empty())
            return std::nullopt;
        TaskRow t;
        t.id             = r[0]["id"].as<int>();
        t.description    = r[0]["description"].as<std::string>();
        t.suggested_time = r[0]["suggested_time"].as<int>();
        return t;
    }

    int create(const std::string& description, int suggested_time) {
        pqxx::work tx(c_);
        auto       r = tx.exec_params(
            "INSERT INTO tasks(description, suggested_time) VALUES ($1,$2) "
                  "RETURNING id",
            description,
            suggested_time);
        tx.commit();
        return r[0]["id"].as<int>();
    }

    // 取得基礎推薦查詢結果（給 service 做 time_fit 與最後排序）
    pqxx::result recommend_rows(const std::vector<int>& tagIds, int limit) {
        pqxx::work  tx(c_);
        std::string arr = to_pg_array(tagIds);
        auto        r   = tx.exec_prepared("recommend_query", arr, limit);
        tx.commit();
        return r;
    }

   private:
    pqxx::connection&  c_;
    static std::string to_pg_array(const std::vector<int>& v) {
        std::string s = "{";
        for (size_t i = 0; i < v.size(); ++i) {
            if (i)
                s += ',';
            s += std::to_string(v[i]);
        }
        s += "}";
        return s;
    }
};
