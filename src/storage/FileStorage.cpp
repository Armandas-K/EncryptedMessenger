#include "storage/FileStorage.h"
#include <iostream>
#include <direct.h>
#include "utils/Logger.h"
#include "utils/base64.h"

FileStorage::FileStorage() {
    loadUser();
}

bool FileStorage::loadUser() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    std::ifstream file(userFilePath_);

    if (!file.is_open()) {
        std::cerr << "[FileStorage] Could not open users file, creating new one.\n";
        data_["users"] = nlohmann::json::array();
        file.close();
        saveUser();
        return true;
    }

    // handle empty file
    if (file.peek() == std::ifstream::traits_type::eof()) {
        std::cerr << "[FileStorage] Empty users file, reinitializing.\n";
        data_["users"] = nlohmann::json::array();
        file.close();
        saveUser();
        return true;
    }

    try {
        file >> data_;
    } catch (...) {
        std::cerr << "[FileStorage] Invalid JSON format in users file.\n";
        file.close();
        data_["users"] = nlohmann::json::array();
        saveUser();
        return false;
    }

    return true;
}

bool FileStorage::saveUser() {
    std::ofstream file(userFilePath_);
    if (!file.is_open()) return false;

    file << data_.dump(4);
    return true;
}

bool FileStorage::createUser(const std::string& username, const std::string& password_hash) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    for (const auto& user : data_["users"]) {
        if (user["username"] == username) {
            std::cerr << "[FileStorage] Username already exists.\n";
            return false;
        }
    }

    nlohmann::json new_user = {
        {"username", username},
        {"password_hash", password_hash}
    };
    data_["users"].push_back(new_user);
    return saveUser();
}

bool FileStorage::createUserKeyFiles(
    const std::string& username)
{
    std::lock_guard<std::mutex> lock(file_mutex_);

    // build "keys/username"
    std::string userKeyDir = std::string(KEY_PATH) + "/" + username;

    // create user directory in data/keys/
    _mkdir(userKeyDir.c_str());

    // generate RSA keypair
    CryptoManager crypto;
    CryptoManager::RSAKeyPair keys;
    try {
        keys = crypto.generateRSAKeyPair();
    } catch (const std::exception& e) {
        std::cerr << "[FileStorage] RSA key generation failed: " << e.what() << std::endl;
        return false;
    }

    std::string pubPath  = userKeyDir + "/public.pem";
    std::string privPath = userKeyDir + "/private.pem";

    // write public key
    {
        std::ofstream out(pubPath);
        if (!out.is_open()) return false;
        out << keys.publicKeyPem;
    }

    // write private key
    {
        std::ofstream out(privPath);
        if (!out.is_open()) return false;
        out << keys.privateKeyPem;
    }

    return true;
}

bool FileStorage::loginUser(const std::string& username, const std::string& password_hash) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    for (const auto& user : data_["users"]) {
        if (user["username"] == username && user["password_hash"] == password_hash) {
            return true;
        }
    }
    return false;
}

std::string FileStorage::getUserPublicKey(const std::string& username) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    // go to keys/username/public.pem
    std::string pubPath = std::string(KEY_PATH) + "/" + username + "/public.pem";

    std::ifstream file(pubPath);
    if (!file.is_open()) {
        std::cerr << "[FileStorage] Failed to open public key: " << pubPath << "\n";
        return "";
    }

    std::string pem(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    return pem;
}

bool FileStorage::userExists(const std::string &username) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    for (const auto& user : data_["users"]) {
        if (user["username"] == username) {
            return true;
        }
    }
    return false;
}

bool FileStorage::appendConversationMessage(
    const std::string& from,
    const std::string& to,
    const CryptoManager::AESEncrypted& ciphertext,
    const std::string& aesForSender,
    const std::string& aesForRecipient,
    long timestamp
) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    // build folder: messages/userA_userB
    // folder name always alphabetical
    std::string folderName =
        (from < to) ? (from + "_" + to) : (to + "_" + from);

    std::string fullDir = std::string(MESSAGE_PATH) + "/" + folderName;

    /// Create directories recursively
    std::error_code ec;
    std::filesystem::create_directories(fullDir, ec);

    if (ec) {
        std::cerr << "[FileStorage] Failed to create directory: "
                  << fullDir << " (" << ec.message() << ")\n";
        return false;
    }

    // path to conversation file
    std::string convoFile = fullDir + "/conversation.json";

    nlohmann::json convoJson;

    // if file exists load it
    {
        std::ifstream in(convoFile);
        if (in.is_open() && in.peek() != std::ifstream::traits_type::eof()) {
            try {
                in >> convoJson;
            } catch (...) {
                std::cerr << "[FileStorage] Invalid JSON in conversation, resetting.\n";
                convoJson = nlohmann::json::object();
            }
        }
    }

    // ensure structure exists
    if (!convoJson.contains("messages"))
        convoJson["messages"] = nlohmann::json::array();

    // -------- Append new message --------
    nlohmann::json entry;
    entry["from"]              = from;
    entry["to"]                = to;
    entry["timestamp"]         = timestamp;
    entry["ciphertext"]        = base64::encode(ciphertext.ciphertext);
    entry["iv"]                = base64::encode(ciphertext.iv);
    entry["tag"]               = base64::encode(ciphertext.tag);
    entry["aes_for_sender"]    = base64::encode(aesForSender);
    entry["aes_for_recipient"] = base64::encode(aesForRecipient);

    convoJson["messages"].push_back(entry);

    // -------- Save back to file --------
    std::ofstream out(convoFile);
    if (!out.is_open()) {
        std::cerr << "[FileStorage] Failed to write conversation file.\n";
        return false;
    }

    out << convoJson.dump(4);
    return true;
}

nlohmann::json FileStorage::loadConversation(
    const std::string& userA,
    const std::string& userB
) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    std::string folderName = (userA < userB)
        ? (userA + "_" + userB)
        : (userB + "_" + userA);

    std::string fullDir = std::string(MESSAGE_PATH) + "/" + folderName;
    std::string convoFile = fullDir + "/conversation.json";

    nlohmann::json convoJson;

    std::ifstream in(convoFile);
    if (!in.is_open()) {
        // empty = no conversation
        return nlohmann::json();
    }

    try {
        in >> convoJson;
    } catch (...) {
        std::cerr << "[FileStorage] Invalid JSON in " << convoFile << ", resetting\n";
        return nlohmann::json();
    }

    if (!convoJson.contains("messages")) {
        return nlohmann::json::object({{"messages", nlohmann::json::array()}});
    }

    return convoJson;
}
