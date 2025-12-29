# Encrypted-Messenger

## Overview



## Features



## Architecture



## Dependencies

This project is written in C++20 and built with CMake. It uses a few third-party libraries that are vendored or installed using a package manager

### Core tools

- C++ Compiler with C++20 support
    - Tested with MinGW-w64 / GCC 13.1.0 on Windows 11
- CMake minimum version 3.28

### Crypto & networking

- **OpenSSL** 3.6.0
    - Installed via vcpkg as `openssl:x64-mingw-dynamic` or `openssl:x64-windows`
    - Project uses CMakes `find_package(OpenSSL REQUIRED)` and links:
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

The project currently works for Windows 10/11 with MinGW-w64

- `_WIN32_WINNT` is set to 0x0A00 (Windows 10/11) for Asio

- Winsock libraries are linked so Asio’s TCP sockets work correctly

- `data/` is configured using compile-time macros:

      set(USER_DATA_PATH "${CMAKE_SOURCE_DIR}/data/users.json")
      set(KEY_DATA_PATH "${CMAKE_SOURCE_DIR}/data/keys")
      set(MESSAGE_DATA_PATH "${CMAKE_SOURCE_DIR}/data/messages")

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

- **CMake >= 3.28**
- **C++20 compiler** (MinGW-w64 recommended)
- **vcpkg** (for OpenSSL)
- Vendored dependencies:
    - Asio (standalone)
    - nlohmann/json

### 2.1 Install vcpkg

    git clone https://github.com/microsoft/vcpkg.git
    cd vcpkg
    bootstrap-vcpkg.bat

### 2.2 Install OpenSSL via vcpkg

MinGW (Recommended):

    vcpkg install openssl:x64-mingw-dynamic

MSVC:

    vcpkg install openssl:x64-windows


### 3. Configure the CMake Project

This project uses vcpkg for OpenSSL

#### Option A: Using CLion (Recommended)

In **Settings / Build, Execution, Deployment / CMake**, add to CMake options:

    -DCMAKE_TOOLCHAIN_FILE=C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic

#### Option B: Command Line

PowerShell:

```powershell
cmake -B build -S . `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic
```

cmd:

```cmd
cmake -B build -S . ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_TOOLCHAIN_FILE="C:/Users/user/vcpkg/scripts/buildsystems/vcpkg.cmake" ^
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic
```

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