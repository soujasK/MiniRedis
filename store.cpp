#include "store.h"
#include <stdexcept>
#include <algorithm>
#include <regex>

using namespace std::chrono;

Store::Store(size_t max_keys) : max_keys_(max_keys) {}

bool Store::is_expired(const Entry& entry) const {
    if (!entry.expires_at) return false;
    return steady_clock::now() > *entry.expires_at;
}

void Store::touch_lru(const std::string& key) {
    auto it = lru_map_.find(key);
    if (it != lru_map_.end()) {
        lru_order_.erase(it->second);
    }
    lru_order_.push_front(key);
    lru_map_[key] = lru_order_.begin();
}

void Store::evict_lru() {
    while (string_store_.size() >= max_keys_ && !lru_order_.empty()) {
        const auto& oldest = lru_order_.back();
        string_store_.erase(oldest);
        lru_map_.erase(oldest);
        lru_order_.pop_back();
    }
}

void Store::evict_expired() {
    std::unique_lock lock(mutex_);
    for (auto it = string_store_.begin(); it != string_store_.end();) {
        if (is_expired(it->second)) {
            lru_map_.erase(it->first);
            it = string_store_.erase(it);
        } else {
            ++it;
        }
    }
}

// ── String commands ──────────────────────────────────────────────────────────

void Store::set(const std::string& key, const std::string& value,
                std::optional<int> ttl_seconds) {
    std::unique_lock lock(mutex_);
    evict_lru();
    std::optional<TimePoint> exp;
    if (ttl_seconds) {
        exp = steady_clock::now() + seconds(*ttl_seconds);
    }
    string_store_[key] = {value, exp};
    touch_lru(key);
}

std::optional<std::string> Store::get(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = string_store_.find(key);
    if (it == string_store_.end()) return std::nullopt;
    if (is_expired(it->second)) {
        string_store_.erase(it);
        lru_map_.erase(key);
        return std::nullopt;
    }
    touch_lru(key);
    return it->second.value;
}

bool Store::del(const std::string& key) {
    std::unique_lock lock(mutex_);
    bool s = string_store_.erase(key) > 0;
    bool l = list_store_.erase(key) > 0;
    if (s || l) { lru_map_.erase(key); return true; }
    return false;
}

bool Store::exists(const std::string& key) {
    return get(key).has_value();
}

bool Store::expire(const std::string& key, int seconds_val) {
    std::unique_lock lock(mutex_);
    auto it = string_store_.find(key);
    if (it == string_store_.end() || is_expired(it->second)) return false;
    it->second.expires_at = steady_clock::now() + seconds(seconds_val);
    return true;
}

std::optional<int> Store::ttl(const std::string& key) {
    std::shared_lock lock(mutex_);
    auto it = string_store_.find(key);
    if (it == string_store_.end()) return std::nullopt;
    if (is_expired(it->second)) return std::nullopt;
    if (!it->second.expires_at) return -1; // no expiry
    auto remaining = duration_cast<std::chrono::seconds>(
        *it->second.expires_at - steady_clock::now()).count();
    return static_cast<int>(std::max(0LL, (long long)remaining));
}

// ── Numeric commands ─────────────────────────────────────────────────────────

std::optional<long long> Store::incr(const std::string& key) {
    return incrby(key, 1);
}

std::optional<long long> Store::decr(const std::string& key) {
    return incrby(key, -1);
}

std::optional<long long> Store::incrby(const std::string& key, long long delta) {
    std::unique_lock lock(mutex_);
    long long current = 0;
    auto it = string_store_.find(key);
    if (it != string_store_.end() && !is_expired(it->second)) {
        try { current = std::stoll(it->second.value); }
        catch (...) { return std::nullopt; } // not an integer
    }
    current += delta;
    string_store_[key] = {std::to_string(current), std::nullopt};
    touch_lru(key);
    return current;
}

// ── List commands ─────────────────────────────────────────────────────────────

int Store::lpush(const std::string& key, const std::vector<std::string>& values) {
    std::unique_lock lock(mutex_);
    auto& lst = list_store_[key];
    for (const auto& v : values) lst.push_front(v);
    touch_lru(key);
    return static_cast<int>(lst.size());
}

int Store::rpush(const std::string& key, const std::vector<std::string>& values) {
    std::unique_lock lock(mutex_);
    auto& lst = list_store_[key];
    for (const auto& v : values) lst.push_back(v);
    touch_lru(key);
    return static_cast<int>(lst.size());
}

std::optional<std::string> Store::lpop(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = list_store_.find(key);
    if (it == list_store_.end() || it->second.empty()) return std::nullopt;
    auto val = it->second.front();
    it->second.pop_front();
    if (it->second.empty()) list_store_.erase(it);
    return val;
}

std::optional<std::string> Store::rpop(const std::string& key) {
    std::unique_lock lock(mutex_);
    auto it = list_store_.find(key);
    if (it == list_store_.end() || it->second.empty()) return std::nullopt;
    auto val = it->second.back();
    it->second.pop_back();
    if (it->second.empty()) list_store_.erase(it);
    return val;
}

std::vector<std::string> Store::lrange(const std::string& key, int start, int stop) {
    std::shared_lock lock(mutex_);
    auto it = list_store_.find(key);
    if (it == list_store_.end()) return {};
    const auto& lst = it->second;
    int size = static_cast<int>(lst.size());
    if (start < 0) start = std::max(0, size + start);
    if (stop < 0) stop = size + stop;
    stop = std::min(stop, size - 1);
    if (start > stop) return {};
    std::vector<std::string> result;
    auto iter = lst.begin();
    std::advance(iter, start);
    for (int i = start; i <= stop; ++i, ++iter) result.push_back(*iter);
    return result;
}

int Store::llen(const std::string& key) {
    std::shared_lock lock(mutex_);
    auto it = list_store_.find(key);
    if (it == list_store_.end()) return 0;
    return static_cast<int>(it->second.size());
}

// ── Utility ───────────────────────────────────────────────────────────────────

bool Store::pattern_match(const std::string& pattern, const std::string& key) const {
    if (pattern == "*") return true;
    std::string regex_str;
    for (char c : pattern) {
        if (c == '*') regex_str += ".*";
        else if (c == '?') regex_str += ".";
        else {
            // Glob metacharacters are handled above; regex syntax in keys is literal.
            if (std::string(R"(.^$|()[]{}+\)").find(c) != std::string::npos)
                regex_str += '\\';
            regex_str += c;
        }
    }
    return std::regex_match(key, std::regex(regex_str));
}

std::vector<std::string> Store::keys(const std::string& pattern) {
    std::shared_lock lock(mutex_);
    std::vector<std::string> result;
    for (const auto& [k, v] : string_store_) {
        if (!is_expired(v) && pattern_match(pattern, k))
            result.push_back(k);
    }
    for (const auto& [k, _] : list_store_) {
        if (pattern_match(pattern, k)) result.push_back(k);
    }
    return result;
}

void Store::flushall() {
    std::unique_lock lock(mutex_);
    string_store_.clear();
    list_store_.clear();
    lru_order_.clear();
    lru_map_.clear();
}

size_t Store::dbsize() {
    std::shared_lock lock(mutex_);
    return string_store_.size() + list_store_.size();
}
