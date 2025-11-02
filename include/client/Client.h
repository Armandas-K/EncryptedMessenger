#ifndef ENCRYPTEDMESSENGER_CLIENT_H
#define ENCRYPTEDMESSENGER_CLIENT_H
#include <string>


class Client {
private:


public:
    Client() = default;

    void sendMessage(std::string recipient, std::string message) {
        // send msg
    }

    bool createAccount(const std::string& username, const std::string & password) {

        return false;
    }

    bool login(const std::string& username, const std::string& password) {

        return false;
    }
};


#endif //ENCRYPTEDMESSENGER_CLIENT_H