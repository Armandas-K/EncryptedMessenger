#ifndef ENCRYPTEDMESSENGER_TCPSERVER_H
#define ENCRYPTEDMESSENGER_TCPSERVER_H

#include <asio.hpp>
#include <memory>
#include <vector>
#include "client/MessageHandler.h"
#include "network/tcpConnection.h"
#include "storage/FileStorage.h"

// manages incoming TCP connections and delegates handling to TcpConnection.
// responsible for accepting new clients and maintaining a list of active connections.
class TcpServer {
public:
    // construct server on given io_context and port number.
    TcpServer(asio::io_context& io_context, unsigned short port);

    // start listening for new incoming connections.
    void startAccept();

    // handle completion of an asynchronous accept operation.
    void handleAccept(TcpConnection::pointer new_connection, const std::error_code& error);

    // decides which function to call from given action
    void handleAction(TcpConnection::pointer connection, const nlohmann::json& message);

    // remove a connection from the active list (called when a client disconnects).
    void removeConnection(TcpConnection::pointer connection);

private:
    // handler declarations
    void handleCreateAccount(TcpConnection::pointer connection, const nlohmann::json &data);
    void handleLogin(TcpConnection::pointer connection, const nlohmann::json &data);
    void handleSendMessage(TcpConnection::pointer connection, const nlohmann::json &data);

    asio::io_context& io_context_;                           // reference to shared io_context
    asio::ip::tcp::acceptor acceptor_;                       // accepts incoming connections
    std::vector<TcpConnection::pointer> active_connections_; // active connected clients
    FileStorage storage_;                                    // write to user storage
    MessageHandler messageHandler_;                          // handle message functionality
};

#endif //ENCRYPTEDMESSENGER_TCPSERVER_H