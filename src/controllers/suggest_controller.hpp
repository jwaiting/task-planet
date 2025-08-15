#pragma once
#include <crow_all.h>
#include <vector>
#include <string>
#include "../db/pool.hpp"
#include "../db/prepared.hpp"
#include "../repositories/tag_repo.hpp"
#include "../services/recommend_service.hpp"

template <typename App>
inline void attach_suggest_routes(App& app, DbPool& pool) {
    CROW_ROUTE(app, "/api/suggest")
        .methods("POST"_method)([&pool](const crow::request& req) {
            auto j = crow::json::load(req.body);
            if (!j)
                return crow::response{400, "invalid json"};

            std::vector<int> tagIds;
            // 支援兩種輸入：tags (int[]) 或 tagCodes (string[])
            if (j.has("tags") && j["tags"].t() == crow::json::type::List) {
                for (auto& v : j["tags"]) tagIds.push_back((int)v.i());
            }
            std::vector<std::string> tagCodes;
            if (j.has("tagCodes") && j["tagCodes"].t() == crow::json::type::List) {
                for (auto& v : j["tagCodes"])
                    tagCodes.emplace_back(std::string(v.s()));
            }
            int timeMin = j.has("time") ? (int)j["time"].i() : 10;
            int limit   = j.has("limit") ? (int)j["limit"].i() : 20;

            try {
                auto h = pool.acquire();

                if (!tagCodes.empty()) {
                    TagRepo tr(*h);
                    auto    ids = tr.ids_by_codes(tagCodes);
                    tagIds.insert(tagIds.end(), ids.begin(), ids.end());
                }

                RecommendService svc(*h);
                auto             items = svc.recommend(tagIds, timeMin, limit);

                crow::json::wvalue::list arr;
                for (auto& it : items) {
                    crow::json::wvalue o;
                    o["id"]            = it.id;
                    o["description"]   = it.description;
                    o["suggestedTime"] = it.suggestedTime;
                    o["tagFit"]        = it.tagFit;
                    o["timeFit"]       = it.timeFit;
                    o["finalScore"]    = it.finalScore;
                    arr.push_back(std::move(o));
                }

                crow::json::wvalue res;
                res["tasks"] = std::move(arr);
                return crow::response{200, res};
            }
            catch (const std::exception& e) {
                crow::json::wvalue err;
                err["error"] = e.what();
                err["hint"] =
                    "If this persists, contact support with the request payload.";
                return crow::response{500, err};
            }
        });
}
