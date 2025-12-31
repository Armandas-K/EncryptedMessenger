#ifndef ENCRYPTEDMESSENGER_KEYSTORE_H
#define ENCRYPTEDMESSENGER_KEYSTORE_H

#include <string>
#include <filesystem>

class KeyStore {
public:
    explicit KeyStore(const std::string& username);

    // create clients key dir if needed
    void ensureKeyDirectory();

    // load keys
    std::string loadPrivateKey() const;
    std::string loadPublicKey() const;

    // save keys
    void savePrivateKey(const std::string& pem);
    void savePublicKey(const std::string& pem);

    // for safety
    bool hasKeyPair() const;

private:
    // dir same as exec
    std::filesystem::path baseDir_;
    std::filesystem::path userDir_;
};

#endif //ENCRYPTEDMESSENGER_KEYSTORE_H