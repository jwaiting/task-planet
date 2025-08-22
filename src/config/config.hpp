#pragma once
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <algorithm>
#include "dotenv.h"

class Config
{
   public:
    // ---- lifecycle ----
    static void loadEnv() {
        dotenv::init(); // 讀取 .env
    }

    // ---- DB ----
    static std::string getDbConnStr() {
        std::string dbname   = mustGet("DB_NAME");
        std::string user     = mustGet("DB_USER");
        std::string password = mustGet("DB_PASSWORD");
        std::string host     = mustGet("DB_HOST");
        std::string port     = mustGet("DB_PORT");
        // libpq DSN 格式：key=value 空白分隔
        return "dbname=" + dbname + " user=" + user + " password=" + password +
               " host=" + host + " port=" + port;
    }

    static std::string getDbSchema() { return getOr("DB_SCHEMA", "public"); }

    // 供連線建立後設定 search_path 用（避免四處散落）
    static std::string getSearchPathSQL() {
        // 允許自訂 schema，但總是把 public 放在最後備援
        return "SET search_path TO " + getDbSchema() + ", public";
    }

    // ---- Server ----
    static unsigned short getPort() {
        return static_cast<unsigned short>(getInt("PORT", 8080));
    }

    // ---- Auth (JWT) ----
    static std::string jwtSecret() {
        // dev 可用簡單字串，prod 要用 openssl 產的強隨機字串
        return mustGet("AUTH_JWT_SECRET");
    }
    static std::string jwtIssuer() { return getOr("AUTH_ISS", "taskplanet-api"); }
    static std::string jwtAudience() { return getOr("AUTH_AUD", "taskplanet-web"); }

    // ---- CORS（預留，可選）----
    // 例：ALLOW_ORIGINS=https://taskplanet.app,https://admin.taskplanet.app
    static std::vector<std::string> allowOrigins() {
        auto raw = getOr("ALLOW_ORIGINS", "*"); // "*" 代表全開（僅開發用）
        return splitCsv(raw);
    }

    // ---- Rate limit（預留，可選）----
    static int votePerMinPerToken() { return getInt("VOTE_PER_MIN_PER_TOKEN", 30); }
    static int votePerMinPerIP() { return getInt("VOTE_PER_MIN_PER_IP", 120); }
    static int suggestPerDayPerToken() {
        return getInt("SUGGEST_PER_DAY_PER_TOKEN", 20);
    }
    static int guestIssuePerMinPerIP() {
        return getInt("GUEST_ISSUE_PER_MIN_PER_IP", 10);
    }

   private:
    // ---- helpers ----
    static std::string mustGet(const char* key) {
        const char* v = std::getenv(key);
        if (!v || !*v)
            throw std::runtime_error(
                std::string("Missing required environment variable: ") + key);
        return std::string(v);
    }

    static std::string getOr(const char* key, const char* def) {
        const char* v = std::getenv(key);
        return (v && *v) ? std::string(v) : std::string(def);
    }

    static int getInt(const char* key, int def) {
        const char* v = std::getenv(key);
        if (!v || !*v)
            return def;
        try {
            return std::stoi(v);
        }
        catch (...) {
            return def;
        }
    }

    static bool getBool(const char* key, bool def) {
        const char* v = std::getenv(key);
        if (!v || !*v)
            return def;
        std::string s = toLower(v);
        return (s == "1" || s == "true" || s == "yes" || s == "on");
    }

    static std::vector<std::string> splitCsv(const std::string& csv) {
        std::vector<std::string> out;
        std::stringstream        ss(csv);
        std::string              item;
        while (std::getline(ss, item, ',')) {
            trim(item);
            if (!item.empty())
                out.push_back(item);
        }
        if (out.empty())
            out.push_back("*");
        return out;
    }

    static std::string toLower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
        return s;
    }

    static void trim(std::string& s) {
        auto notSpace = [](unsigned char c) { return !std::isspace(c); };
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), notSpace));
        s.erase(std::find_if(s.rbegin(), s.rend(), notSpace).base(), s.end());
    }
};
