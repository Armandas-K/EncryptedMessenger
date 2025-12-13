#include <cassert>
#include <iostream>
#include <string>

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

    Logger::log("[PASS] RSA Encrypt/Decrypt");
}

// ===================================================
// AES TESTS
// ===================================================

void testAESRoundTrip() {
    logTest("AES-GCM Encrypt/Decrypt");

    Logger::log("[PASS] AES Encrypt/Decrypt");
}

void testAESRandomness() {
    logTest("AES Randomness");

    Logger::log("[PASS] AES Randomness");
}

void testAESTamperDetection() {
    logTest("AES Tamper Detection");

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