#ifndef ENCRYPTEDMESSENGER_CLI_H
#define ENCRYPTEDMESSENGER_CLI_H
#include <memory>
#include <string>
#include <iostream>
#include "Client.h"

enum class Page {
    MAIN_MENU,
    LOGIN,
    CREATE_ACCOUNT,
    EXIT,
    CONVERSATIONS,
    VIEW_MESSAGES
};

class CLI {
public:
    CLI(asio::io_context& io,
        const std::string& host,
        unsigned short port);

    // run CLI loop
    void run();

private:
    std::shared_ptr<TcpConnection> connection_;
    std::shared_ptr<Client> client_;
    Page currentPage_;

    // page logic
    void showMainMenu();
    void handleMainMenuInput(int choice);

    void showLoginPage();
    void handleLoginInput(int choice);

    void showCreateAccountPage();
    void handleCreateAccountInput(int choice);

    void showConversationsPage();
    void handleConversationsInput(int choice);

    void showMessagesPage();
    void handleMessagesInput(int choice);

    // for showMessagesPage auto refresh
    void renderMessagesOnce();
    void startMessagePolling();
    void stopMessagePolling();

    // helpers
    void displayCurrentPage();
    int getUserChoice(int min, int max);

    // username of other user in currently viewed conversation
    std::string activeChatUser_;

    // starts and stops background message polling
    std::atomic<bool> pollingMessages_{false};
    // background thread that fetches messages
    std::thread messagePoller_;
    // time between message fetches
    static constexpr int fetchIntervalMs_ = 1000;
};
#endif //ENCRYPTEDMESSENGER_CLI_H