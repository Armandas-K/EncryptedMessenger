#include "client/Client.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <iostream>
#include "utils/Logger.h"

using json = nlohmann::json;

Client::Client(std::shared_ptr<TcpConnection> connection)
    : connection_(std::move(connection))
{
    // install callback so tcpConnection can forward server responses to client
    connection_->onServerResponse_ =
        [this](const std::string& status, const std::string& message)
        {
            this->handleResponse(status, message);
        };
}

std::string Client::hashPassword(const std::string& password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)password.c_str(), password.size(), hash);
    std::stringstream ss;
    for (unsigned char c : hash)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    return ss.str();
}

bool Client::createAccount(const std::string &username, const std::string &password) {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot create account: no active connection.\n";
        return false;
    }

    pendingAction_ = "create_account";

    json msg = {
        {"action", "create_account"},
        {"username", username},
        {"password_hash", hashPassword(password)}
    };

    connection_->send(msg.dump());
    connection_->beginRead();
    return waitForResponse();
}

bool Client::login(const std::string& username, const std::string& password) {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot login: no active connection.\n";
        return false;
    }

    pendingAction_ = "login";
    lastLoginUsername_ = username;

    json msg = {
        {"action", "login"},
        {"username", username},
        {"password_hash", hashPassword(password)}
    };

    connection_->send(msg.dump());
    connection_->beginRead();
    return waitForResponse();
}

bool Client::sendMessage(const std::string& to, const std::string& message) {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot send message: no active connection.\n";
        return false;
    }

    pendingAction_ = "send_message";

    json msg = {
        {"action", "send_message"},
        {"to", to},
        {"message", message}
    };

    connection_->send(msg.dump());
    return waitForResponse();
}

void Client::handleResponse(const std::string& status, const std::string& message) {
    lastStatus_ = status;
    lastMessage_ = message;

    // action-specific handling
    // clear pending action
    if (pendingAction_ == "login" && status == "success") {
        username_ = lastLoginUsername_;
        Logger::log("[Client] Logged in as: " + username_);
        pendingAction_.clear();
        return;
    }

    if (pendingAction_ == "create_account" && status == "success") {
        Logger::log("[Client] Account created successfully.");
        pendingAction_.clear();
        return;
    }

    if (pendingAction_ == "send_message" && status == "success") {
        Logger::log("[Client] Message delivered.");
        pendingAction_.clear();
        return;
    }

    // general logging
    if (status == "success") {
        Logger::log("[Client] SUCCESS: " + message);
    } else if (status == "error") {
        std::cerr << "[Client] ERROR: " << message << "\n";
    } else {
        Logger::log("[Client] Response: " + message);
    }

    // just in case
    pendingAction_.clear();
}

bool Client::waitForResponse() {
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    return lastStatus_ == "success";
}