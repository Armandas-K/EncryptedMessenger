#include "network/tcpServer.h"
#include <iostream>
#include "utils/Logger.h"

TcpServer::TcpServer(asio::io_context& io_context, unsigned short port)
    : io_context_(io_context),
      acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      storage_(),
      messageHandler_(this, storage_)
{
    Logger::log("[TcpServer] Listening on port " + std::to_string(port));
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
        Logger::log("[TcpServer] New Connection accepted. Active connections: "
            + std::to_string(active_connections_.size()+1));
        active_connections_.push_back(new_connection);
        new_connection->beginRead();
    } else {
        std::cerr << "[TcpServer] Accept error: " << error.message() << std::endl;
    }

    // continue accepting next connections
    startAccept();
}

void TcpServer::handleAction(TcpConnection::pointer connection, const nlohmann::json& message) {
    std::string action = message.value("action", "");

    if (action == "create_account") {
        handleCreateAccount(connection, message);
    } else if (action == "login") {
        handleLogin(connection, message);
    } else if (action == "send_message") {
        handleSendMessage(connection, message);
    } else if (action == "get_conversations") {
        handleGetConversations(connection, message);
    } else if (action == "get_messages") {
        handleGetMessages(connection, message);
    } else if (action == "user_exists") {
        handleUserExists(connection, message);
    } else if (action == "get_public_key") {
        handleGetPublicKey(connection, message);
    } else {
        std::cerr << "[TcpServer] Unknown action: " << action << std::endl;
    }
}

void TcpServer::handleCreateAccount(
    TcpConnection::pointer connection,
    const nlohmann::json& data) {
    const std::string username      = data.value("username", "");
    const std::string password_hash = data.value("password_hash", "");
    const std::string public_key    = data.value("public_key", "");

    if (username.empty() || password_hash.empty() || public_key.empty()) {
        connection->send(R"({"status":"error","message":"Missing credentials or public key"})");
        return;
    }

    // atomic operation start
    std::lock_guard<std::mutex> guard(storage_.mutex());

    if (storage_.userExists_NoLock(username)) {
        connection->send(R"({"status":"error","message":"User already exists"})");
        return;
    }

    // write user to json
    if (!storage_.createUser_NoLock(username, password_hash)) {
        connection->send(R"({"status":"error","message":"Failed to create user"})");
        return;
    }

    // store public key
    if (!storage_.storeUserPublicKey_NoLock(username, public_key)) {
        // rollback
        storage_.deleteUserJson_NoLock(username);
        storage_.saveUser_NoLock();
        connection->send(R"({"status":"error","message":"Failed to store public key"})");
        return;
    }

    connection->send(R"({"status":"success","message":"Account created"})");
}

void TcpServer::handleLogin(TcpConnection::pointer connection, const nlohmann::json& data) {
    std::string username = data.value("username", "");
    std::string password_hash = data.value("password_hash", "");

    if (!storage_.userExists(username)) {
        connection->send(R"({"status":"error","message":"Invalid username"})");
        return;
    }

    if (!storage_.loginUser(username, password_hash)) {
        connection->send(R"({"status":"error","message":"Invalid password"})");
        return;
    }
    // assign username to connection instance
    connection->setUsername(username);

    connection->send(R"({"status":"success","message":"Login successful"})");
}

void TcpServer::handleGetConversations(
    TcpConnection::pointer connection,
    const nlohmann::json& data) {
    messageHandler_.fetchConversations(connection);
}

void TcpServer::handleSendMessage(TcpConnection::pointer connection, const nlohmann::json& data) {
    messageHandler_.processMessage(connection, data);
}

void TcpServer::handleGetMessages(
    TcpConnection::pointer connection,
    const nlohmann::json& data) {
    std::string withUser = data.value("with", "");
    long lastSeen = data.value("last_seen", 0L);

    if (withUser.empty()) {
        connection->send(R"({"status":"error","message":"Missing username"})");
        return;
    }

    messageHandler_.fetchMessages(connection, withUser, lastSeen);
}

void TcpServer::handleUserExists(TcpConnection::pointer connection,
                                 const nlohmann::json& data) {
    const std::string username = data.value("username", "");

    if (username.empty()) {
        connection->send(R"({"status":"error","message":"Missing username"})");
        return;
    }

    bool exists = storage_.userExists(username);

    nlohmann::json response;
    response["status"] = "success";
    response["exists"] = exists;

    connection->send(response.dump());
}

void TcpServer::handleGetPublicKey(
    TcpConnection::pointer connection,
    const nlohmann::json& data) {
    std::string user = data.value("username", "");
    if (user.empty()) {
        connection->send(R"({"status":"error","message":"Missing username"})");
        return;
    }

    if (!storage_.userExists(user)) {
        connection->send(R"({"status":"error","message":"User does not exist"})");
        return;
    }

    std::string pub = storage_.getUserPublicKey(user);
    if (pub.empty()) {
        connection->send(R"({"status":"error","message":"Public key not found"})");
        return;
    }

    nlohmann::json resp;
    resp["status"] = "success";
    resp["message"] = pub; // PEM as JSON string
    connection->send(resp.dump());
}

void TcpServer::removeConnection(TcpConnection::pointer connection) {
    auto it = std::find(active_connections_.begin(), active_connections_.end(), connection);
    if (it != active_connections_.end()) {
        active_connections_.erase(it);
        Logger::log("[TcpServer] Connection removed. Active connections: "
                  + std::to_string(active_connections_.size()));
    }
}