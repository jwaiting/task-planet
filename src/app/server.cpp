#include "server.hpp"
#include "crow_all.h"

#include "../config/config.hpp"
#include "../db/prepared.hpp"

#include "../app/routes.hpp"

#include "middleware.hpp"

namespace app {

    Server::Server() {
        // 1) 環境變數
        std::string connStr = Config::getDbConnStr();
        std::string schema  = Config::getDbSchema();
        if (schema.empty())
            schema = "public";

        // 2) 初始化 DbPool
        pool_ = std::make_shared<DbPool>(connStr, 8, [schema](pqxx::connection& c) {
            pqxx::work w(c);
            w.exec("SET search_path TO " + schema + ", public");
            w.commit();
            register_prepared(c);
        });

        // 3) 健康檢查 掛上 API routes
        register_routes(app_, *pool_);
    }

    int Server::run(uint16_t port) {
        std::string schema = Config::getDbSchema();
        if (schema.empty())
            schema = "public";
        std::cout << "[INFO] Server listening on :" << port << " (schema=" << schema
                  << ")\n";
        app_.port(port).multithreaded().run();
        return 0;
    }

} // namespace app
