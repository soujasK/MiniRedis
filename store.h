#pragma once
#include <string>
#include <unordered_map>
#include <list>
#include <optional>
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <variant>

using TimePoint = std::chrono::steady_clock::time_point;

struct Entry {
    std::string value;
    std::optional<TimePoint> expires_at;
};

class Store {
public:
    explicit Store(size_t max_keys = 10000);

    // Core commands
    void set(const std::string& key, const std::string& value,
             std::optional<int> ttl_seconds = std::nullopt);
    std::optional<std::string> get(const std::string& key);
    bool del(const std::string& key);
    bool exists(const std::string& key);
    bool expire(const std::string& key, int seconds);
    std::optional<int> ttl(const std::string& key);

    // Numeric commands
    std::optional<long long> incr(const std::string& key);
    std::optional<long long> decr(const std::string& key);
    std::optional<long long> incrby(const std::string& key, long long delta);

    // List commands
    int lpush(const std::string& key, const std::vector<std::string>& values);
    int rpush(const std::string& key, const std::vector<std::string>& values);
    std::optional<std::string> lpop(const std::string& key);
    std::optional<std::string> rpop(const std::string& key);
    std::vector<std::string> lrange(const std::string& key, int start, int stop);
    int llen(const std::string& key);

    // Utility
    std::vector<std::string> keys(const std::string& pattern = "*");
    void flushall();
    size_t dbsize();
    void evict_expired();

private:
    // String store
    std::unordered_map<std::string, Entry> string_store_;

    // List store
    std::unordered_map<std::string, std::list<std::string>> list_store_;

    // LRU tracking
    std::list<std::string> lru_order_;
    std::unordered_map<std::string, std::list<std::string>::iterator> lru_map_;

    mutable std::shared_mutex mutex_;
    size_t max_keys_;

    void touch_lru(const std::string& key);
    void evict_lru();
    bool is_expired(const Entry& entry) const;
    bool pattern_match(const std::string& pattern, const std::string& key) const;
};
