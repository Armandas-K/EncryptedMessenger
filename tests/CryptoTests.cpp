#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "crypto/CryptoManager.h"
#include "utils/Logger.h"

// helper
static void logTest(const std::string& name) {
    Logger::log("\n[Test] " + name);
}

// ===================================================
// RSA TESTS
// ===================================================

void testRSAKeyGeneration() {
    logTest("RSA Key Generation");

    CryptoManager crypto;
    auto keys = crypto.generateRSAKeyPair();

    assert(!keys.publicKeyPem.empty());
    assert(!keys.privateKeyPem.empty());

    Logger::log("[PASS] RSA Key Generation");
}

void testRSAEncryptDecrypt() {
    logTest("RSA Encrypt/Decrypt");

    CryptoManager crypto;
    auto keys = crypto.generateRSAKeyPair();

    std::string plaintext = "Hello world!";
    std::string encrypted = crypto.rsaEncrypt(plaintext, keys.publicKeyPem);
    std::string decrypted = crypto.rsaDecrypt(encrypted, keys.privateKeyPem);

    assert(decrypted == plaintext);

    Logger::log("[PASS] RSA Encrypt/Decrypt");
}

// ===================================================
// AES TESTS
// ===================================================

void testAESRoundTrip() {
    logTest("AES-GCM Encrypt/Decrypt");

    CryptoManager crypto;

    std::string plaintext = "Hello world!";
    auto key = crypto.generateAESKey();

    auto encrypted = crypto.aesEncrypt(plaintext, key);
    std::string decrypted = crypto.aesDecrypt(
        key,
        encrypted.iv,
        encrypted.ciphertext,
        encrypted.tag
    );

    assert(decrypted == plaintext);

    Logger::log("[PASS] AES Encrypt/Decrypt");
}

void testAESRandomness() {
    logTest("AES Randomness");

    CryptoManager crypto;
    auto key = crypto.generateAESKey();

    std::string plaintext = "Same message";

    auto e1 = crypto.aesEncrypt(plaintext, key);
    auto e2 = crypto.aesEncrypt(plaintext, key);

    // iv or ciphertext should be different
    assert(e1.ciphertext != e2.ciphertext);

    Logger::log("[PASS] AES Randomness");
}

void testAESTamperDetection() {
    logTest("AES Tamper Detection");

    CryptoManager crypto;
    auto key = crypto.generateAESKey();

    std::string plaintext = "Sensitive data";
    auto encrypted = crypto.aesEncrypt(plaintext, key);

    // tamper with ciphertext
    encrypted.ciphertext[0] ^= 0xFF;

    bool failed = false;
    try {
        crypto.aesDecrypt(
            key,
            encrypted.iv,
            encrypted.ciphertext,
            encrypted.tag
        );
    } catch (...) {
        failed = true;
    }

    assert(failed && "AES-GCM should detect tampering");

    Logger::log("[PASS] AES Tamper Detection");
}

// ===================================================
// Main Entry
// ===================================================

int main() {
    Logger::log("=============================");
    Logger::log(" Running Crypto Unit Tests");
    Logger::log("=============================");

    testRSAKeyGeneration();
    testRSAEncryptDecrypt();
    testAESRoundTrip();
    testAESRandomness();
    testAESTamperDetection();

    Logger::log("\nAll crypto tests passed.\n");
    return 0;
}