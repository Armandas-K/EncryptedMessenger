#ifndef ENCRYPTEDMESSENGER_CRYPTOMANAGER_H
#define ENCRYPTEDMESSENGER_CRYPTOMANAGER_H

#include <cstdint>
#include <string>
#include <vector>
#include <optional>

class CryptoManager {
public:
    CryptoManager();

    // ---------RSA keys---------

    // generate a new RSA keypair (returns public + private)
    struct RSAKeyPair {
        std::string publicKeyPem;
        std::string privateKeyPem;
    };

    RSAKeyPair generateRSAKeyPair();

    // get stored public key for user (from fileStorage)
    std::optional<std::string> getPublicKey(const std::string& username) const;

    // ---------RSA encryption---------

    // encrypt message using PEM public key
    std::string rsaEncrypt(const std::string& plaintext, const std::string& publicKeyPem);

    // decrypt message using PEM private key
    std::string rsaDecrypt(const std::string& ciphertext, const std::string& privateKeyPem);

    // ---------AES keys---------

    // Generate random AES key (32 bytes = AES-256)
    std::vector<uint8_t> generateAESKey(size_t keySize = 32);

    // ---------AES encryption---------

    struct AESEncrypted {
        std::vector<uint8_t> iv;
        std::vector<uint8_t> ciphertext;
        std::vector<uint8_t> tag;
    };

    // Encrypt with AES-GCM
    AESEncrypted aesEncrypt(const std::string& plaintext,
                            const std::vector<uint8_t>& key);

    std::string aesDecrypt(const std::vector<uint8_t>& key,
                           const std::vector<uint8_t>& iv,
                           const std::vector<uint8_t>& ciphertext,
                           const std::vector<uint8_t>& tag);

private:
    // cache keys for performance - could implement later
    // std::unordered_map<std::string, std::string> publicKeyCache;
};

#endif //ENCRYPTEDMESSENGER_CRYPTOMANAGER_H