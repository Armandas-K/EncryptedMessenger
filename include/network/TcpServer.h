#ifndef ENCRYPTEDMESSENGER_TCPSERVER_H
#define ENCRYPTEDMESSENGER_TCPSERVER_H

#include <asio.hpp>
#include <memory>
#include "network/tcpConnection.h"

class TcpServer {
public:
    TcpServer(asio::io_context& io_context, unsigned short port);

    void startAccept();

private:
    void handleAccept(TcpConnection::pointer new_connection, const std::error_code& error);

    asio::ip::tcp::acceptor acceptor_;
};

#endif //ENCRYPTEDMESSENGER_TCPSERVER_H