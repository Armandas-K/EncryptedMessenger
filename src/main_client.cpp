#include "iostream"
#include "client/CLI.h"
#include "client/Client.h"

int main() {
    try {
        asio::io_context io;

        // work guard so io doesnt exit
        auto work = asio::make_work_guard(io);

        std::thread ioThread([&]() {
            io.run();
        });

        CLI cli(io, "127.0.0.1", 5555);
        cli.run();

        // shutdown
        work.reset();
        io.stop();
        ioThread.join();
    }
    catch (const std::exception& e) {
        std::cerr << "[Client] error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
