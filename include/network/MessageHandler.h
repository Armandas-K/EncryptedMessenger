#ifndef ENCRYPTEDMESSENGER_MESSAGEHANDLER_H
#define ENCRYPTEDMESSENGER_MESSAGEHANDLER_H

#include <string>
#include "crypto/CryptoManager.h"
#include "network/tcpConnection.h"
#include "storage/FileStorage.h"

class TcpServer; // forward declaration

class MessageHandler {
public:
    MessageHandler(TcpServer* server, FileStorage& storage);

    // called by tcpServer for send message action
    bool processMessage(
        TcpConnection::pointer sender,
        const std::string& to,
        const std::string& message
    );

    //
    bool fetchMessages(TcpConnection::pointer requester, const std::string &withUser);

private:
    TcpServer* server_;       // not owned
    FileStorage& storage_;    // reference to storage system
    CryptoManager crypto_;    // encryption
};

#endif //ENCRYPTEDMESSENGER_MESSAGEHANDLER_H