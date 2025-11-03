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
    void start();

    // send a string message to the connected client.
    bool send(const std::string& message);

private:
    TcpConnection(asio::io_context& io_context, TcpServer* server);

    // begin asynchronous read operation for incoming messages.
    void readAction();

    // called when a complete json message is received.
    void handleAction(const nlohmann::json& message);

    // handler declarations
    void handleLogin(const nlohmann::json& data);
    void handleCreateAccount(const nlohmann::json& data);
    void handleSendMessage(const nlohmann::json& data);

    asio::ip::tcp::socket socket_;   // active socket for this client
    TcpServer* server_;              // reference to parent server
    std::array<char, 1024> buffer_;  // temp buffer for reads
};


#endif //ENCRYPTEDMESSENGER_TCPCONNECTION_H