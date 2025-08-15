#pragma once
#include <string>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "dotenv.h"

class Config
{
   public:
    static void loadEnv() {
        dotenv::init(); // 讀取 .env
    }

    static std::string getEnvVar(const char* key) {
        const char* value = std::getenv(key);
        if (!value) {
            throw std::runtime_error(
                std::string("Missing required environment variable: ") + key);
        }
        return std::string(value);
    }

    static std::string getEnvVarOptional(const char* key, const char* def = "") {
        const char* value = std::getenv(key);
        return std::string(value ? value : def);
    }

    static std::string getDbConnStr() {
        std::string dbname   = getEnvVar("DB_NAME");
        std::string user     = getEnvVar("DB_USER");
        std::string password = getEnvVar("DB_PASSWORD");
        std::string host     = getEnvVar("DB_HOST");
        std::string port     = getEnvVar("DB_PORT");
        // libpq DSN 格式：key=value 空白分隔
        return "dbname=" + dbname + " user=" + user + " password=" + password +
               " host=" + host + " port=" + port;
    }

    static std::string getDbSchema() {
        // 可選，預設 public
        return getEnvVarOptional("DB_SCHEMA", "public");
    }

    static unsigned short getPort() {
        std::string s = getEnvVarOptional("PORT", "8080");
        try {
            return static_cast<unsigned short>(std::stoi(s));
        }
        catch (...) {
            return 8080;
        }
    }
};
