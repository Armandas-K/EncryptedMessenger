#ifndef ENCRYPTEDMESSENGER_CLIENTTESTCONTEXT_H
#define ENCRYPTEDMESSENGER_CLIENTTESTCONTEXT_H

#include <asio.hpp>
#include <thread>

class ClientTestContext {
public:
    ClientTestContext() : io_thread_([this]() { io_.run(); }) {}

    ~ClientTestContext() {
        io_.stop();
        if (io_thread_.joinable())
            io_thread_.join();
    }

    asio::io_context& io() { return io_; }

private:
    asio::io_context io_;
    std::thread io_thread_;
};

#endif