#ifndef ENCRYPTEDMESSENGER_CLIENTTESTCONTEXT_H
#define ENCRYPTEDMESSENGER_CLIENTTESTCONTEXT_H

#include <asio.hpp>
#include <thread>

class ClientTestContext {
public:
    ClientTestContext()
        : work_(asio::make_work_guard(io_)),
          io_thread_([this]() { io_.run(); })
    {}

    ~ClientTestContext() {
        work_.reset();
        io_.stop();
        if (io_thread_.joinable())
            io_thread_.join();
    }

    asio::io_context& io() { return io_; }

private:
    asio::io_context io_;
    asio::executor_work_guard<asio::io_context::executor_type> work_;
    std::thread io_thread_;
};

#endif