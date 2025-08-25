#pragma once
#include "crow_all.h"
#include <jwt-cpp/jwt.h>
#include <unordered_set>
#include <string>
#include "../config/config.hpp"

// ---- 既有：CORS ----
struct Cors
{
    struct context
    {
    };
    void before_handle(crow::request& req, crow::response& res, context&) {
        res.add_header("Access-Control-Allow-Origin", "*");
        res.add_header("Access-Control-Allow-Headers",
                       "Content-Type, Authorization");
        res.add_header("Access-Control-Allow-Methods",
                       "GET,POST,PUT,PATCH,DELETE,OPTIONS");
        // 預檢請求直接通過（避免卡在後續驗證）
        if (req.method == crow::HTTPMethod::Options) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response&, context&) {}
};

// ---- 新增：JWT ----
struct JwtContext
{
    bool        authenticated = false;
    std::string sub;
    std::string role; // guest | user | admin
};

class JwtMiddleware
{
   public:
    struct context
    {
        JwtContext jwt;
    };

    JwtMiddleware()
        : secret_(Config::getEnvOrThrow("AUTH_JWT_SECRET")),
          issuer_(Config::getEnvOrDefault("AUTH_JWT_ISSUER", "taskplanet")) {
        // 白名單（不需帶 token）
        whitelist_ = {
            "/ping", "/api/suggest", "/api/tags", "/api/suggestions/buffer"};
#ifdef TP_ENABLE_DEV_LOGIN
        whitelist_.insert("/api/auth/dev-login");
#endif
    }

    void before_handle(crow::request& req, crow::response& res, context& ctx) {
        // CORS 預檢已在 Cors middleware 處理完畢，這裡再保險放行
        if (req.method == crow::HTTPMethod::Options)
            return;

        if (isWhitelisted(req.url))
            return;

        auto auth = req.get_header_value("Authorization");
        if (auth.size() < 8 || auth.rfind("Bearer ", 0) != 0) {
            unauthorized(res, "missing_or_invalid_authorization_header");
            return;
        }

        const std::string token = auth.substr(7);
        try {
            auto decoded  = jwt::decode(token);
            auto verifier = jwt::verify()
                                .allow_algorithm(jwt::algorithm::hs256{secret_})
                                .with_issuer(issuer_);
            verifier.verify(decoded);

            auto get_claim_str = [&](const char* k) -> std::string {
                if (!decoded.has_payload_claim(k))
                    return {};
                return decoded.get_payload_claim(k).as_string();
            };

            ctx.jwt.authenticated = true;
            ctx.jwt.sub           = get_claim_str("sub");
            ctx.jwt.role          = get_claim_str("role");
            if (ctx.jwt.role.empty())
                ctx.jwt.role = "user";
        }
        catch (const std::exception& e) {
            unauthorized(res, std::string("invalid_token: ") + e.what());
        }
    }

    void after_handle(crow::request&, crow::response&, context&) {}

    // handler 端可用的小幫手
    static bool requiresRoleOr403(const JwtContext&  jwt,
                                  const std::string& need,
                                  crow::response&    res) {
        if (!jwt.authenticated) {
            unauthorized(res, "unauthenticated");
            return false;
        }
        auto rank = [](const std::string& r) -> int {
            if (r == "guest")
                return 0;
            if (r == "user")
                return 1;
            if (r == "admin")
                return 2;
            return -1;
        };
        if (rank(jwt.role) < rank(need)) {
            res.code = 403;
            res.set_header("Content-Type", "application/json");
            res.write(R"({"error":"forbidden"})");
            res.end();
            return false;
        }
        return true;
    }

   private:
    static void unauthorized(crow::response& res, const std::string& msg) {
        res.code = 401;
        res.set_header("Content-Type", "application/json");
        res.write(std::string(R"({"error":")") + msg + R"("})");
        res.end();
    }

    bool isWhitelisted(const std::string& path) const {
        return whitelist_.count(path) > 0;
    }

    std::string                     secret_;
    std::string                     issuer_;
    std::unordered_set<std::string> whitelist_;
};
