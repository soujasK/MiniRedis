#include "resp.h"
#include <sstream>
#include <stdexcept>

std::optional<std::pair<std::string, size_t>>
RESP::parse_line(const std::string& data, size_t pos) {
    auto end = data.find("\r\n", pos);
    if (end == std::string::npos) return std::nullopt;
    return std::make_pair(data.substr(pos, end - pos), end + 2);
}

std::optional<std::vector<std::string>>
RESP::parse(const std::string& data, size_t& consumed) {
    if (data.empty()) return std::nullopt;

    size_t pos = 0;

    // Inline command (non-RESP, e.g. typed by hand)
    if (data[0] != '*') {
        auto end = data.find("\r\n");
        if (end == std::string::npos) return std::nullopt;
        std::vector<std::string> parts;
        std::istringstream ss(data.substr(0, end));
        std::string token;
        while (ss >> token) parts.push_back(token);
        consumed = end + 2;
        return parts;
    }

    // RESP Array: *<count>\r\n
    auto line = parse_line(data, pos + 1);
    if (!line) return std::nullopt;
    int count = std::stoi(line->first);
    pos = line->second;

    std::vector<std::string> args;
    for (int i = 0; i < count; ++i) {
        if (pos >= data.size()) return std::nullopt;
        if (data[pos] != '$') return std::nullopt;

        auto len_line = parse_line(data, pos + 1);
        if (!len_line) return std::nullopt;
        int len = std::stoi(len_line->first);
        pos = len_line->second;

        if (pos + len + 2 > data.size()) return std::nullopt;
        args.push_back(data.substr(pos, len));
        pos += len + 2; // skip \r\n
    }

    consumed = pos;
    return args;
}

// ── Serializers ───────────────────────────────────────────────────────────────

std::string RESP::simple_string(const std::string& s) {
    return "+" + s + "\r\n";
}

std::string RESP::error(const std::string& msg) {
    return "-ERR " + msg + "\r\n";
}

std::string RESP::integer(long long n) {
    return ":" + std::to_string(n) + "\r\n";
}

std::string RESP::bulk_string(const std::string& s) {
    return "$" + std::to_string(s.size()) + "\r\n" + s + "\r\n";
}

std::string RESP::null_bulk_string() {
    return "$-1\r\n";
}

std::string RESP::null_array() {
    return "*-1\r\n";
}

std::string RESP::array(const std::vector<std::string>& items) {
    std::string out = "*" + std::to_string(items.size()) + "\r\n";
    for (const auto& item : items) out += bulk_string(item);
    return out;
}
