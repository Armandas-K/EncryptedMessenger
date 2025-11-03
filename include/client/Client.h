#ifndef ENCRYPTEDMESSENGER_CLIENT_H
#define ENCRYPTEDMESSENGER_CLIENT_H
#include <string>
#include "network/TcpConnection.h"
#include "json.hpp"  // nlohmann::json

class Client {
public:
    explicit Client(std::shared_ptr<TcpConnection> connection);
    bool createAccount(const std::string& username, const std::string& password);
    bool login(const std::string& username, const std::string& password);
    void sendMessage(const std::string & string, const std::string & message);

private:
    std::shared_ptr<TcpConnection> connection_;
    std::string hashPassword(const std::string& password);
};

#endif //ENCRYPTEDMESSENGER_CLIENT_H