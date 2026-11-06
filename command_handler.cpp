#include "command_handler.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>

namespace {
template <typename T>
std::optional<T> parse_integer(const std::string& input) {
    T value{};
    auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), value);
    if (error != std::errc{} || end != input.data() + input.size()) return std::nullopt;
    return value;
}
}

CommandHandler::CommandHandler(Store& store) : store_(store) {}

std::string CommandHandler::handle(const std::vector<std::string>& args) {
    if (args.empty()) return RESP::error("empty command");

    std::string cmd = args[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);

    if (cmd == "PING")    return cmd_ping(args);
    if (cmd == "SET")     return cmd_set(args);
    if (cmd == "GET")     return cmd_get(args);
    if (cmd == "DEL")     return cmd_del(args);
    if (cmd == "EXISTS")  return cmd_exists(args);
    if (cmd == "EXPIRE")  return cmd_expire(args);
    if (cmd == "TTL")     return cmd_ttl(args);
    if (cmd == "INCR")    return cmd_incr(args);
    if (cmd == "DECR")    return cmd_decr(args);
    if (cmd == "INCRBY")  return cmd_incrby(args);
    if (cmd == "LPUSH")   return cmd_lpush(args);
    if (cmd == "RPUSH")   return cmd_rpush(args);
    if (cmd == "LPOP")    return cmd_lpop(args);
    if (cmd == "RPOP")    return cmd_rpop(args);
    if (cmd == "LRANGE")  return cmd_lrange(args);
    if (cmd == "LLEN")    return cmd_llen(args);
    if (cmd == "KEYS")    return cmd_keys(args);
    if (cmd == "FLUSHALL") return cmd_flushall(args);
    if (cmd == "DBSIZE")  return cmd_dbsize(args);

    return RESP::error("unknown command '" + args[0] + "'");
}

std::string CommandHandler::cmd_ping(const std::vector<std::string>& args) {
    if (args.size() > 1) return RESP::bulk_string(args[1]);
    return RESP::simple_string("PONG");
}

std::string CommandHandler::cmd_set(const std::vector<std::string>& args) {
    if (args.size() < 3) return RESP::error("SET requires key and value");
    std::optional<int> ttl;
    // Support: SET key value EX seconds
    for (size_t i = 3; i < args.size(); ++i) {
        std::string opt = args[i];
        std::transform(opt.begin(), opt.end(), opt.begin(), ::toupper);
        if (opt == "EX" && i + 1 < args.size()) {
            auto seconds = parse_integer<int>(args[++i]);
            if (!seconds || *seconds <= 0) return RESP::error("invalid expire time in SET");
            ttl = *seconds;
        }
    }
    store_.set(args[1], args[2], ttl);
    return RESP::simple_string("OK");
}

std::string CommandHandler::cmd_get(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("GET requires key");
    auto val = store_.get(args[1]);
    if (!val) return RESP::null_bulk_string();
    return RESP::bulk_string(*val);
}

std::string CommandHandler::cmd_del(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("DEL requires key");
    int count = 0;
    for (size_t i = 1; i < args.size(); ++i)
        if (store_.del(args[i])) ++count;
    return RESP::integer(count);
}

std::string CommandHandler::cmd_exists(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("EXISTS requires key");
    return RESP::integer(store_.exists(args[1]) ? 1 : 0);
}

std::string CommandHandler::cmd_expire(const std::vector<std::string>& args) {
    if (args.size() < 3) return RESP::error("EXPIRE requires key and seconds");
    auto seconds = parse_integer<int>(args[2]);
    if (!seconds) return RESP::error("seconds is not an integer");
    bool ok = store_.expire(args[1], *seconds);
    return RESP::integer(ok ? 1 : 0);
}

std::string CommandHandler::cmd_ttl(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("TTL requires key");
    auto t = store_.ttl(args[1]);
    if (!t) return RESP::integer(-2); // key doesn't exist
    return RESP::integer(*t);
}

std::string CommandHandler::cmd_incr(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("INCR requires key");
    auto val = store_.incr(args[1]);
    if (!val) return RESP::error("value is not an integer");
    return RESP::integer(*val);
}

std::string CommandHandler::cmd_decr(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("DECR requires key");
    auto val = store_.decr(args[1]);
    if (!val) return RESP::error("value is not an integer");
    return RESP::integer(*val);
}

std::string CommandHandler::cmd_incrby(const std::vector<std::string>& args) {
    if (args.size() < 3) return RESP::error("INCRBY requires key and delta");
    auto delta = parse_integer<long long>(args[2]);
    if (!delta) return RESP::error("delta is not an integer");
    auto val = store_.incrby(args[1], *delta);
    if (!val) return RESP::error("value is not an integer");
    return RESP::integer(*val);
}

std::string CommandHandler::cmd_lpush(const std::vector<std::string>& args) {
    if (args.size() < 3) return RESP::error("LPUSH requires key and value(s)");
    std::vector<std::string> values(args.begin() + 2, args.end());
    return RESP::integer(store_.lpush(args[1], values));
}

std::string CommandHandler::cmd_rpush(const std::vector<std::string>& args) {
    if (args.size() < 3) return RESP::error("RPUSH requires key and value(s)");
    std::vector<std::string> values(args.begin() + 2, args.end());
    return RESP::integer(store_.rpush(args[1], values));
}

std::string CommandHandler::cmd_lpop(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("LPOP requires key");
    auto val = store_.lpop(args[1]);
    if (!val) return RESP::null_bulk_string();
    return RESP::bulk_string(*val);
}

std::string CommandHandler::cmd_rpop(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("RPOP requires key");
    auto val = store_.rpop(args[1]);
    if (!val) return RESP::null_bulk_string();
    return RESP::bulk_string(*val);
}

std::string CommandHandler::cmd_lrange(const std::vector<std::string>& args) {
    if (args.size() < 4) return RESP::error("LRANGE requires key start stop");
    auto start = parse_integer<int>(args[2]);
    auto stop = parse_integer<int>(args[3]);
    if (!start || !stop) return RESP::error("range bounds must be integers");
    auto items = store_.lrange(args[1], *start, *stop);
    return RESP::array(items);
}

std::string CommandHandler::cmd_llen(const std::vector<std::string>& args) {
    if (args.size() < 2) return RESP::error("LLEN requires key");
    return RESP::integer(store_.llen(args[1]));
}

std::string CommandHandler::cmd_keys(const std::vector<std::string>& args) {
    std::string pattern = args.size() > 1 ? args[1] : "*";
    auto ks = store_.keys(pattern);
    return RESP::array(ks);
}

std::string CommandHandler::cmd_flushall(const std::vector<std::string>&) {
    store_.flushall();
    return RESP::simple_string("OK");
}

std::string CommandHandler::cmd_dbsize(const std::vector<std::string>&) {
    return RESP::integer(static_cast<long long>(store_.dbsize()));
}
