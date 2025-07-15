#include "crow_all.h"
#include "models/task.hpp"
#include "routes/roulette_routes.hpp"
#include "routes/suggestion_routes.hpp"
#include "routes/task_routes.hpp"

int main() {
    crow::SimpleApp app;

    // 初始任務池
    std::vector<Task> taskPool = {{"散步 15 分鐘", "放鬆", 15},
                                  {"閱讀 20 分鐘", "靜心", 20},
                                  {"整理房間", "積極", 30},
                                  {"泡杯咖啡", "放鬆", 10},
                                  {"寫下三件感恩小事", "靜心", 10}};

    // 註冊路由
    registerTaskRoutes(app, taskPool);
    registerSuggestionRoutes(app, taskPool);
    registerRouletteRoutes(app, taskPool);

    CROW_ROUTE(app,
               "/")([]() { return "<h1>Mini Task List Alliance 啟動 🌱</h1>"; });

    app.port(18080).multithreaded().run();
}
