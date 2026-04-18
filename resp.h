#pragma once
#include <string>
#include <vector>
#include <optional>

// Redis Serialization Protocol (RESP) parser & serializer
class RESP {
public:
    // Parse incoming RESP data into command + args
    // Returns nullopt if data is incomplete
    static std::optional<std::vector<std::string>> parse(const std::string& data, size_t& consumed);

    // Serialize responses back to client
    static std::string simple_string(const std::string& s);
    static std::string error(const std::string& msg);
    static std::string integer(long long n);
    static std::string bulk_string(const std::string& s);
    static std::string null_bulk_string();
    static std::string array(const std::vector<std::string>& items);
    static std::string null_array();

private:
    static std::optional<std::pair<std::string, size_t>> parse_line(const std::string& data, size_t pos);
};
