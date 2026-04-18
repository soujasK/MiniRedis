#pragma once
#include "store.h"
#include "resp.h"
#include <string>
#include <vector>

class CommandHandler {
public:
    explicit CommandHandler(Store& store);
    std::string handle(const std::vector<std::string>& args);

private:
    Store& store_;

    std::string cmd_set(const std::vector<std::string>& args);
    std::string cmd_get(const std::vector<std::string>& args);
    std::string cmd_del(const std::vector<std::string>& args);
    std::string cmd_exists(const std::vector<std::string>& args);
    std::string cmd_expire(const std::vector<std::string>& args);
    std::string cmd_ttl(const std::vector<std::string>& args);
    std::string cmd_incr(const std::vector<std::string>& args);
    std::string cmd_decr(const std::vector<std::string>& args);
    std::string cmd_incrby(const std::vector<std::string>& args);
    std::string cmd_lpush(const std::vector<std::string>& args);
    std::string cmd_rpush(const std::vector<std::string>& args);
    std::string cmd_lpop(const std::vector<std::string>& args);
    std::string cmd_rpop(const std::vector<std::string>& args);
    std::string cmd_lrange(const std::vector<std::string>& args);
    std::string cmd_llen(const std::vector<std::string>& args);
    std::string cmd_keys(const std::vector<std::string>& args);
    std::string cmd_flushall(const std::vector<std::string>& args);
    std::string cmd_dbsize(const std::vector<std::string>& args);
    std::string cmd_ping(const std::vector<std::string>& args);
};
