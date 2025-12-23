#include "storage/FileStorage.h"
#include <iostream>
#include <direct.h>
#include "utils/Logger.h"
#include "utils/base64.h"

FileStorage::FileStorage() {
    initializeDirectories();
    loadUser();
}

void FileStorage::initializeDirectories() {
    std::error_code ec;

    // create root "data" directory
    std::filesystem::create_directories(std::filesystem::path(USERS_PATH).parent_path(), ec);

    // create data/keys directory
    std::filesystem::create_directories(KEY_PATH, ec);

    // create data/messages directory
    std::filesystem::create_directories(MESSAGE_PATH, ec);
}

bool FileStorage::loadUser() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    std::ifstream file(userFilePath_);

    if (!file.is_open()) {
        // first run: initialise user.json
        data_["users"] = nlohmann::json::array();
        saveUser_NoLock();
        return true;
    }

    // user.json exists but is empty
    if (file.peek() == std::ifstream::traits_type::eof()) {
        data_["users"] = nlohmann::json::array();
        saveUser_NoLock();
        return true;
    }

    try {
        file >> data_;
    } catch (...) {
        std::cerr << "[FileStorage] Invalid JSON format in users file.\n";
        file.close();
        data_["users"] = nlohmann::json::array();
        saveUser_NoLock();
        return false;
    }

    return true;
}

std::string FileStorage::makeConversationId(const std::string& a, const std::string& b) {
    // length prefixed encoding: <lengthA>:<userA>|<lengthB>:<userB>
    if (a < b) {
        return std::to_string(a.size()) + ":" + a + "|" +
               std::to_string(b.size()) + ":" + b;
    } else {
        return std::to_string(b.size()) + ":" + b + "|" +
               std::to_string(a.size()) + ":" + a;
    }
}

auto FileStorage::parseConversationId(const std::string &id) -> std::pair<std::string, std::string> {
    size_t colon1 = id.find(':');
    size_t pipe   = id.find('|');

    int lenA = std::stoi(id.substr(0, colon1));
    std::string a = id.substr(colon1 + 1, lenA);

    size_t colon2 = id.find(':', pipe);
    int lenB = std::stoi(id.substr(pipe + 1, colon2 - pipe - 1));
    std::string b = id.substr(colon2 + 1, lenB);

    return {a, b};
}

std::filesystem::path FileStorage::conversationDirPath_NoLock(const std::string& userA,
                                                             const std::string& userB) {
    const std::string id = makeConversationId(userA, userB);
    const std::string safe = toFilesystemSafe(id);
    return std::filesystem::path(MESSAGE_PATH) / safe;
}

std::filesystem::path FileStorage::conversationFilePath_NoLock(const std::string& userA,
                                                              const std::string& userB) {
    return conversationDirPath_NoLock(userA, userB) / "conversation.json";
}

std::string FileStorage::toFilesystemSafe(const std::string& input) {
    std::string b64 = base64::encode(
        reinterpret_cast<const uint8_t*>(input.data()),
        input.size()
    );

    for (char& c : b64) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }

    // strip padding
    while (!b64.empty() && b64.back() == '=') {
        b64.pop_back();
    }

    return b64;
}

std::string FileStorage::fromFilesystemSafe(const std::string& safe) {
    std::string b64 = safe;

    for (char& c : b64) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }

    // restore padding to multiple of 4
    while (b64.size() % 4 != 0) {
        b64.push_back('=');
    }

    std::vector<uint8_t> bytes = base64::decode(b64);
    return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
}

bool FileStorage::createUser(const std::string& username,
                             const std::string& password_hash) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    return createUser_NoLock(username, password_hash);
}

bool FileStorage::createUser_NoLock(const std::string& username, const std::string& password_hash) {
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
    return saveUser_NoLock();
}

bool FileStorage::createUserKeyFiles_NoLock(
    const std::string& username) {

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

bool FileStorage::userExists_NoLock(const std::string &username) {
    for (const auto& user : data_["users"]) {
        if (user["username"] == username) {
            return true;
        }
    }
    return false;
}

bool FileStorage::userExists(const std::string &username) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    return userExists_NoLock(username);
}

nlohmann::json FileStorage::listConversations(const std::string& username) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    nlohmann::json result = nlohmann::json::array();
    std::filesystem::path root = MESSAGE_PATH;

    std::error_code ec;
    if (!std::filesystem::exists(root, ec)) {
        return result;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;

        const std::string dirName = entry.path().filename().string();

        try {
            const std::string convoId = fromFilesystemSafe(dirName);
            auto [a, b] = parseConversationId(convoId);

            if (a == username) result.push_back(b);
            else if (b == username) result.push_back(a);
        } catch (...) {
            continue;
        }
    }

    return result;
}

nlohmann::json FileStorage::loadConversationJson_NoLock(
    const std::filesystem::path& convoFile) {
    nlohmann::json convo;

    std::ifstream in(convoFile);
    if (in.is_open() && in.peek() != std::ifstream::traits_type::eof()) {
        try {
            in >> convo;
        } catch (...) {
            std::cerr << "[FileStorage] Invalid JSON in conversation file, resetting.\n";
            convo = nlohmann::json::object();
        }
    }

    if (!convo.contains("messages")) {
        convo["messages"] = nlohmann::json::array();
    }

    return convo;
}

nlohmann::json FileStorage::buildMessageEntry_NoLock(
    const std::string& from,
    const std::string& to,
    const CryptoManager::AESEncrypted& ciphertext,
    const std::string& aesForSender,
    const std::string& aesForRecipient,
    long timestamp) {
    nlohmann::json entry;
    entry["from"]              = from;
    entry["to"]                = to;
    entry["timestamp"]         = timestamp;
    entry["ciphertext"]        = base64::encode(ciphertext.ciphertext);
    entry["iv"]                = base64::encode(ciphertext.iv);
    entry["tag"]               = base64::encode(ciphertext.tag);
    entry["aes_for_sender"]    = base64::encode(aesForSender);
    entry["aes_for_recipient"] = base64::encode(aesForRecipient);

    return entry;
}

bool FileStorage::saveConversationJson_NoLock(
    const std::filesystem::path& convoFile,
    const nlohmann::json& convo) {
    std::ofstream out(convoFile);
    if (!out.is_open()) {
        std::cerr << "[FileStorage] Failed to write conversation file: "
                  << convoFile << "\n";
        return false;
    }

    out << convo.dump(4);
    return true;
}

bool FileStorage::appendConversationMessage(
    const std::string& from,
    const std::string& to,
    const CryptoManager::AESEncrypted& ciphertext,
    const std::string& aesForSender,
    const std::string& aesForRecipient,
    long timestamp) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    // make conversation id and directory
    const std::string convoDirName =
    toFilesystemSafe(makeConversationId(from, to));
    const auto convoDir = std::filesystem::path(MESSAGE_PATH) / convoDirName;
    const auto convoFile = convoDir / "conversation.json";

    // check if directory exists
    std::error_code ec;
    std::filesystem::create_directories(convoDir, ec);
    if (ec) {
        std::cerr << "[FileStorage] Failed to create directory: "
                  << convoDir << " (" << ec.message() << ")\n";
        return false;
    }

    // load/initialise conversation
    nlohmann::json convo = loadConversationJson_NoLock(convoFile);

    // append message
    convo["messages"].push_back(
        buildMessageEntry_NoLock(
            from,
            to,
            ciphertext,
            aesForSender,
            aesForRecipient,
            timestamp
        )
    );

    return saveConversationJson_NoLock(convoFile, convo);
}

bool FileStorage::saveUser_NoLock() {
    std::ofstream file(userFilePath_);
    if (!file.is_open()) return false;

    file << data_.dump(4);
    return true;
}

bool FileStorage::saveUser() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    return saveUser_NoLock();
}

bool FileStorage::deleteUserJson_NoLock(const std::string& username) {
    auto& users = data_["users"];

    for (auto it = users.begin(); it != users.end(); ++it) {
        if ((*it)["username"] == username) {
            users.erase(it);
            return true;
        }
    }

    // nothing to delete
    return true;
}

bool FileStorage::deleteUserKeys_NoLock(const std::string& username) {
    std::error_code ec;
    std::string userKeyDir = std::string(KEY_PATH) + "/" + username;

    if (!std::filesystem::exists(userKeyDir, ec)) {
        // nothing to delete
        return true;
    }

    if (!std::filesystem::remove_all(userKeyDir, ec) || ec) {
        std::cerr << "[FileStorage] Failed to remove key dir '" << userKeyDir
                  << "': " << ec.message() << "\n";
        return false;
    }
    return true;
}

bool FileStorage::deleteUserConversations_NoLock(const std::string& username) {
    std::filesystem::path root = MESSAGE_PATH;
    std::error_code ec;

    if (!std::filesystem::exists(root, ec)) {
        // nothing to delete
        return true;
    }

    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_directory()) continue;

        const std::string dirName = entry.path().filename().string();

        try {
            const std::string convoId = fromFilesystemSafe(dirName);
            auto [a, b] = parseConversationId(convoId);

            if (a == username || b == username) {
                std::error_code ec2;
                std::filesystem::remove_all(entry.path(), ec2);
                if (ec2) {
                    std::cerr << "[FileStorage] Failed to delete conversation directory: "
                              << entry.path().string() << " (" << ec2.message() << ")\n";
                    return false;
                }
            }
        } catch (...) {
            continue;
        }
    }

    return true;
}

bool FileStorage::deleteUser(const std::string& username) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    // rollback safe delete everything
    bool json = deleteUserJson_NoLock(username);
    bool keys = deleteUserKeys_NoLock(username);
    bool convo = deleteUserConversations_NoLock(username);

    saveUser_NoLock();

    return json && keys && convo;
}

nlohmann::json FileStorage::loadConversation(const std::string& userA,
                                            const std::string& userB) {
    std::lock_guard<std::mutex> lock(file_mutex_);

    const auto convoFile = conversationFilePath_NoLock(userA, userB);

    std::ifstream in(convoFile);
    if (!in.is_open()) {
        // empty = no conversation
        return nlohmann::json();
    }

    nlohmann::json convoJson;
    try {
        in >> convoJson;
    } catch (...) {
        std::cerr << "[FileStorage] Invalid JSON in " << convoFile.string() << ", resetting\n";
        return nlohmann::json();
    }

    if (!convoJson.contains("messages")) {
        convoJson["messages"] = nlohmann::json::array();
    }

    return convoJson;
}
