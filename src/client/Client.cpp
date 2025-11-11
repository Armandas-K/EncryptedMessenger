#include "client/Client.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <iostream>

using json = nlohmann::json;

Client::Client(std::shared_ptr<TcpConnection> connection)
    : connection_(std::move(connection)) {}

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

    nlohmann::json msg = {
        {"action", "create_account"},
        {"username", username},
        {"password_hash", hashPassword(password)}
    };

    connection_->send(msg.dump());
    return connection_->beginRead();
}

bool Client::login(const std::string& username, const std::string& password) {
    json msg = {
        {"action", "login"},
        {"username", username},
        {"password_hash", hashPassword(password)}
    };
    connection_->send(msg.dump());
    return connection_->beginRead();
}
void Client::sendMessage(const std::string &string, const std::string &message) {
}
