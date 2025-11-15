#include "storage/FileStorage.h"
#include <iostream>

FileStorage::FileStorage() {
    load();
}

bool FileStorage::load() {
    std::lock_guard<std::mutex> lock(file_mutex_);
    std::ifstream file(userFilePath_);

    if (!file.is_open()) {
        std::cerr << "[FileStorage] Could not open users file, creating new one.\n";
        data_["users"] = nlohmann::json::array();
        file.close();
        save();
        return true;
    }

    // handle empty file
    if (file.peek() == std::ifstream::traits_type::eof()) {
        std::cerr << "[FileStorage] Empty users file, reinitializing.\n";
        data_["users"] = nlohmann::json::array();
        file.close();
        save();
        return true;
    }

    try {
        file >> data_;
    } catch (...) {
        std::cerr << "[FileStorage] Invalid JSON format in users file.\n";
        file.close();
        data_["users"] = nlohmann::json::array();
        save();
        return false;
    }

    return true;
}

bool FileStorage::save() {
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
    return save();
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

bool FileStorage::userExists(const std::string &username) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    for (const auto& user : data_["users"]) {
        if (user["username"] == username) {
            return true;
        }
    }
    return false;
}
