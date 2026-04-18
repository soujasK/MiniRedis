#include "server.h"
#include <iostream>
#include <csignal>

Server* g_server = nullptr;

void signal_handler(int sig) {
    std::cout << "\nShutting down MiniRedis...\n";
    if (g_server) g_server->stop();
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 6379;
    if (argc > 1) port = std::stoi(argv[1]);

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    Server server(port);
    g_server = &server;

    std::cout << "MiniRedis listening on port " << port << "...\n";
    server.run();

    return 0;
}
