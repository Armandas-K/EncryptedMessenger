#include "network/MessageHandler.h"
#include "network/tcpServer.h"
#include "utils/Logger.h"

MessageHandler::MessageHandler(TcpServer* server, FileStorage& storage)
    : server_(server), storage_(storage), crypto_() {}

bool MessageHandler::processMessage(
    TcpConnection::pointer sender,
    const std::string& to,
    const std::string& message
) {
    std::cout << "proccessMsg called" << "\n";

    // server trusted sender
    std::string from = sender->getUsername();

    if (from.empty()) {
        sender->send(R"({"status":"error","message":"User not logged in"})");
        return false;
    }

    if (to.empty() || message.empty()) {
        sender->send(R"({"status":"error","message":"Missing fields"})");
        return false;
    }

    // validate recipient exists
    if (!storage_.userExists(to)) {
        sender->send(R"({"status":"error","message":"Recipient does not exist"})");
        return false;
    }

    // fetch RSA public keys
    std::string sender_pub    = storage_.getUserPublicKey(from);
    std::string recipient_pub = storage_.getUserPublicKey(to);

    if (sender_pub.empty() || recipient_pub.empty()) {
        sender->send(R"({"status":"error","message":"Missing RSA keys"})");
        return false;
    }

    // encrypt message with AES
    std::vector<uint8_t> aes_key = crypto_.generateAESKey();
    CryptoManager::AESEncrypted ciphertext = crypto_.aesEncrypt(message, aes_key);

    // Convert AES key to string using black magic
    std::string aes_key_str(
        reinterpret_cast<char*>(aes_key.data()),
        aes_key.size()
    );

    // encrypt AES key for both users
    std::string aes_for_sender    = crypto_.rsaEncrypt(aes_key_str, sender_pub);
    std::string aes_for_recipient = crypto_.rsaEncrypt(aes_key_str, recipient_pub);

    long timestamp = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );

    bool stored = storage_.appendConversationMessage(
        from,
        to,
        ciphertext,
        aes_for_sender,
        aes_for_recipient,
        timestamp
    );
    if (!stored) {
        sender->send(R"({"status":"error","message":"Failed to save message"})");
        return false;
    }

    sender->send(R"({"status":"success","message":"Message stored"})");
    return true;
}

bool MessageHandler::fetchMessages(
    const TcpConnection::pointer requester,
    const std::string& withUser
) {
    std::string requesterName = requester->getUsername();

    if (requesterName.empty()) {
        requester->send(R"({"status":"error","message":"Not logged in"})");
        return false;
    }

    if (withUser.empty()) {
        requester->send(R"({"status":"error","message":"Missing 'with' field"})");
        return false;
    }

    if (!storage_.userExists(withUser)) {
        requester->send(R"({"status":"error","message":"User does not exist"})");
        return false;
    }

    // load conversation JSON
    nlohmann::json convo = storage_.loadConversation(requesterName, withUser);

    if (convo.is_null()) {
        requester->send(R"({"status":"success","message":"[]"})");
        return true;
    }

    // build response
    nlohmann::json response;
    response["status"] = "success";
    response["messages"] = convo["messages"];

    requester->send(response.dump());
    return true;
}