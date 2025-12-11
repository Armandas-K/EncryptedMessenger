# Encrypted-Messenger

## Overview



## Features



## Architecture



## Dependencies

This project is written in C++ 20 and built with CMake. It uses a few third-party libraries that are vendored or installed via a package manager.

### Core tools

- **C++ Compiler** with C++ 20 support
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
    - Tested with Asio **1.36.x** style API (`asio::io_context`, `asio::ip::tcp::socket`, etc.)
    - Upstream: https://github.com/chriskohlhoff/asio

### JSON

- **nlohmann/json** 3.12.0
    - Included under `third_party/json`
    - Header `json.hpp` is used and made available via:
        - `#include "json.hpp"`
    - Upstream: https://github.com/nlohmann/json

### Platform specifics (Windows)

The project currently targets **Windows 10/11** with MinGW-w64 / GCC.

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