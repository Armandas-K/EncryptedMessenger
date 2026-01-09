#include "network/MessageHandler.h"
#include "network/tcpServer.h"
#include "utils/Logger.h"
#include "utils/base64.h"

MessageHandler::MessageHandler(TcpServer* server, FileStorage& storage)
    : server_(server), storage_(storage), crypto_() {}

bool MessageHandler::processMessage(
    TcpConnection::pointer sender,
    const nlohmann::json& message) {
    std::string from = sender->getUsername();
    if (from.empty()) {
        sender->send(R"({"status":"error","message":"User not logged in"})");
        return false;
    }

    nlohmann::json payload = message["payload"];

    // validate required fields
    if (!payload.contains("to") ||
        !payload.contains("ciphertext") ||
        !payload.contains("iv") ||
        !payload.contains("tag") ||
        !payload.contains("aes_for_sender") ||
        !payload.contains("aes_for_recipient")) {

        sender->send(R"({"status":"error","message":"Invalid payload"})");
        return false;
        }

    std::string to = payload["to"];

    if (!storage_.userExists(to)) {
        sender->send(R"({"status":"error","message":"Recipient does not exist"})");
        return false;
    }

    long timestamp =
        std::chrono::system_clock::to_time_t(
            std::chrono::system_clock::now());

    auto ivB64  = payload["iv"].get<std::string>();
    auto ctB64  = payload["ciphertext"].get<std::string>();
    auto tagB64 = payload["tag"].get<std::string>();
    auto aesSB64 = payload["aes_for_sender"].get<std::string>();
    auto aesRB64 = payload["aes_for_recipient"].get<std::string>();

    bool ok = storage_.appendConversationMessage(
        from,
        to,
        CryptoManager::AESEncrypted{
            base64::decode(ivB64),
            base64::decode(ctB64),
            base64::decode(tagB64)
        },
        base64::bytesToString(base64::decode(aesSB64)),
        base64::bytesToString(base64::decode(aesRB64)),
        timestamp
    );

    if (!ok) {
        sender->send(R"({"status":"error","message":"Failed to save message"})");
        return false;
    }

    sender->send(R"({"status":"success","message":"Message stored"})");
    return true;
}

bool MessageHandler::checkUserExists(
    const TcpConnection::pointer requester,
    const std::string& username) {
    nlohmann::json response;
    response["status"] = "success";
    response["exists"] = storage_.userExists(username);
    requester->send(response.dump());
    return true;
}

bool MessageHandler::fetchMessages(
    const TcpConnection::pointer requester,
    const std::string& withUser,
    long lastSeen,
    std::string mode) {
    std::string requesterName = requester->getUsername();

    if (requesterName.empty()) {
        requester->send(R"({"status":"error","message":"Not logged in"})");
        return false;
    }

    if (!storage_.userExists(withUser)) {
        // empty conversation - no error
        nlohmann::json response;
        response["status"] = "success";
        response["messages"] = nlohmann::json::array();
        requester->send(response.dump());
        return true;
    }

    // load conversation JSON
    nlohmann::json convo =
        storage_.loadConversationSince(requesterName, withUser, lastSeen);

    // build response
    nlohmann::json response;
    response["status"] = "success";
    response["messages"] = convo["messages"];
    response["mode"] = mode;

    requester->send(response.dump());
    return true;
}

bool MessageHandler::fetchConversations(TcpConnection::pointer requester) {
    std::string user = requester->getUsername();

    if (user.empty()) {
        requester->send(R"({"status":"error","message":"Not logged in"})");
        return false;
    }

    nlohmann::json conversations = storage_.listConversations(user);

    nlohmann::json response;
    response["status"] = "success";
    response["conversations"] = conversations;

    requester->send(response.dump());
    return true;
}