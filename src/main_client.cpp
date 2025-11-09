#include "client/CLI.h"
#include "client/Client.h"

int main() {
    asio::io_context io_context;
    auto connection = TcpConnection::create(io_context, nullptr);
    auto client = std::make_shared<Client>(connection);

    CLI cli(client);
    cli.run();

    return 0;
}