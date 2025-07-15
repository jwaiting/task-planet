#include "crow_all.h"

int main()
{
    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        return "Hello, Task Planet!";
    });

    CROW_ROUTE(app, "/api/today")([](){
        crow::json::wvalue x;
        x["tasks"] = crow::json::wvalue::list({"task1", "task2"});
        return x;
    });

    app.port(18080).multithreaded().run();
    return 0;
}
