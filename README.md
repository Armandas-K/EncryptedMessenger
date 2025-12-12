# Encrypted-Messenger

## Overview



## Features



## Architecture



## Dependencies

This project is written in C++20 and built with CMake. It uses a few third-party libraries that are vendored or installed via a package manager.

### Core tools

- **C++ Compiler** with C++20 support
    - Tested with **MinGW-w64 / GCC 13.1.0** on Windows 11
- **CMake** ≥ 3.28

### Crypto & networking

- **OpenSSL** 3.6.0
    - Installed via **vcpkg** as `openssl:x64-mingw-dynamic` or `openssl:x64-windows`
    - Project uses CMake’s `find_package(OpenSSL REQUIRED)` and links against:
        - `OpenSSL::SSL`
        - `OpenSSL::Crypto`
    - OpenSSL project: https://github.com/openssl/openssl
    - vcpkg: https://github.com/microsoft/vcpkg

- **Asio** (standalone, header-only, non-Boost)
    - Included under `third_party/asio`
    - Tested with Asio **1.36.x** using (`asio::io_context`, `asio::ip::tcp::socket`, etc.)
    - Upstream: https://github.com/chriskohlhoff/asio

### JSON

- **nlohmann/json** 3.12.0
    - Included under `third_party/json`
    - Header `json.hpp` is used and made available via:
        - `#include "json.hpp"`
    - Upstream: https://github.com/nlohmann/json

### Platform specifics (Windows)

The project currently targets Windows 10/11 with MinGW-w64 / GCC.

Key points:

- `_WIN32_WINNT` is set to **0x0A00** (Windows 10/11) for Asio:

      add_compile_definitions(_WIN32_WINNT=0x0A00)

- Winsock libraries are linked so Asio’s TCP sockets work correctly:

      target_link_libraries(messenger_common
          PRIVATE
          OpenSSL::SSL
          OpenSSL::Crypto
          ws2_32
          mswsock
      )

- `data/` is configured using compile-time macros:

      set(USER_DATA_PATH "${CMAKE_SOURCE_DIR}/data/users.json")
      set(KEY_DATA_PATH "${CMAKE_SOURCE_DIR}/data/keys")
      set(MESSAGE_DATA_PATH "${CMAKE_SOURCE_DIR}/data/messages")

      add_compile_definitions(USERS_PATH="${USER_DATA_PATH}")
      add_compile_definitions(KEY_PATH="${KEY_DATA_PATH}")
      add_compile_definitions(MESSAGE_PATH="${MESSAGE_DATA_PATH}")

At runtime the program automatically creates:

- data/
- data/users.json
- data/keys/
- data/messages/

## Building and Running

### 1. Clone the Repository

Clone the repository:

    git clone https://github.com/Armandas-K/EncryptedMessenger.git
    cd EncryptedMessenger

### 2. Install Dependencies

Requirements:

- **CMake ≥ 3.28**
- **C++20 compiler** (MinGW-w64 recommended)
- **vcpkg** (used for OpenSSL)
- Vendored dependencies:
    - Asio (standalone) — third_party/asio
    - nlohmann/json — third_party/json

### 2.1 Install vcpkg

    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    bootstrap-vcpkg.bat

### 2.2 Install OpenSSL via vcpkg

MSVC triplet:

    vcpkg install openssl:x64-windows

MinGW example:

    vcpkg install openssl:x64-mingw-dynamic

Asio and JSON are bundled in the project

### 3. Configure the CMake Project

Run CMake and point it at your vcpkg toolchain file:

PowerShell:

    cmake -B build -S . `
      -DCMAKE_BUILD_TYPE=Debug `
      -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"

cmd:

    cmake -B build -S . ^
      -DCMAKE_BUILD_TYPE=Debug ^
      -DCMAKE_TOOLCHAIN_FILE="C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake"

This CMake project:

- Adds include paths
- Builds a static library: messenger_common
- Builds executables:
  - messenger_server
  - messenger_client
  - test_crypto
  - test_network
- Defines macros:
  - USERS_PATH
  - KEY_PATH
  - MESSAGE_PATH

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
    ./messenger_server.exe

Run client:

    cd build
    ./messenger_client.exe

### 6. Running Tests

Run:

    ./test_crypto.exe
    ./test_network.exe

test_crypto tests:
- RSA/AES encryption
- hashing functions

test_network tests:
- account creation/login
- sending/storing messages
- multi-client connections