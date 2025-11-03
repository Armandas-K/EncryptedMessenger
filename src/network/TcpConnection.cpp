#include "network/tcpConnection.h"
#include "network/tcpServer.h"
#include <iostream>

TcpConnection::TcpConnection(asio::io_context& io_context, TcpServer* server)
    : socket_(io_context), server_(server) {}

TcpConnection::pointer TcpConnection::create(asio::io_context& io_context, TcpServer* server) {
    return pointer(new TcpConnection(io_context, server));
}

asio::ip::tcp::socket& TcpConnection::socket() {
    return socket_;
}

void TcpConnection::start() {
    std::cout << "[TcpConnection] Started connection from: "
              << socket_.remote_endpoint().address().to_string() << std::endl;
    readAction();
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
            } else {
                std::cerr << "[TcpConnection] Read error: " << ec.message() << std::endl;
            }
        }
    );
}

void TcpConnection::handleAction(const nlohmann::json& message) {
    if (!message.contains("action")) {
        std::cerr << "[TcpConnection] Missing action field.\n";
        return;
    }

    std::string action = message["action"];

    if (action == "login") {
        handleLogin(message);
    } else if (action == "create_account") {
        handleCreateAccount(message);
    } else if (action == "send_message") {
        handleSendMessage(message);
    } else {
        std::cerr << "[TcpConnection] Unknown action: " << action << std::endl;
    }
}

void TcpConnection::handleLogin(const nlohmann::json& data) {
    std::cout << "[TcpConnection] Handling login for user: "
              << data.value("username", "unknown") << std::endl;

    // todo forward to TcpServer for processing
}

void TcpConnection::handleCreateAccount(const nlohmann::json& data) {
    std::cout << "[TcpConnection] Handling account creation for user: "
              << data.value("username", "unknown") << std::endl;
}

void TcpConnection::handleSendMessage(const nlohmann::json& data) {
    std::cout << "[TcpConnection] Handling send message.\n";
}