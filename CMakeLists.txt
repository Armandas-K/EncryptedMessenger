cmake_minimum_required(VERSION 3.28)
project(EncryptedMessenger LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ===================================================
# Include directories
# ===================================================
include_directories(
        ${PROJECT_SOURCE_DIR}/include
        ${PROJECT_SOURCE_DIR}/third_party/asio/include
        ${PROJECT_SOURCE_DIR}/third_party/json
)

# ===================================================
# Source groups
# ===================================================
file(GLOB_RECURSE CLIENT_SRC src/client/*.cpp)
file(GLOB_RECURSE SERVER_SRC src/server/*.cpp)
file(GLOB_RECURSE CRYPTO_SRC src/crypto/*.cpp)
file(GLOB_RECURSE NETWORK_SRC src/network/*.cpp)
file(GLOB_RECURSE STORAGE_SRC src/storage/*.cpp)
file(GLOB_RECURSE UTIL_SRC src/utils/*.cpp)

set(COMMON_SRC
        ${CRYPTO_SRC}
        ${NETWORK_SRC}
        ${STORAGE_SRC}
        ${UTIL_SRC}
)

# ===================================================
# OpenSSL via vcpkg
# ===================================================
find_package(OpenSSL REQUIRED)

# ===================================================
# Shared library
# ===================================================
add_library(messenger_common STATIC ${COMMON_SRC})
target_include_directories(messenger_common PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(messenger_common PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# ===================================================
# Executables
# ===================================================
add_executable(messenger_server src/main_server.cpp ${SERVER_SRC})
target_link_libraries(messenger_server PRIVATE messenger_common)

add_executable(messenger_client src/main_client.cpp ${CLIENT_SRC})
target_link_libraries(messenger_client PRIVATE messenger_common)

# ===================================================
# Test executables
# ===================================================
add_executable(test_crypto tests/CryptoTests.cpp)
target_link_libraries(test_crypto PRIVATE messenger_common)

add_executable(test_network tests/NetworkTests.cpp)
target_link_libraries(test_network PRIVATE messenger_common)