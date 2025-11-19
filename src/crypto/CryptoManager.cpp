#include "crypto/CryptoManager.h"

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <vector>
#include <iostream>

CryptoManager::CryptoManager() {
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
}

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