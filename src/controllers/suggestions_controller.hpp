#pragma once
#include <crow_all.h>
#include <vector>
#include <string>
#include "../db/pool.hpp"
#include "../db/prepared.hpp"
#include "../repositories/tag_repo.hpp"
#include "../services/suggestion_service.hpp"

template <typename App>
inline void attach_suggestions_routes(App&    app,
                                      DbPool& pool,
                                      double  simThreshold = 0.87) {
    // POST /api/suggestions/buffer
    // body: { "description": "...", "suggestedTime": 15, "tags":[1,2],
    // "tagCodes":["context/desk", ...] }
    CROW_ROUTE(app, "/api/suggestions/buffer")
        .methods("POST"_method)([&pool, simThreshold](const crow::request& req) {
            auto j = crow::json::load(req.body);
            if (!j)
                return crow::response{400, "invalid json"};

            std::string desc =
                j.has("description") ? std::string(j["description"].s()) : "";
            int sugTime = j.has("suggestedTime") ? (int)j["suggestedTime"].i() : 10;

            std::vector<int> tagIds;
            if (j.has("tags") && j["tags"].t() == crow::json::type::List) {
                for (auto& v : j["tags"]) tagIds.push_back((int)v.i());
            }
            std::vector<std::string> tagCodes;
            if (j.has("tagCodes") && j["tagCodes"].t() == crow::json::type::List) {
                for (auto& v : j["tagCodes"])
                    tagCodes.emplace_back(std::string(v.s()));
            }
            if (desc.empty())
                return crow::response{400, "missing description"};

            try {
                auto h = pool.acquire();

                if (!tagCodes.empty()) {
                    TagRepo tr(*h);
                    auto    ids = tr.ids_by_codes(tagCodes);
                    tagIds.insert(tagIds.end(), ids.begin(), ids.end());
                }

                SuggestionService svc(*h, simThreshold);
                auto              res = svc.create_or_alias(desc, sugTime, tagIds);

                crow::json::wvalue out;
                out["merged"]       = res.merged;
                out["suggestionId"] = res.suggestionId;
                if (res.matchedTaskId)
                    out["matchedTaskId"] = *res.matchedTaskId;
                if (res.similarity)
                    out["similarity"] = *res.similarity;

                return crow::response{200, out};
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
