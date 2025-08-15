#pragma once
#include "crow_all.h"

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
        if (req.method == crow::HTTPMethod::Options) {
            res.code = 204;
            res.end();
        }
    }
    void after_handle(crow::request&, crow::response&, context&) {}
};
