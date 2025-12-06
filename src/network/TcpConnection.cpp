#include "network/tcpConnection.h"
#include "network/tcpServer.h"
#include <iostream>

#include "utils/Logger.h"

TcpConnection::TcpConnection(asio::io_context& io_context, TcpServer* server)
    : socket_(io_context),
      io_context_(io_context),
      server_(server),
      username_("")
{}

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
        Logger::log("[TcpConnection] Started connection from: "
                  + endpoint.address().to_string() + ":"
                  + std::to_string(endpoint.port()));
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
            if (ec) {
                disconnect();
                return;
            }

            // append received bytes to stream buffer
            incomingBuffer_.append(buffer_.data(), length);

            size_t start = 0;
            int depth = 0;

            for (size_t i = 0; i < incomingBuffer_.size(); ++i) {

                char c = incomingBuffer_[i];

                if (c == '{') {
                    if (depth == 0) {
                        start = i;  // potential start of JSON object
                    }
                    depth++;
                }
                else if (c == '}') {
                    depth--;
                }

                // if depth = 0 means complete json object
                if (depth == 0 && c == '}') {

                    std::string jsonStr = incomingBuffer_.substr(start, i - start + 1);

                    try {
                        nlohmann::json msg = nlohmann::json::parse(jsonStr);
                        handleAction(msg);
                    }
                    catch (std::exception& e) {
                        std::cerr << "[TcpConnection] JSON parse error: " << e.what() << "\n";
                    }
                }
            }

            // remove json objects
            // if depth != 0, keep partial object
            if (depth == 0) {
                incomingBuffer_.clear();
            } else {
                incomingBuffer_ = incomingBuffer_.substr(start);
            }

            // continue reading
            readAction();
        }
    );
}

void TcpConnection::send(const std::string& message) {
    if (message.empty()) {
        std::cerr << "[TcpConnection] Cannot send: empty message.\n";
        return;
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
                Logger::log("[TcpConnection] Outgoing request queued for delivery.\n");
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

    Logger::log("[TcpConnection] Disconnected.\n");

    if (server_) {
        server_->removeConnection(shared_from_this());
    }
}

void TcpConnection::handleAction(const nlohmann::json& message) {

    // request (client to server)
    if (message.contains("action")) {
        if (server_) {
            server_->handleAction(shared_from_this(), message);
        } else {
            std::cerr << "[TcpConnection] Received request but no server is attached.\n";
        }
        return;
    }

    // response (server to client)
    if (message.contains("status")) {
        handleServerResponse(message);
        return;
    }

    // unknown message
    std::cerr << "[TcpConnection] Unknown message type: " << message.dump() << "\n";
}

void TcpConnection::handleServerResponse(const nlohmann::json& msg) {
    std::string status  = msg.value("status", "unknown");
    std::string message = msg.value("message", "");

    if (status == "success") {
        Logger::log("[TcpConnection] Server SUCCESS: " + message);
    } else if (status == "error") {
        std::cerr << "[TcpConnection] Server ERROR: " << message << "\n";
    } else {
        std::cout << "[TcpConnection] Server Response: " << message << "\n";
    }
    // could route it to Client::onResponse() in the future
}