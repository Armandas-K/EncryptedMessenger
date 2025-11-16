#ifndef ENCRYPTEDMESSENGER_LOGGER_H
#define ENCRYPTEDMESSENGER_LOGGER_H

#include <iostream>
#include <mutex>
#include <string>

class Logger {
public:
    // Thread-safe logging
    static void log(const std::string& msg) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::cout << msg << std::endl;
    }

private:
    static std::mutex mutex_;
};

#endif