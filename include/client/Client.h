#ifndef ENCRYPTEDMESSENGER_CLIENT_H
#define ENCRYPTEDMESSENGER_CLIENT_H

#include <string>
#include "network/TcpConnection.h"
#include "json.hpp"  // nlohmann::json

// represents a single connected user (client-side)
// responsible for sending structured requests such as login,
// account creation, and messages to the server through TcpConnection
class Client {
public:
    // construct a client with an existing TCP connection
    // the connection is owned via shared_ptr for safe lifetime management
    explicit Client(std::shared_ptr<TcpConnection> connection);

    // send a request to create a new account on the server
    // hashes the password before transmission
    // returns true if the request was successfully sent
    bool createAccount(const std::string& username, const std::string& password);

    // send a login request to the server using hashed password credentials
    // returns true if the request was successfully sent
    bool login(const std::string& username, const std::string& password);

    // send a message to another user through the server
    // message routing and delivery confirmation will be handled by the server
    bool sendMessage(const std::string &recipient, const std::string &message);

private:
    // used to check if tcpConnection function calls fail or pass
    void handleResponse(const std::string& status, const std::string& message);

    // helper for checking success/error
    bool waitForResponse();

    // hash a plain-text password using SHA-256 before sending to the server
    std::string hashPassword(const std::string& password);

private:
    std::shared_ptr<TcpConnection> connection_;  // active TCP connection to the server
    std::string username_;

    // pending action system
    std::string pendingAction_;
    std::string lastLoginUsername_;

    // debug info for tests
    std::string lastStatus_;
    std::string lastMessage_;
};

#endif //ENCRYPTEDMESSENGER_CLIENT_H