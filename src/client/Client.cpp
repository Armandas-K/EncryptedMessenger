#include "client/Client.h"
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <openssl/sha.h>
#include <crypto/CryptoManager.h>
#include <utils/base64.h>
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
    // get messages callback
    connection_->onMessagesResponse_ =
        [this](const nlohmann::json& msg) {
            handleMessagesResponse(msg);
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

std::string Client::decryptMessage(const nlohmann::json& msg) {
    // select correct AES key
    const std::string& encKeyB64 =
        (msg["to"] == username_)
            ? msg["aes_for_recipient"]
            : msg["aes_for_sender"];

    // decode + decrypt AES key
    std::string aesKeyStr = crypto_.rsaDecrypt(
        base64::bytesToString(base64::decode(encKeyB64)),
        privateKeyPem_
    );

    std::vector<uint8_t> aesKey(
        aesKeyStr.begin(), aesKeyStr.end()
    );

    // decode AES fields
    auto iv  = base64::decode(msg["iv"]);
    auto ct  = base64::decode(msg["ciphertext"]);
    auto tag = base64::decode(msg["tag"]);

    // decrypt message
    return crypto_.aesDecrypt(aesKey, iv, ct, tag);
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

std::vector<std::string> Client::getDecryptedMessages() {
    std::lock_guard<std::mutex> lock(responseMutex_);

    std::vector<std::string> out;
    out.reserve(lastMessages_.size());

    for (const auto& msg : lastMessages_) {
        try {
            std::string plaintext = decryptMessage(msg);

            std::string from = msg.value("from", "unknown");
            out.push_back(from + ": " + plaintext);
        }
        catch (const std::exception& e) {
            out.push_back("[Failed to decrypt message]");
        }
    }

    return out;
}

std::string Client::loadPrivateKey(const std::string& username) {
    std::string path = std::string(KEY_PATH) + "/" + username + "/private.pem";

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open private key for " + username);
    }

    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
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

                try {
                    privateKeyPem_ = loadPrivateKey(username_);
                } catch (const std::exception& e) {
                    std::cerr << "[Client] " << e.what() << "\n";
                }

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
        /* GET MESSAGES
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
        */
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

void Client::handleMessagesResponse(const nlohmann::json& msg) {
    std::lock_guard<std::mutex> lock(responseMutex_);

    lastStatus_ = msg.value("status", "error");

    if (lastStatus_ == "success" && msg.contains("messages")) {
        lastMessages_.clear();
        for (const auto& m : msg["messages"]) {

            lastMessages_.push_back(m);
        }
    }

    pendingAction_.clear();
    responseReady_ = true;
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