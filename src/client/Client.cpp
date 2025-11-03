#include "client/Client.h"
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

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
}

bool Client::login(const std::string& username, const std::string& password) {
}

void Client::sendMessage(const std::string &string, const std::string &message) {
}
