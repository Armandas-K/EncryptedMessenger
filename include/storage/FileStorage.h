#ifndef ENCRYPTEDMESSENGER_FILESTORAGE_H
#define ENCRYPTEDMESSENGER_FILESTORAGE_H

#include <string>
#include <json.hpp>
#include <mutex>
#include <fstream>
#include "crypto/CryptoManager.h"

// manages user data stored in data/users.json
// provides thread-safe account creation and validation
class FileStorage {
public:
    FileStorage();

    // wrapper for createUser atomic operation
    bool createUser(const std::string &username, const std::string &password_hash);

    // add new user, returns true if created successfully, false if username exists
    bool createUser_NoLock(const std::string& username, const std::string& password_hash);

    // make keys for encryption on account creation
    bool createUserKeyFiles_NoLock(const std::string& username);

    // verify username and hashed password against users.json data
    bool loginUser(const std::string& username, const std::string& password_hash);

    // return public key PEM for user, or empty string on failure
    std::string getUserPublicKey(const std::string& username);

    // check if username is taken
    bool userExists_NoLock(const std::string &username);
    bool userExists(const std::string & username);

    // get all conversations user is in
    nlohmann::json listConversations(const std::string &username);

    nlohmann::json loadConversationJson_NoLock(const std::filesystem::path &convoFile);

    nlohmann::json buildMessageEntry_NoLock(const std::string &from, const std::string &to,
                                            const CryptoManager::AESEncrypted &ciphertext,
                                            const std::string &aesForSender,
                                            const std::string &aesForRecipient, long timestamp);

    bool saveConversationJson_NoLock(const std::filesystem::path &convoFile, const nlohmann::json &convo);

    // append to message json shared between 2 users
    bool appendConversationMessage(
    const std::string& from,
    const std::string& to,
    const CryptoManager::AESEncrypted& ciphertext,
    const std::string& aesForSender,
    const std::string& aesForRecipient,
    long timestamp
    );

    // write users.json data to storage
    bool saveUser_NoLock();
    bool saveUser();

    // danger
    bool deleteUserJson_NoLock(const std::string& username);
    bool deleteUserKeys_NoLock(const std::string& username);
    bool deleteUserConversations_NoLock(const std::string& username);
    bool deleteUser(const std::string &username);

    // get message json shared between 2 users
    nlohmann::json loadConversation(const std::string &userA, const std::string &userB);

    // allow tcpServer to access mutex
    std::mutex& mutex() { return file_mutex_; }

private:
    // if data, keys, and messages directories are missing
    void initializeDirectories();
    // load users.json data into memory
    bool loadUser();

    // helpers for making/parsing conversation directory names
    std::string makeConversationId(const std::string &a, const std::string &b);
    auto parseConversationId(const std::string &id) -> std::pair<std::string, std::string>;
    std::filesystem::path conversationDirPath_NoLock(const std::string &userA, const std::string &userB);
    std::filesystem::path conversationFilePath_NoLock(const std::string &userA, const std::string &userB);


    // convert between base64url and string (used in conv dirs)
    std::string toFilesystemSafe(const std::string &input);
    std::string fromFilesystemSafe(const std::string &safe);

private:
    // hardcoded path to user account file
    std::string userFilePath_ = USERS_PATH;
    nlohmann::json data_;      // in-memory cache of user credentials
    std::mutex file_mutex_;    // thread-safe access control for reads/writes
};

#endif //ENCRYPTEDMESSENGER_FILESTORAGE_H