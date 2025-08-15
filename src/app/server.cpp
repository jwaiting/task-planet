#include "crow_all.h"
#include "../config/config.hpp"
#include "../db/pool.hpp"
#include "../db/prepared.hpp"
#include "../controllers/suggest_controller.hpp"
#include "../controllers/suggestions_controller.hpp"
#include "../controllers/events_controller.hpp"
#include "../controllers/tags_controller.hpp"
#include "middleware.hpp"

static bool valid_schema_ident(const std::string& s) {
    if (s.empty())
        return false;
    for (unsigned char ch : s) {
        if (!(std::isalnum(ch) || ch == '_'))
            return false;
    }
    return true;
}

int main() {
    try {
        // 讀 .env
        Config::loadEnv();

        // 取 DB 連線字串與 schema
        std::string connStr = Config::getDbConnStr();
        std::string schema  = Config::getDbSchema();
        if (!valid_schema_ident(schema)) {
            std::cerr << "[WARN] Invalid DB_SCHEMA: " << schema
                      << ", fallback to 'public'\n";
            schema = "public";
        }

        // 建立連線池：先設 search_path，再註冊 prepared statements
        DbPool pool(connStr, /*size*/ 8, [schema](pqxx::connection& c) {
            pqxx::work w(c);
            w.exec("SET search_path TO " + schema + ", public");
            w.commit();
            register_prepared(c);
        });

        // Crow App（含 CORS）
        crow::App<Cors> app;

        // 健康檢查
        CROW_ROUTE(app, "/").methods("GET"_method)(
            [] { return crow::response{200, "ok"}; });
        CROW_ROUTE(app, "/ping").methods("GET"_method)([] {
            return crow::response{200, "pong"};
        });

        // 掛路由
        attach_suggest_routes(app, pool);
        attach_suggestions_routes(app, pool, /*simThreshold*/ 0.87);
        attach_events_routes(app, pool);
        attach_tags_routes(app, pool);

        auto port = Config::getPort();
        std::cout << "[INFO] Server listening on :" << port << " (schema=" << schema
                  << ")\n";
        app.port(port).multithreaded().run();
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
    return 0;
}
