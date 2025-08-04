#include "crow_all.h"
#include "config/config.hpp"
#include "models/task.hpp"
#include "utils/db_task.hpp"
#include "routes/roulette_routes.hpp"
#include "routes/suggestion_routes.hpp"
#include "routes/suggestion_buffer_routes.hpp"
#include "routes/task_routes.hpp"

int main() {
    crow::SimpleApp app;
    std::string     connStr;
    try {
        Config::loadEnv();
        connStr = Config::getDbConnStr();
        std::cout << "DB Connection: " << connStr << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << std::endl;
        return 1;
    }

    pqxx::connection conn(connStr);

    // 註冊路由
    registerTaskRoutes(app, conn);
    registerSuggestionRoutes(app, conn);
    registerRouletteRoutes(app, conn);
    registerSuggestionBufferRoutes(app, conn);
    CROW_ROUTE(app,
               "/")([]() { return "<h1>Mini Task List Alliance 啟動 🌱</h1>"; });

    app.port(18080).multithreaded().run();
}
