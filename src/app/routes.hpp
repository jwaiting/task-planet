// app/routes.hpp
#pragma once
#include "crow_all.h"
#include "middleware.hpp"
#include "../db/pool.hpp"

// 各 controller 的 attach_* 宣告
#include "../controllers/suggest_controller.hpp"
#include "../controllers/suggestions_controller.hpp"
#include "../controllers/events_controller.hpp"
#include "../controllers/tags_controller.hpp"

// 安全防呆：禁止在 Release 搭配 dev-login
#if defined(TP_ENABLE_DEV_LOGIN) && defined(NDEBUG)
#error "TP_ENABLE_DEV_LOGIN must NOT be enabled in Release builds."
#endif

#ifdef TP_ENABLE_DEV_LOGIN
#include <jwt-cpp/jwt.h>
#include "../config/config.hpp"
inline std::string tp_issue_dev_token(const std::string& sub,
                                      const std::string& role,
                                      int                expSec = 3600) {
    const auto secret = Config::getEnvOrThrow("AUTH_JWT_SECRET");
    const auto issuer = Config::getEnvOrDefault("AUTH_JWT_ISSUER", "taskplanet");
    auto       now    = std::chrono::system_clock::now();
    return jwt::create()
        .set_issuer(issuer)
        .set_subject(sub)
        .set_issued_at(now)
        .set_expires_at(now + std::chrono::seconds(expSec))
        .set_payload_claim("role", jwt::claim(role))
        .sign(jwt::algorithm::hs256{secret});
}
#endif

namespace app {

    inline void register_routes(App& app, DbPool& pool) {
        // 健康檢查
        CROW_ROUTE(app, "/")([] { return crow::response{200, "ok"}; });
        CROW_ROUTE(app, "/ping").methods(crow::HTTPMethod::GET)([] {
            return crow::response{200, "pong"};
        });

#ifdef TP_ENABLE_DEV_LOGIN
        // 僅在開發啟用的發 token 端點
        CROW_ROUTE(app, "/api/auth/dev-login")
            .methods(crow::HTTPMethod::POST)([](const crow::request&) {
                crow::json::wvalue out;
                out["token"] = tp_issue_dev_token("user-123", "user", 3600);
                return crow::response{200, out.dump()};
            });
#endif

        // 集中掛你原本分散在 controllers 裡的路由
        attach_suggest_routes(app, pool);
        attach_suggestions_routes(app, pool, 0.87);
        attach_events_routes(app, pool); // 這裡面會保護 /api/events/adopt
        attach_tags_routes(app, pool);
    }

} // namespace app
