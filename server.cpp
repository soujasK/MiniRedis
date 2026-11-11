#include "server.h"
#include "resp.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <thread>
#include <chrono>

Server::Server(int port)
    : port_(port), store_(100000), handler_(store_) {}

Server::~Server() {
    stop();
}

void Server::stop() {
    running_ = false;
    if (server_fd_ >= 0) {
        close(server_fd_);
        server_fd_ = -1;
    }
    if (expiry_thread_.joinable()) expiry_thread_.join();
}

void Server::expiry_worker() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        store_.evict_expired();
    }
}

void Server::run() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) { perror("socket"); return; }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(server_fd_, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); return;
    }
    if (listen(server_fd_, 128) < 0) {
        perror("listen"); return;
    }

    running_ = true;
    expiry_thread_ = std::thread(&Server::expiry_worker, this);

    while (running_) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd_, (sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (running_) perror("accept");
            continue;
        }
        std::cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << "\n";
        std::thread([this, client_fd]() {
            handle_client(client_fd);
        }).detach();
    }
}

void Server::handle_client(int client_fd) {
    std::string buffer;
    char chunk[4096];

    while (running_) {
        ssize_t n = recv(client_fd, chunk, sizeof(chunk), 0);
        if (n <= 0) break;

        buffer.append(chunk, n);

        while (!buffer.empty()) {
            size_t consumed = 0;
            auto args = RESP::parse(buffer, consumed);
            if (!args) break; // need more data

            buffer.erase(0, consumed);

            if (!args->empty()) {
                std::string response = handler_.handle(*args);
                size_t sent = 0;
                while (sent < response.size()) {
                    ssize_t written = send(client_fd, response.data() + sent,
                                           response.size() - sent, MSG_NOSIGNAL);
                    if (written < 0 && errno == EINTR) continue;
                    if (written <= 0) {
                        close(client_fd);
                        return;
                    }
                    sent += static_cast<size_t>(written);
                }
            }
        }
    }

    std::cout << "Client disconnected\n";
    close(client_fd);
}
