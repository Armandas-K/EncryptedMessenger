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

nlohmann::json Client::encryptMessagePayload(
    const std::string& to,
    const std::string& plaintext) {
    if (username_.empty()) throw std::runtime_error("Not logged in");
    if (privateKeyPem_.empty()) throw std::runtime_error("Private key not loaded");

    // get pubkey edits pendingAction_ but system only works with 1 function 1 request
    // save previous send_message pendingAction before it is overwritten by get_pub_key
    std::string savedAction;
    {
        std::lock_guard<std::mutex> lock(responseMutex_);
        savedAction = pendingAction_;
        pendingAction_.clear();
    }
    // pubkeys from cache or server
    std::string recipientPub = getPublicKeyCachedOrFetch(to);
    std::string senderPub    = getPublicKeyCachedOrFetch(username_);
    {
        std::lock_guard<std::mutex> lock(responseMutex_);
        pendingAction_ = savedAction;
    }


    // generate AES key and encrypt
    std::vector<uint8_t> aesKey = crypto_.generateAESKey();
    CryptoManager::AESEncrypted enc = crypto_.aesEncrypt(plaintext, aesKey);

    // convert AES key bytes to string
    std::string aesKeyStr(reinterpret_cast<const char*>(aesKey.data()), aesKey.size());

    // RSA encrypt AES key for sender + recipient
    std::string aesForSender    = crypto_.rsaEncrypt(aesKeyStr, senderPub);
    std::string aesForRecipient = crypto_.rsaEncrypt(aesKeyStr, recipientPub);

    // build payload (base64 for transport)
    nlohmann::json payload;
    payload["to"] = to;
    payload["ciphertext"]        = base64::encode(enc.ciphertext);
    payload["iv"]                = base64::encode(enc.iv);
    payload["tag"]               = base64::encode(enc.tag);
    payload["aes_for_sender"]    = base64::encode(aesForSender);
    payload["aes_for_recipient"] = base64::encode(aesForRecipient);

    return payload;
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

    keyStore_ = std::make_unique<KeyStore>(username);
    // generate keys for new account
    CryptoManager::RSAKeyPair kp = crypto_.generateRSAKeyPair();

    // store keys locally
    keyStore_->savePrivateKey(kp.privateKeyPem);
    keyStore_->savePublicKey(kp.publicKeyPem);

    json msg = {
        {"action", "create_account"},
        {"username", username},
        {"password_hash", hashPassword(password)},
        {"public_key", kp.publicKeyPem}
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

bool Client::logout() {
    std::lock_guard<std::mutex> lock(responseMutex_);

    // clear identity
    username_.clear();
    privateKeyPem_.clear();
    lastLoginUsername_.clear();

    // clear cached data
    conversations_.clear();
    lastMessages_.clear();
    keyStore_.reset();

    // clear pending/response state
    pendingAction_.clear();
    lastStatus_.clear();
    lastMessage_.clear();
    responseReady_ = false;

    Logger::log("[Client] Logged out");

    return true;
}

bool Client::sendMessage(const std::string& to,
                         const std::string& message) {
    if (!connection_ || !connection_->socket().is_open()) {
        std::cerr << "[Client] Cannot send message: no active connection\n";
        return false;
    }

    if (username_.empty()) {
        std::cerr << "[Client] Cannot send message: not logged in\n";
        return false;
    }

    pendingAction_ = "send_message";

    try {
        nlohmann::json encrypted = encryptMessagePayload(to, message);

        nlohmann::json req;
        req["action"] = "send_message";
        req["payload"] = encrypted;

        connection_->send(req.dump());
    }
    catch (const std::exception& e) {
        std::cerr << "[Client] Encryption failed: " << e.what() << "\n";
        pendingAction_.clear();
        return false;
    }

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

    // find last seen timestamp for this conversation, 0 = full conv
    long since = 0;
    {
        std::lock_guard<std::mutex> lock(responseMutex_);
        auto it = lastSeenTimestamps_.find(withUser);
        if (it != lastSeenTimestamps_.end()) {
            since = it->second;
        }
    }

    json msg = {
        {"action", "get_messages"},
        {"with", withUser},
        {"since", since}
    };

    connection_->send(msg.dump());
    return waitForResponse();
}

bool Client::userExists(const std::string& username) {
    pendingAction_ = "user_exists";

    json msg = {
        {"action", "user_exists"},
        {"username", username}
    };

    connection_->send(msg.dump());
    return waitForResponse();
}

bool Client::fetchPublicKey(const std::string& username) {
    if (!connection_ || !connection_->socket().is_open()) return false;

    pendingAction_ = "get_public_key";
    requestedPublicKeyUser_ = username;

    nlohmann::json req = {
        {"action", "get_public_key"},
        {"username", username}
    };

    connection_->send(req.dump());
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
            long ts = msg.value("timestamp", 0L);

            // format timestamp
            std::time_t t = static_cast<std::time_t>(ts);
            std::tm tm{};
            localtime_s(&tm, &t);

            std::ostringstream timeStr;
            timeStr << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

            out.push_back(
                "[" + timeStr.str() + "] " + from + ": " + plaintext
            );
        }
        catch (const std::exception&) {
            out.push_back("[Failed to decrypt message]");
        }
    }

    return out;
}

void Client::clearCachedMessages() {
    std::lock_guard<std::mutex> lock(responseMutex_);
    lastMessages_.clear();
}

std::string Client::getPublicKeyCachedOrFetch(const std::string& user) {
    // get clients own pubkey
    if (user == username_) {
        return keyStore_->loadPublicKey();
    }

    // attempt cache lookup
    {
        std::lock_guard<std::mutex> lock(responseMutex_);
        if (auto it = publicKeyCache_.find(user); it != publicKeyCache_.end()) {
            return it->second;
        }
    }

    // fetch from server
    if (!fetchPublicKey(user)) {
        throw std::runtime_error("Failed to fetch public key for " + user);
    }

    // add fetched pubkey to cache
    std::lock_guard<std::mutex> lock(responseMutex_);
    return publicKeyCache_.at(user);
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

                keyStore_ = std::make_unique<KeyStore>(username_);

                try {
                    privateKeyPem_ = keyStore_->loadPrivateKey();
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
                    auto arr = nlohmann::json::parse(message);
                    conversations_.clear();
                    for (auto& c : arr) {
                        conversations_.push_back(c.get<std::string>());
                    }
                } catch (...) {
                    std::cerr << "[Client] Failed to parse conversations list\n";
                }
            } else {
                std::cerr << "[Client] Failed to get conversations: " << message << "\n";
            }

            pendingAction_.clear();
            responseReady_ = true;
            responseCv_.notify_one();
            return;
        }
        // USER EXISTS
        if (pendingAction_ == "user_exists") {
            bool exists = false;

            try {
                auto obj = nlohmann::json::parse(message);
                exists = obj.value("exists", false);
            } catch (...) {
                exists = false;
            }

            pendingAction_.clear();
            responseReady_ = true;
            responseCv_.notify_one();

            // override lastStatus_ so waitForResponse returns correctly
            lastStatus_ = exists ? "success" : "error";
            return;
        }
        // GET PUBLIC KEY
        if (pendingAction_ == "get_public_key") {
            if (status == "success") {
                // message contains the PEM
                publicKeyCache_[requestedPublicKeyUser_] = message;
            } else {
                std::cerr << "[Client] Failed to get public key: " << message << "\n";
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

void Client::handleMessagesResponse(const nlohmann::json& msg) {
    std::lock_guard<std::mutex> lock(responseMutex_);

    lastStatus_ = msg.value("status", "error");

    if (lastStatus_ != "success" ||
        !msg.contains("messages") ||
        !msg["messages"].is_array()) {

        pendingAction_.clear();
        responseReady_ = true;
        responseCv_.notify_one();
        return;
    }

    for (const auto& m : msg["messages"]) {
        // skip if not json obj
        if (!m.is_object()) {
            continue;
        }

        // append each new message
        lastMessages_.push_back(m);

        const long ts = m.value("timestamp", 0L);
        const std::string from = m.value("from", "");
        const std::string to   = m.value("to", "");

        // skip if missing from/to fields
        if (from.empty() || to.empty()) {
            continue;
        }

        // get other users username
        std::string other =
            (from == username_) ? to : from;

        // update last seen timestamp
        auto& lastSeen = lastSeenTimestamps_[other];
        if (ts > lastSeen) {
            lastSeen = ts;
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