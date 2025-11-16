#include "network/tcpConnection.h"
#include "network/tcpServer.h"
#include <iostream>

TcpConnection::TcpConnection(asio::io_context& io_context, TcpServer* server)
    : socket_(io_context),
      io_context_(io_context),
      server_(server) {}

TcpConnection::pointer TcpConnection::create(asio::io_context& io_context, TcpServer* server) {
    return pointer(new TcpConnection(io_context, server));
}

asio::ip::tcp::socket& TcpConnection::socket() {
    return socket_;
}

bool TcpConnection::beginRead() {
    if (!socket_.is_open()) {
        std::cerr << "[TcpConnection] Cannot start: socket is not open.\n";
        return false;
    }

    try {
        auto endpoint = socket_.remote_endpoint();
        std::cout << "[TcpConnection] Started connection from: "
                  << endpoint.address().to_string() << ":" << endpoint.port() << std::endl;
    } catch (const std::system_error& e) {
        std::cerr << "[TcpConnection] Could not retrieve remote endpoint: "
                  << e.what() << std::endl;
        // connection might still be valid
    }

    readAction();
    return true;
}

void TcpConnection::readAction() {
    auto self(shared_from_this());
    socket_.async_read_some(
        asio::buffer(buffer_),
        [this, self](std::error_code ec, std::size_t length) {
            if (!ec) {
                std::string data(buffer_.data(), length);

                try {
                    auto json_msg = nlohmann::json::parse(data);
                    handleAction(json_msg);
                } catch (std::exception& e) {
                    std::cerr << "[TcpConnection] Invalid JSON: " << e.what() << std::endl;
                }

                // keep reading for next messages
                readAction();
            }
            else
            {
                // expected disconnects
                if (ec == asio::error::eof ||
                    ec == asio::error::connection_reset ||
                    ec == asio::error::connection_aborted)
                {
                    // silently disconnect
                    disconnect();
                    return;
                }

                // real errors
                std::cerr << "[TcpConnection] Read error: " << ec.message() << "\n";
                disconnect();
            }
        }
    );
}

void TcpConnection::send(const std::string& message) {
    if (message.empty()) {
        std::cerr << "[TcpConnection] Cannot send: empty message.\n";
    }

    auto self(shared_from_this());

    asio::async_write(
        socket_,
        asio::buffer(message),
        [this, self](std::error_code ec, std::size_t /*bytes_transferred*/) {
            if (ec) {
                std::cerr << "[TcpConnection] Request failed: " << ec.message() << std::endl;
                disconnect();
            } else {
                std::cout << "[TcpConnection] Outgoing request queued for delivery.\n";
            }
        }
    );
}

bool TcpConnection::connect(const std::string& host, int port) {
    asio::ip::tcp::resolver resolver(io_context_);
    asio::error_code ec;

    auto endpoints = resolver.resolve(host, std::to_string(port), ec);
    if (ec) {
        std::cerr << "[TcpConnection] Resolve error: " << ec.message() << "\n";
        return false;
    }

    asio::connect(socket_, endpoints, ec);
    if (ec) {
        std::cerr << "[TcpConnection] Connect error: " << ec.message() << "\n";
        return false;
    }

    return true;
}

void TcpConnection::disconnect() {
    if (!socket_.is_open()) {
        return; // already closed
    }

    asio::error_code ec;

    // Shutdown cleanly
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
    socket_.close(ec);

    std::cout << "[TcpConnection] Disconnected.\n";

    if (server_) {
        server_->removeConnection(shared_from_this());
    }
}

void TcpConnection::handleAction(const nlohmann::json& message) {
    if (!message.contains("action")) {
        std::cerr << "[TcpConnection] Missing action field.\n";
        return;
    }

    if (server_) {
        server_->handleAction(shared_from_this(), message);
    }
}