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

    // get list of users conversations
    bool getConversations();

    // receive messages from conversation with this user and withUser
    bool getMessages(const std::string &withUser);

    // getters for CLI (copies to avoid returning refs guarded by a mutex)
    std::vector<std::string> getCachedConversations();
    std::vector<nlohmann::json> getCachedMessages();

    // helpers
    bool isLoggedIn() const { return !username_.empty(); }
    const std::string& getUsername() const { return username_; }

private:
    // used to check if tcpConnection function calls fail or pass
    void handleResponse(const std::string& status, const std::string& message);
    // check if getMessages call pass or fail
    void handleMessagesResponse(const nlohmann::json &msg);

    // helper for checking success/error response from server
    bool waitForResponse();

    // hash a plain-text password using SHA-256 before sending to the server
    std::string hashPassword(const std::string& password);

private:
    std::shared_ptr<TcpConnection> connection_;  // active TCP connection to the server
    std::string username_;

    // pending action system
    std::string pendingAction_;
    std::string lastLoginUsername_;

    // cached data for CLI
    std::vector<std::string> conversations_;
    std::vector<nlohmann::json> lastMessages_;

    // server response checking/debug
    std::string lastStatus_;
    std::string lastMessage_;
    std::mutex responseMutex_;
    std::condition_variable responseCv_;
    bool responseReady_ = false;
    // timeout for server responses
    int timeoutMs_ = 500;
};

#endif //ENCRYPTEDMESSENGER_CLIENT_H