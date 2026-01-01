#include "client/KeyStore.h"
#include <fstream>
#include <stdexcept>

KeyStore::KeyStore(const std::string& username) {
    baseDir_ = std::filesystem::current_path() / "client_data" / "keys";
    userDir_ = baseDir_ / username;
}

void KeyStore::ensureKeyDirectory() {
    std::error_code ec;
    std::filesystem::create_directories(userDir_, ec);
    if (ec) {
        throw std::runtime_error("Failed to create key directory: " + ec.message());
    }
}

std::filesystem::path KeyStore::privateKeyPath() const {
    return userDir_ / "private.pem";
}

std::filesystem::path KeyStore::publicKeyPath() const {
    return userDir_ / "public.pem";
}

bool KeyStore::hasKeyPair() const {
    return std::filesystem::exists(privateKeyPath()) &&
           std::filesystem::exists(publicKeyPath());
}

std::string KeyStore::loadPrivateKey() const {
    std::ifstream file(privateKeyPath(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Private key not found");
    }
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

std::string KeyStore::loadPublicKey() const {
    std::ifstream file(publicKeyPath(), std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Public key not found");
    }
    return std::string(
        std::istreambuf_iterator<char>(file),
        std::istreambuf_iterator<char>()
    );
}

void KeyStore::savePrivateKey(const std::string& pem) {
    ensureKeyDirectory();
    std::ofstream out(privateKeyPath(), std::ios::binary);
    out << pem;
}

void KeyStore::savePublicKey(const std::string& pem) {
    ensureKeyDirectory();
    std::ofstream out(publicKeyPath(), std::ios::binary);
    out << pem;
}