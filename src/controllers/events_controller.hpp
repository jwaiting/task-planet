#pragma once
#include <crow_all.h>
#include <vector>
#include <string>
#include "../db/pool.hpp"
#include "../db/prepared.hpp"
#include "../repositories/tag_repo.hpp"
#include "../services/event_service.hpp"
#include "../app/middleware.hpp" // << 新增：拿 JwtMiddleware context

template <typename App>
inline void attach_events_routes(App& app, DbPool& pool) {
    // POST /api/events
    // Body: { "taskId":123, "event":"adopt"|"skip"|"impression", "tags":[1,2],
    // "tagCodes":[...] }
    CROW_ROUTE(app, "/api/events")
        .methods("POST"_method)([&app, &pool](const crow::request& req) {
            // --- JWT 保護：需要 user+
            crow::response authRes;
            auto&          ctx = app.template get_context<JwtMiddleware>(req);

            if (!JwtMiddleware::requiresRoleOr403(ctx.jwt, "user", authRes)) {
                return authRes; // 401/403 已在 helper 內處理
            }
            const std::string userId = ctx.jwt.sub; // 可用於審計或風控

            auto j = crow::json::load(req.body);
            if (!j)
                return crow::response{400, "invalid json"};

            int taskId     = j.has("taskId") ? static_cast<int>(j["taskId"].i()) : 0;
            std::string ev = j.has("event") ? std::string(j["event"].s()) : "";

            std::vector<int> tagIds;
            if (j.has("tags") && j["tags"].t() == crow::json::type::List) {
                for (auto& v : j["tags"]) tagIds.push_back(static_cast<int>(v.i()));
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

                // TODO: 若你要把 userId 寫入事件審計，將 handle_event 簽名改成：
                // svc.handle_event(taskId, ev, tagIds, userId);
                // 目前先沿用既有版本：
                svc.handle_event(taskId, ev, tagIds);

                // 回應
                crow::json::wvalue ok;
                ok["ok"]     = true;
                ok["userId"] = userId; // 方便前端/QA 確認是誰上報
                ok["event"]  = ev;
                ok["taskId"] = taskId;
                return crow::response{200, ok};
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
