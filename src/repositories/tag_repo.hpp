#pragma once
#include <pqxx/pqxx>
#include <vector>
#include <string>

struct TagRow
{
    int         id;
    std::string code;
    std::string label;
    std::string group_code;
    bool        is_active;
};

class TagRepo
{
   public:
    explicit TagRepo(pqxx::connection& c) : c_(c) {}

    std::vector<int> ids_by_codes(const std::vector<std::string>& codes) {
        if (codes.empty())
            return {};
        pqxx::work tx(c_);

        // 產生 Postgres 陣列字串：{"context/desk","focus/high"}
        std::string arr = to_pg_text_array(codes);

        // ✅ 用參數化，讓 $1 轉成 text[] 後餵給 ANY()
        auto r = tx.exec_params(
            "SELECT id FROM tag_dim WHERE code = ANY($1::text[])", arr);

        tx.commit();
        std::vector<int> out;
        out.reserve(r.size());
        for (auto const& row : r) out.push_back(row["id"].as<int>());
        return out;
    }

    std::vector<TagRow> list_active() {
        pqxx::work tx(c_);
        auto       r = tx.exec(R"(SELECT id, code, label, group_code, is_active
                        FROM tag_dim WHERE is_active = TRUE
                        ORDER BY group_code, code)");
        tx.commit();
        std::vector<TagRow> out;
        out.reserve(r.size());
        for (auto const& row : r) {
            out.push_back({row["id"].as<int>(),
                           row["code"].as<std::string>(),
                           row["label"].as<std::string>(),
                           row["group_code"].as<std::string>(),
                           row["is_active"].as<bool>()});
        }
        return out;
    }

   private:
    pqxx::connection& c_;

    static std::string to_pg_text_array(const std::vector<std::string>& v) {
        // 產生 {"a","b"}（簡單轉義引號與反斜線）
        std::string s = "{";
        for (size_t i = 0; i < v.size(); ++i) {
            if (i)
                s += ',';
            s += '"';
            for (char ch : v[i]) {
                if (ch == '"')
                    s += "\\\"";
                else if (ch == '\\')
                    s += "\\\\";
                else
                    s += ch;
            }
            s += '"';
        }
        s += "}";
        return s;
    }
};
