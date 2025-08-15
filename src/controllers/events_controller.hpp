#pragma once
#include <crow_all.h>
#include <vector>
#include <string>
#include "../db/pool.hpp"
#include "../db/prepared.hpp"
#include "../repositories/tag_repo.hpp"
#include "../services/event_service.hpp"

template <typename App>
inline void attach_events_routes(App& app, DbPool& pool) {
    // POST /api/events  { "taskId":123, "event":"adopt"|"skip"|"impression",
    // "tags":[1,2], "tagCodes":[...] }
    CROW_ROUTE(app, "/api/events")
        .methods("POST"_method)([&pool](const crow::request& req) {
            auto j = crow::json::load(req.body);
            if (!j)
                return crow::response{400, "invalid json"};

            int         taskId = j.has("taskId") ? (int)j["taskId"].i() : 0;
            std::string ev     = j.has("event") ? std::string(j["event"].s()) : "";

            std::vector<int> tagIds;
            if (j.has("tags") && j["tags"].t() == crow::json::type::List) {
                for (auto& v : j["tags"]) tagIds.push_back((int)v.i());
            }
            std::vector<std::string> tagCodes;
            if (j.has("tagCodes") && j["tagCodes"].t() == crow::json::type::List) {
                for (auto& v : j["tagCodes"])
                    tagCodes.emplace_back(std::string(v.s()));
            }
            if (!taskId || ev.empty())
                return crow::response{400, "missing taskId or event"};

            try {
                auto h = pool.acquire();

                if (!tagCodes.empty()) {
                    TagRepo tr(*h);
                    auto    ids = tr.ids_by_codes(tagCodes);
                    tagIds.insert(tagIds.end(), ids.begin(), ids.end());
                }

                EventService svc(*h);
                svc.handle_event(taskId, ev, tagIds);

                return crow::response{200, "ok"};
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
