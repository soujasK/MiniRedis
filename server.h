#pragma once
#include "store.h"
#include "command_handler.h"
#include <atomic>
#include <thread>

class Server {
public:
    explicit Server(int port);
    ~Server();
    void run();
    void stop();

private:
    int port_;
    int server_fd_ = -1;
    std::atomic<bool> running_{false};
    Store store_;
    CommandHandler handler_;

    void handle_client(int client_fd);
    void expiry_worker();
    std::thread expiry_thread_;
};
