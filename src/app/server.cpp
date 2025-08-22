#include "server.hpp"
#include "crow_all.h"
#include "../config/config.hpp"
#include "../db/pool.hpp"
#include "../db/prepared.hpp"
#include "../controllers/suggest_controller.hpp"
#include "../controllers/suggestions_controller.hpp"
#include "../controllers/events_controller.hpp"
#include "../controllers/tags_controller.hpp"
#include "middleware.hpp"

int run_server() {
    try {
        Config::loadEnv();
        std::string connStr = Config::getDbConnStr();
        std::string schema  = Config::getDbSchema();

        if (schema.empty())
            schema = "public";

        DbPool pool(connStr, 8, [schema](pqxx::connection& c) {
            pqxx::work w(c);
            w.exec("SET search_path TO " + schema + ", public");
            w.commit();
            register_prepared(c);
        });

        crow::App<Cors> app;

        CROW_ROUTE(app, "/")([] { return crow::response{200, "ok"}; });
        CROW_ROUTE(app, "/ping")([] { return crow::response{200, "pong"}; });

        attach_suggest_routes(app, pool);
        attach_suggestions_routes(app, pool, 0.87);
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
