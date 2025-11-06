#include "network/tcpServer.h"
#include <iostream>

TcpServer::TcpServer(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)) {
    std::cout << "[TcpServer] Listening on port " << port << std::endl;
    startAccept();
}

void TcpServer::startAccept() {
    // create a new connection object for the next incoming client.
    auto new_connection = TcpConnection::create(io_context_, this);

    // asynchronously wait for a new client to connect.
    acceptor_.async_accept(
        new_connection->socket(),
        [this, new_connection](const std::error_code& error) {
            handleAccept(new_connection, error);
        }
    );
}

void TcpServer::handleAccept(TcpConnection::pointer new_connection, const std::error_code& error) {
    if (!error) {
        std::cout << "[TcpServer] New connection accepted.\n";
        active_connections_.push_back(new_connection);
        new_connection->start();
    } else {
        std::cerr << "[TcpServer] Accept error: " << error.message() << std::endl;
    }

    // continue accepting next connections
    startAccept();
}

void TcpServer::removeConnection(TcpConnection::pointer connection) {
    auto it = std::find(active_connections_.begin(), active_connections_.end(), connection);
    if (it != active_connections_.end()) {
        active_connections_.erase(it);
        std::cout << "[TcpServer] Connection removed. Active connections: "
                  << active_connections_.size() << std::endl;
    }
}