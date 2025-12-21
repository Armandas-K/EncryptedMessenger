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

    // called by tcpServer for receive message action
    bool fetchMessages(TcpConnection::pointer requester, const std::string &withUser);

    // called by tcpServer for get conversations action
    bool fetchConversations(TcpConnection::pointer requester);

private:
    TcpServer* server_;       // not owned
    FileStorage& storage_;    // reference to storage system
    CryptoManager crypto_;    // encryption
};

#endif //ENCRYPTEDMESSENGER_MESSAGEHANDLER_H