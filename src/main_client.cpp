#include "client/CLI.h"
#include "client/Client.h"
#include "network/tcpServer.h"

int main() {
    // server (prob shouldnt be in client)
    asio::io_context serverIo;
    auto serverWork =
        asio::make_work_guard(serverIo);

    std::thread serverThread([&]() {
        TcpServer server(serverIo, 5555);
        serverIo.run();
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // client
    asio::io_context io;
    auto clientWork =
        asio::make_work_guard(io);

    std::thread ioThread([&]() {
        io.run();
    });

    try {
        CLI cli(io, "127.0.0.1", 5555);
        cli.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
    }

    // shutdown
    clientWork.reset();
    io.stop();
    ioThread.join();

    serverWork.reset();
    serverIo.stop();
    serverThread.join();
}
