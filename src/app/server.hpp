#pragma once
#include "crow_all.h"
#include "middleware.hpp"
#include "../db/pool.hpp"

namespace app {

    using App = crow::App<Cors, JwtMiddleware>;

    class Server
    {
       public:
        Server();
        int                     run(uint16_t port);
        App&                    app() { return app_; }
        std::shared_ptr<DbPool> pool() { return pool_; }

       private:
        App                     app_;
        std::shared_ptr<DbPool> pool_;
    };

} // namespace app
