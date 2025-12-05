#ifndef ENCRYPTEDMESSENGER_TCPCONNECTION_H
#define ENCRYPTEDMESSENGER_TCPCONNECTION_H
#include <asio.hpp>
#include <memory>
#include <string>
#include <array>
#include <json.hpp>

class TcpServer; // Forward declaration


// represents a single TCP client connection.
// handles reading, writing, and parsing of json messages.
// owned and managed by TcpServer.
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
public:
    using pointer = std::shared_ptr<TcpConnection>;

    // factory method to create a new shared TcpConnection instance.
    static pointer create(asio::io_context& io_context, TcpServer* server);

    // get a reference to the socket so the server can accept connections into it.
    asio::ip::tcp::socket& socket();

    // start asynchronous reading from the connection.
    bool beginRead();

    // send a string message from connected client.
    void send(const std::string& message);

    // set username when user logs in
    void setUsername(const std::string& username) { username_ = username; }

    // return username of connected client
    const std::string& getUsername() const { return username_; }

    // connect to socket
    bool connect(const std::string& host, int port);

    // close the connection and notify the server.
    void disconnect();

private:
    TcpConnection(asio::io_context& io_context, TcpServer* server);

    // begin asynchronous read operation for incoming messages.
    void readAction();

    // called when a complete json message is received.
    void handleAction(const nlohmann::json& message);

    std::string username_;           // assign when login
    asio::ip::tcp::socket socket_;   // active socket for this client
    asio::io_context& io_context_;   // used for I/O
    TcpServer* server_;              // reference to parent server
    std::array<char, 1024> buffer_;  // temp buffer for reads
};

#endif //ENCRYPTEDMESSENGER_TCPCONNECTION_H