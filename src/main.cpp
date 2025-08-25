// main.cpp
#include "app/server.hpp" // 若你的 include 路徑已把 app/ 加進來，也可用 #include "server.hpp"
#include "config/config.hpp"
#include <iostream>

int main() {
    try {
        Config::loadEnv();
        app::Server server; // 內部會載入 .env 並初始化連線池
        return server.run(Config::getPort());
    }
    catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return 1;
    }
}
