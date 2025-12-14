#include "crypto/CryptoManager.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <vector>
#include <iostream>

CryptoManager::CryptoManager() = default;

// -------------RSA KEY GENERATION-------------

CryptoManager::RSAKeyPair CryptoManager::generateRSAKeyPair() {
    RSAKeyPair kp;

    // create RSA key
    RSA* rsa = RSA_new();
    BIGNUM* bn = BN_new();
    BN_set_word(bn, RSA_F4);

    if (!RSA_generate_key_ex(rsa, 2048, bn, nullptr)) {
        BN_free(bn);
        RSA_free(rsa);
        throw std::runtime_error("RSA_generate_key_ex failed");
    }

    // write public key to PEM string
    {
        BIO* bio = BIO_new(BIO_s_mem());
        PEM_write_bio_RSA_PUBKEY(bio, rsa);

        char* data;
        long len = BIO_get_mem_data(bio, &data);
        kp.publicKeyPem.assign(data, len);

        BIO_free(bio);
    }

    // write private key to PEM string
    {
        BIO* bio = BIO_new(BIO_s_mem());
        PEM_write_bio_RSAPrivateKey(bio, rsa, nullptr, nullptr, 0, nullptr, nullptr);

        char* data;
        long len = BIO_get_mem_data(bio, &data);
        kp.privateKeyPem.assign(data, len);

        BIO_free(bio);
    }

    BN_free(bn);
    RSA_free(rsa);

    return kp;
}

// -------------RSA ENCRYPT-------------

std::string CryptoManager::rsaEncrypt(const std::string& plaintext,
                                      const std::string& publicKeyPem) {
    BIO* bio = BIO_new_mem_buf(publicKeyPem.data(), static_cast<int>(publicKeyPem.size()));
    RSA* pubKey = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!pubKey) {
        throw std::runtime_error("Failed to load public key PEM");
    }

    if (plaintext.size() > RSA_size(pubKey) - 42) {
        RSA_free(pubKey);
        throw std::runtime_error("RSA plaintext too large");
    }

    std::string output;
    output.resize(RSA_size(pubKey));

    int len = RSA_public_encrypt(
        plaintext.size(),
        (const unsigned char*)plaintext.data(),
        (unsigned char*)output.data(),
        pubKey,
        RSA_PKCS1_OAEP_PADDING
    );

    RSA_free(pubKey);

    if (len == -1)
        throw std::runtime_error("RSA_public_encrypt failed");

    output.resize(len);
    return output;
}

// -------------RSA DECRYPT-------------

std::string CryptoManager::rsaDecrypt(const std::string& ciphertext,
                                      const std::string& privateKeyPem) {
    BIO* bio = BIO_new_mem_buf(privateKeyPem.data(), static_cast<int>(privateKeyPem.size()));
    RSA* privKey = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);

    if (!privKey)
        throw std::runtime_error("Failed to load private key PEM");

    std::string output;
    output.resize(RSA_size(privKey));

    int len = RSA_private_decrypt(
        ciphertext.size(),
        (const unsigned char*)ciphertext.data(),
        (unsigned char*)output.data(),
        privKey,
        RSA_PKCS1_OAEP_PADDING
    );

    RSA_free(privKey);

    if (len == -1)
        throw std::runtime_error("RSA_private_decrypt failed");

    output.resize(len);
    return output;
}

// -------------AES KEY GENERATION-------------

std::vector<uint8_t> CryptoManager::generateAESKey(size_t keySize) {
    std::vector<uint8_t> key(keySize);
    if (RAND_bytes(key.data(), keySize) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    return key;
}

// -------------AES-GCM ENCRYPT-------------

CryptoManager::AESEncrypted CryptoManager::aesEncrypt(
    const std::string& plaintext,
    const std::vector<uint8_t>& key
) {
    AESEncrypted result;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();

    // Create 12-byte random IV (recommended for GCM)
    result.iv.resize(12);
    RAND_bytes(result.iv.data(), result.iv.size());

    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr))
        throw std::runtime_error("AES init failed");

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, result.iv.size(), nullptr);

    EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), result.iv.data());

    result.ciphertext.resize(plaintext.size());

    int outLen;
    if (!EVP_EncryptUpdate(ctx,
                       result.ciphertext.data(),
                       &outLen,
                       reinterpret_cast<const uint8_t*>(plaintext.data()),
                       plaintext.size())) {
        throw std::runtime_error("AES encrypt update failed");
    }

    int finalLen;
    EVP_EncryptFinal_ex(ctx, result.ciphertext.data() + outLen, &finalLen);

    result.ciphertext.resize(outLen + finalLen);

    // 16 byte GCM auth tag
    result.tag.resize(16);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, result.tag.data());

    EVP_CIPHER_CTX_free(ctx);

    return result;
}

// -------------AES-GCM DECRYPT-------------

std::string CryptoManager::aesDecrypt(const std::vector<uint8_t>& key,
                                      const std::vector<uint8_t>& iv,
                                      const std::vector<uint8_t>& ciphertext,
                                      const std::vector<uint8_t>& tag) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv.size(), nullptr);

    EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), iv.data());

    std::string plaintext(ciphertext.size(), '\0');

    int outLen;
    if (!EVP_DecryptUpdate(ctx,
                           (uint8_t*)plaintext.data(),
                           &outLen,
                           ciphertext.data(),
                           ciphertext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES decrypt update failed");
    }

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag.size(), (void*)tag.data());

    int finalLen = 0;
    if (!EVP_DecryptFinal_ex(ctx,
                             (uint8_t*)plaintext.data() + outLen,
                             &finalLen)) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("AES decrypt final failed (auth error)");
    }

    EVP_CIPHER_CTX_free(ctx);

    plaintext.resize(outLen + finalLen);
    return plaintext;
}