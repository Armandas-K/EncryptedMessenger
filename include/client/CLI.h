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
    SEND_MESSAGE,
    EXIT
};

class CLI {
public:
    explicit CLI(std::shared_ptr<Client> client);

    // Run the CLI loop
    void run();

private:
    std::shared_ptr<Client> client;
    Page currentPage;

    // Page logic
    void showMainMenu();
    void handleMainMenuInput(int choice);

    void showLoginPage();
    void handleLoginInput(int choice);

    void showCreateAccountPage();
    void handleCreateAccountInput(int choice);

    void showSendMessagePage();
    void handleSendMessageInput(int choice);

    void showConversationsPage();
    void handleConversationsInput(int choice);

    void showMessagesPage();
    void handleMessagesInput(int choice);

    // Helper
    void displayCurrentPage();
    int getUserChoice(int min, int max);
};
#endif //ENCRYPTEDMESSENGER_CLI_H