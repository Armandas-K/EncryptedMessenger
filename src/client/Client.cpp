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
        std::cerr << "[Client] Cannot create account: no active connection\n";
        return false;
    }

    pendingAction_ = "create_account";

    json msg = {
        {"action", "create_account"},
        {"username", username},
        {"password_hash", hashPassword(password)}
    };

    connection_->send(msg.dump());
    return waitForResponse();
}

bool Client::login(const std::string& username, const std::string& password) {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot login: no active connection\n";
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
    return waitForResponse();
}

bool Client::sendMessage(const std::string& to, const std::string& message) {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot send message: no active connection\n";
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

bool Client::getConversations() {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot get conversations: no active connection\n";
        return false;
    }

    if (username_.empty()) {
        std::cerr << "[Client] Cannot get conversations: not logged in\n";
        return false;
    }

    pendingAction_ = "get_conversations";

    json msg = {
        {"action", "get_conversations"}
    };

    connection_->send(msg.dump());
    return waitForResponse();
}

bool Client::getMessages(const std::string& withUser) {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot get messages: no active connection\n";
        return false;
    }

    pendingAction_ = "get_messages";

    json msg = {
        {"action", "get_messages"},
        {"with", withUser}
    };

    connection_->send(msg.dump());
    return waitForResponse();
}

std::vector<std::string> Client::getCachedConversations() {
    std::lock_guard<std::mutex> lock(responseMutex_);
    return conversations_;
}

std::vector<nlohmann::json> Client::getCachedMessages() {
    std::lock_guard<std::mutex> lock(responseMutex_);
    return lastMessages_;
}

void Client::handleResponse(const std::string& status, const std::string& message) {
    {
        // lock before modifying state
        std::lock_guard<std::mutex> lock(responseMutex_);
        lastStatus_ = status;
        lastMessage_ = message;

        // LOGIN
        if (pendingAction_ == "login") {
            if (status == "success") {
                username_ = lastLoginUsername_;
                Logger::log("[Client] Logged in as: " + username_);
            } else {
                std::cerr << "[Client] Login failed: " << message << "\n";
            }

            pendingAction_.clear();
            responseReady_ = true;
            responseCv_.notify_one();
            return;
        }
        // CREATE ACCOUNT
        if (pendingAction_ == "create_account") {
            if (status == "success") {
                Logger::log("[Client] Account created successfully");
            } else {
                std::cerr << "[Client] Failed to create account: " << message << "\n";
            }

            pendingAction_.clear();
            responseReady_ = true;
            responseCv_.notify_one();
            return;
        }
        // SEND MESSAGE
        if (pendingAction_ == "send_message") {
            if (status == "success") {
                Logger::log("[Client] Message delivered");
            } else {
                std::cerr << "[Client] Failed to send message: " << message << "\n";
            }

            pendingAction_.clear();
            responseReady_ = true;
            responseCv_.notify_one();
            return;
        }
        // GET CONVERSATIONS
        if (pendingAction_ == "get_conversations") {
            if (status == "success") {
                try {
                    auto jsonObj = nlohmann::json::parse(message);

                    conversations_.clear();
                    if (jsonObj.contains("conversations") && jsonObj["conversations"].is_array()) {
                        for (auto& c : jsonObj["conversations"]) {
                            if (c.is_string())
                                conversations_.push_back(c.get<std::string>());
                        }
                    }

                    Logger::log("[Client] Retrieved " + std::to_string(conversations_.size()) + " conversations");
                } catch (...) {
                    std::cerr << "[Client] Failed to parse conversations JSON\n";
                }
            } else {
                std::cerr << "[Client] Failed to retrieve conversations: " << message << "\n";
            }

            pendingAction_.clear();
            responseReady_ = true;
            responseCv_.notify_one();
            return;
        }
        // GET MESSAGES
        if (pendingAction_ == "get_messages") {
            if (status == "success") {
                // parse json list of messages
                try {
                    auto jsonObj = nlohmann::json::parse(message);

                    lastMessages_.clear();
                    if (jsonObj.contains("messages")) {
                        for (auto& m : jsonObj["messages"])
                            lastMessages_.push_back(m);
                    }

                    Logger::log("[Client] Retrieved " + std::to_string(lastMessages_.size()) + " messages");
                }
                catch (...) {
                    // todo still being called... fix maybe
                    std::cerr << "[Client] Failed to parse message list JSON\n";
                }
            } else {
                std::cerr << "[Client] Failed to retrieve messages: " << message << "\n";
            }

            pendingAction_.clear();
            responseReady_ = true;
            responseCv_.notify_one();
            return;
        }

        // default / unknown action
        if (status == "success") {
            Logger::log("[Client] SUCCESS: " + message);
        } else if (status == "error") {
            std::cerr << "[Client] ERROR: " << message << "\n";
        } else {
            Logger::log("[Client] Response: " + message);
        }

        // fallback
        pendingAction_.clear();
        responseReady_ = true;
    }
    // notify outside lock
    responseCv_.notify_one();
}

bool Client::waitForResponse() {
    std::unique_lock<std::mutex> lock(responseMutex_);

    // wait until responseReady_ becomes true or timeout
    if (!responseCv_.wait_for(lock, std::chrono::milliseconds(timeoutMs_),
                              [this] { return responseReady_; })) {
        std::cerr << "[Client] Response timed out\n";
        return false;
    }

    responseReady_ = false;
    return lastStatus_ == "success";
}