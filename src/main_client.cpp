#include "client/CLI.h"
#include "client/Client.h"

int main() {
    auto client = std::make_shared<Client>();
    CLI cli(client);
    cli.run();
    return 0;
}