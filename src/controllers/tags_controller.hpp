#pragma once
#include <crow_all.h>
#include "../db/pool.hpp"
#include "../repositories/tag_repo.hpp"

template <typename App>
inline void attach_tags_routes(App& app, DbPool& pool) {
    CROW_ROUTE(app, "/api/tags")
        .methods("GET"_method)([&pool](const crow::request&) {
            try {
                auto    h = pool.acquire(); // 連線池連線
                TagRepo tr(*h);
                auto    rows = tr.list_active();

                crow::json::wvalue::list arr;
                arr.reserve(rows.size());
                for (auto& t : rows) {
                    crow::json::wvalue o;
                    o["id"]    = t.id;
                    o["code"]  = t.code;
                    o["label"] = t.label;
                    o["group"] = t.group_code;
                    arr.push_back(std::move(o));
                }
                crow::json::wvalue res;
                res["tags"] = std::move(arr);
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
