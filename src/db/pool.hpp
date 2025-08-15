#pragma once
#include <pqxx/pqxx>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <string>
#include <utility>

/// Simple thread-safe PostgreSQL connection pool (pqxx)
/// - Construct with connStr and pool size
/// - Optional initializer(conn) runs once per new connection (e.g., register
/// prepared)
/// - Acquire returns a RAII handle; on destruction it returns the connection to the
/// pool
/// - try_acquire(timeout) to avoid indefinite blocking
class DbPool
{
   public:
    using Initializer = std::function<void(pqxx::connection&)>;

    DbPool(std::string connStr, std::size_t size = 8, Initializer init = nullptr)
        : connStr_(std::move(connStr)), init_(std::move(init)), shutdown_(false) {
        if (size == 0)
            size = 1;
        for (std::size_t i = 0; i < size; ++i) {
            pool_.push(make_connection());
        }
        size_ = size;
    }

    DbPool(const DbPool&)            = delete;
    DbPool& operator=(const DbPool&) = delete;

    ~DbPool() {
        std::lock_guard<std::mutex> lk(m_);
        shutdown_ = true;
        while (!pool_.empty())
            pool_.pop(); // unique_ptr destructors close connections
        // no notify here; destructor is called at program end or when no threads
        // should be waiting
    }

    /// RAII handle returned by acquire/try_acquire
    class Handle
    {
       public:
        Handle() = default;
        Handle(DbPool* pool, std::unique_ptr<pqxx::connection> c)
            : pool_(pool), conn_(std::move(c)) {}

        Handle(Handle&& other) noexcept { *this = std::move(other); }
        Handle& operator=(Handle&& other) noexcept {
            if (this != &other) {
                release();
                pool_       = other.pool_;
                conn_       = std::move(other.conn_);
                other.pool_ = nullptr;
            }
            return *this;
        }

        Handle(const Handle&)            = delete;
        Handle& operator=(const Handle&) = delete;

        ~Handle() { release(); }

        pqxx::connection& operator*() { return *conn_; }
        pqxx::connection* operator->() { return conn_.get(); }
        pqxx::connection* get() const { return conn_.get(); }
        explicit          operator bool() const { return static_cast<bool>(conn_); }

        /// Manually return the connection to the pool (optional)
        void release() {
            if (pool_ && conn_) {
                pool_->return_to_pool(std::move(conn_));
                pool_ = nullptr;
            }
        }

       private:
        DbPool*                           pool_ = nullptr;
        std::unique_ptr<pqxx::connection> conn_{};
    };

    /// Block until a connection is available; throws on shutdown
    Handle acquire() {
        std::unique_ptr<pqxx::connection> c;
        {
            std::unique_lock<std::mutex> lk(m_);
            cv_.wait(lk, [&] { return shutdown_ || !pool_.empty(); });
            if (shutdown_)
                throw std::runtime_error("DbPool shutdown");
            c = std::move(pool_.front());
            pool_.pop();
        }
        ensure_alive(*c);
        return Handle(this, std::move(c));
    }

    /// Try to acquire within timeout; returns empty handle on timeout
    template <typename Rep, typename Period>
    Handle try_acquire(const std::chrono::duration<Rep, Period>& timeout) {
        std::unique_ptr<pqxx::connection> c;
        {
            std::unique_lock<std::mutex> lk(m_);
            if (!cv_.wait_for(
                    lk, timeout, [&] { return shutdown_ || !pool_.empty(); })) {
                return {}; // timeout
            }
            if (shutdown_)
                return {};
            c = std::move(pool_.front());
            pool_.pop();
        }
        ensure_alive(*c);
        return Handle(this, std::move(c));
    }

    std::size_t size() const { return size_; }

   private:
    std::string connStr_;
    Initializer init_;
    std::size_t size_{0};

    mutable std::mutex                            m_;
    std::condition_variable                       cv_;
    std::queue<std::unique_ptr<pqxx::connection>> pool_;
    bool                                          shutdown_;

    std::unique_ptr<pqxx::connection> make_connection() {
        auto c = std::make_unique<pqxx::connection>(connStr_);
        c->set_client_encoding("UTF8");
        // Health check
        {
            pqxx::work w(*c);
            w.exec("SELECT 1");
            w.commit();
        }
        if (init_)
            init_(*c);
        return c;
    }

    void ensure_alive(pqxx::connection& c) {
        // If connection is closed/broken, recreate it.
        if (!c.is_open()) {
            auto nc = make_connection();
            // Replace object by moving new connection into place.
            c = std::move(*nc);
            return;
        }
        // Lightweight ping; recreate if it fails
        try {
            pqxx::work w(c);
            w.exec("SELECT 1");
            w.commit();
        }
        catch (...) {
            auto nc = make_connection();
            c       = std::move(*nc);
        }
    }

    void return_to_pool(std::unique_ptr<pqxx::connection> c) {
        if (!c)
            return;
        std::lock_guard<std::mutex> lk(m_);
        if (shutdown_)
            return; // dropping on shutdown
        pool_.push(std::move(c));
        cv_.notify_one();
    }
};
