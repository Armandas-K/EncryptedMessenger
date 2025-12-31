# Encrypted-Messenger

## Overview

Encrypted Messenger is a client–server messaging application written in C++20.
The primary goal of the project is to explore how a secure messaging system can be built from the ground up using modern C++ and widely used libraries.

Rather than focusing on polish or production deployment, the project emphasizes understanding:

- Asynchronous TCP networking using Asio
- End-to-end encryption using OpenSSL
- Clear separation of responsibilities between client and server
- Practical issues around message storage, concurrency, and protocol design 

All message encryption and decryption happens on the client side. The server is treated as an untrusted relay that stores and forwards encrypted data without access to plaintext.

The project also includes a terminal-based client (CLI) and a suite of unit tests,
making it suitable both as a reference implementation and as a sandbox for experimenting with networking, cryptography, and system design in C++.

## Features

- Account creation and authentication
- RSA keypair generation per user
- End-to-end encrypted messaging (RSA + AES-GCM)
- Message are stored encrypted at rest
- Conversation listing
- Multi-client concurrent connections
- Terminal-based interactive client (CLI)
- Unit tests for crypto and networking

## Architecture

This project follows a client–server architecture with clear separation of responsibilities.

### High-level flow

1. Client connects to the server over TCP (Asio)
2. Users authenticate using hashed credentials
3. Messages are encrypted using:
  - AES-GCM for message content
  - RSA for encrypting the AES key for both sender and recipient
4. Server stores encrypted messages without access to plaintext
5. Clients fetch, decrypt, and display messages locally

### Components

- **Client**
  - Handles encryption/decryption
  - Manages user interaction (CLI)
  - Communicates with the server asynchronously

- **Server**
  - Accepts TCP connections
  - Validates requests
  - Stores encrypted data

- **CryptoManager**
  - RSA key generation
  - RSA encryption/decryption
  - AES-GCM encryption/decryption

- **FileStorage**
  - Manages users, keys, and conversations
  - Creates missing directories/files on first run

### Project structure

- `src/client/` – client logic and CLI
- `src/network/` – TCP server and connection abstractions
- `src/crypto/` – cryptographic primitives
- `src/storage/` – persistent storage logic
- `src/utils/` – useful helpers
- `tests/` – crypto and network tests
- `data/` – runtime-generated user and message data

## Dependencies

This project is written in C++20 and built with CMake. It uses a few third-party libraries that are vendored or installed using a package manager.

### Core tools

- C++ Compiler with C++20 support
    - Tested with MinGW-w64 / GCC 13.1.0 on Windows 11
- CMake minimum version 3.28

### Crypto & networking

- **OpenSSL** 3.6.0
  - Installed via vcpkg as `openssl:x64-mingw-dynamic` or `openssl:x64-windows`
  - Project uses CMake `find_package(OpenSSL REQUIRED)` and links:
    - `OpenSSL::SSL`
    - `OpenSSL::Crypto`
  - OpenSSL: https://github.com/openssl/openssl
  - vcpkg: https://github.com/microsoft/vcpkg

- **Asio** 1.36.0 (standalone, header-only, non-boost)
  - Included under `third_party/asio`
  - Upstream: https://github.com/chriskohlhoff/asio

### JSON

- **nlohmann/json** 3.12.0
  - Included under `third_party/json`
  - Upstream: https://github.com/nlohmann/json

### Platform specifics (Windows)

The project currently targets Windows 10/11 using MinGW-w64

- `_WIN32_WINNT` is set to 0x0A00 (Windows 10/11) for Asio
- Winsock libraries are linked so Asio TCP sockets work correctly
- `data/` paths are configured using compile-time macros:

      set(USER_DATA_PATH "${CMAKE_SOURCE_DIR}/data/users.json")
      set(KEY_DATA_PATH "${CMAKE_SOURCE_DIR}/data/keys")
      set(MESSAGE_DATA_PATH "${CMAKE_SOURCE_DIR}/data/messages")

At runtime, the program creates:

- `data/`
- `data/users.json`
- `data/keys/`
- `data/messages/`

## Building and Running

### 1. Clone the Repository

    git clone https://github.com/Armandas-K/EncryptedMessenger.git
    cd EncryptedMessenger

### 2. Install Dependencies

Requirements:

- **CMake >= 3.28**
- **C++20 compiler**
- **vcpkg** (for OpenSSL)

Vendored dependencies:

- Asio (standalone)
- nlohmann/json

### 2.1 Install vcpkg

    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    bootstrap-vcpkg.bat

### 2.2 Install OpenSSL via vcpkg

MinGW (recommended):

    vcpkg install openssl:x64-mingw-dynamic

MSVC:

    vcpkg install openssl:x64-windows

### 3. Configure the CMake Project

This project uses vcpkg for OpenSSL

#### Option A: Using CLion (recommended)

In **Settings / Build, Execution, Deployment / CMake**, add to CMake options:

    -DCMAKE_TOOLCHAIN_FILE=C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

#### Option B: Command Line

PowerShell:

    cmake -B build -S . `
      -DCMAKE_BUILD_TYPE=Debug `
      -DCMAKE_TOOLCHAIN_FILE="C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake" `
      -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

cmd:

    cmake -B build -S . ^
      -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_TOOLCHAIN_FILE="C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
      -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

### 4. Build the Project

Build everything:

    cmake --build build --config Debug

Executables will appear in `build/`:

- messenger_server.exe
- messenger_client.exe
- test_crypto.exe
- test_network.exe

### 5. Run Server and Client

Run server:

    cd build
    .\messenger_server.exe

Then on seperate terminal or CLion run client:

    .\messenger_client.exe

### 6. Running Tests

Run:

    .\test_crypto.exe
    .\test_network.exe

test_crypto tests:
- RSA/AES encryption
- AES randomness/tamper detection

test_network tests:
- account creation/login
- sending/storing messages
- multi-client connections