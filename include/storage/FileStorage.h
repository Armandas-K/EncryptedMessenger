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

    // add new user, returns true if created successfully, false if username exists
    bool createUser(const std::string& username, const std::string& password_hash);

    // make keys for encryption on account creation
    bool createUserKeyFiles(const std::string& username);

    // verify username and hashed password against users.json data
    bool loginUser(const std::string& username, const std::string& password_hash);

    // check if username is taken
    bool userExists(const std::string & username);

private:
    // load users.json data into memory
    bool loadUser();

    // write users.json data to storage
    bool saveUser();

private:
    // hardcoded path to user account file
    std::string userFilePath_ = USERS_PATH;
    nlohmann::json data_;      // in-memory cache of user credentials
    std::mutex file_mutex_;    // thread-safe access control for reads/writes
};

#endif //ENCRYPTEDMESSENGER_FILESTORAGE_H