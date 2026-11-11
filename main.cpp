#include "server.h"
#include <iostream>
#include <csignal>
#include <charconv>
#include <string_view>

Server* g_server = nullptr;

void signal_handler(int sig) {
    std::cout << "\nShutting down MiniRedis...\n";
    if (g_server) g_server->stop();
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 6379;
    if (argc > 2) {
        std::cerr << "Usage: " << argv[0] << " [port (1-65535)]\n";
        return 1;
    }
    if (argc == 2) {
        std::string_view input(argv[1]);
        int parsed = 0;
        const auto [end, error] = std::from_chars(input.data(), input.data() + input.size(), parsed);
        if (error != std::errc{} || end != input.data() + input.size() ||
            parsed < 1 || parsed > 65535) {
            std::cerr << "Invalid port: expected an integer from 1 to 65535\n";
            return 1;
        }
        port = parsed;
    }

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    Server server(port);
    g_server = &server;

    std::cout << "MiniRedis listening on port " << port << "...\n";
    server.run();

    return 0;
}
