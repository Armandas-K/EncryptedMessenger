#ifndef ENCRYPTEDMESSENGER_FILESTORAGE_H
#define ENCRYPTEDMESSENGER_FILESTORAGE_H

#include <string>
#include <json.hpp>
#include <mutex>
#include <fstream>

// manages user data stored in data/users.json
// provides thread-safe account creation and validation
class FileStorage {
public:
    FileStorage();

    // add new user, returns true if created successfully, false if username exists
    bool createUser(const std::string& username, const std::string& password_hash);

    // verify username and hashed password against users.json data
    bool validateUser(const std::string& username, const std::string& password_hash);

private:
    // load users.json data into memory
    bool load();

    // write users.json data to storage
    bool save();

private:
    // hardcoded path to user account file
    static constexpr const char* USERS_PATH = "data/users.json";
    nlohmann::json data_;      // in-memory cache of user credentials
    std::mutex file_mutex_;    // thread-safe access control for reads/writes
};

#endif //ENCRYPTEDMESSENGER_FILESTORAGE_H