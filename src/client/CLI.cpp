#include "client/CLI.h"
#include "utils/Logger.h"

// Constructor, initialises members
CLI::CLI(std::shared_ptr<Client> client)
    : client(std::move(client)), currentPage(Page::MAIN_MENU) {}

void CLI::run() {
    while (currentPage != Page::EXIT) {
        displayCurrentPage();
    }
}

// Show the page based on the currentPage enum
void CLI::displayCurrentPage() {
    switch (currentPage) {
        case Page::MAIN_MENU:
            showMainMenu();
            break;
        case Page::LOGIN:
            showLoginPage();
            break;
        case Page::CREATE_ACCOUNT:
            showCreateAccountPage();
            break;
        case Page::SEND_MESSAGE:
            showSendMessagePage();
            break;
        default:
            currentPage = Page::EXIT;
            break;
    }
}

// ----------- Main Menu Page -------------
void CLI::showMainMenu() {
    Logger::log("\n=== Encrypted Messenger ===\n");
    Logger::log("1. Log in\n");
    Logger::log("2. Create Account\n");
    Logger::log("3. Exit\n");
    int choice = getUserChoice(1, 3);
    handleMainMenuInput(choice);
}

void CLI::handleMainMenuInput(int choice) {
    switch (choice) {
        case 1: currentPage = Page::LOGIN; break;
        case 2: currentPage = Page::CREATE_ACCOUNT; break;
        case 3: currentPage = Page::EXIT; break;
    }
}

// ----------- Login Page -------------
void CLI::showLoginPage() {
    Logger::log("\n=== Login ===\n");
    Logger::log("1. Enter credentials\n");
    Logger::log("2. Back\n");
    int choice = getUserChoice(1, 2);
    handleLoginInput(choice);
}

void CLI::handleLoginInput(int choice) {
    switch (choice) {
        case 1: {
            std::string username, password;
            Logger::log("Username: ");
            std::cin >> username;
            Logger::log("Password: ");
            std::cin >> password;
            // Call your client’s login function
            if (client->login(username, password)) {
                Logger::log("Login successful!\n");
                currentPage = Page::SEND_MESSAGE;
            } else {
                Logger::log("Login failed.\n");
                currentPage = Page::MAIN_MENU;
            }
            break;
        }
        case 2:
            currentPage = Page::MAIN_MENU;
            break;
    }
}

// ----------- Create Account Page -------------
void CLI::showCreateAccountPage() {
    Logger::log("\n=== Create Account ===\n");
    Logger::log("1. Enter details\n");
    Logger::log("2. Back\n");
    int choice = getUserChoice(1, 2);
    handleCreateAccountInput(choice);
}

void CLI::handleCreateAccountInput(int choice) {
    switch (choice) {
        case 1: {
            std::string username, password;
            Logger::log("New username: ");
            std::cin >> username;
            Logger::log("New password: ");
            std::cin >> password;
            if (client->createAccount(username, password)) {
                Logger::log("Account created!\n");
            } else {
                Logger::log("Account creation failed.\n");
            }
            currentPage = Page::MAIN_MENU;
            break;
        }
        case 2:
            currentPage = Page::MAIN_MENU;
            break;
    }
}

// ----------- Send Message Page -------------
void CLI::showSendMessagePage() {
    Logger::log("\n=== Send Message ===\n");
    Logger::log("1. Send a message\n");
    Logger::log("2. Log out\n");
    int choice = getUserChoice(1, 2);
    handleSendMessageInput(choice);
}

void CLI::handleSendMessageInput(int choice) {
    switch (choice) {
        case 1: {
            std::string recipient, message;
            Logger::log("Recipient: ");
            std::cin >> recipient;
            std::cin.ignore();
            Logger::log("Message: ");
            std::getline(std::cin, message);
            client->sendMessage(recipient, message);
            Logger::log("Message sent!\n");
            break;
        }
        case 2:
            currentPage = Page::MAIN_MENU;
            break;
    }
}

// todo client function for getting conversations

void CLI::showConversationsPage() {
    Logger::log("\n=== Conversations ===\n");

    if (!client->getConversations()) {
        Logger::log("Failed to load conversations.\n");
        currentPage = Page::MAIN_MENU;
        return;
    }

    const auto& convos = client->getCachedConversations();
    for (size_t i = 0; i < convos.size(); ++i) {
        Logger::log(std::to_string(i + 1) + ". " + convos[i] + "\n");
    }

    Logger::log(std::to_string(convos.size() + 1) + ". Back\n");

    int choice = getUserChoice(1, convos.size() + 1);
    if (choice == convos.size() + 1) {
        currentPage = Page::SEND_MESSAGE;
    } else {
        client->selectConversation(convos[choice - 1]);
        currentPage = Page::VIEW_MESSAGES;
    }
}

void CLI::handleConversationsInput(int choice) {
}

void CLI::showMessagesPage() {
    Logger::log("\n=== Messages ===\n");

    const auto& msgs = client->getLastMessages();
    for (const auto& msg : msgs) {
        Logger::log(msg["from"].get<std::string>() + ": [encrypted]\n");
    }

    Logger::log("\n1. Send message\n");
    Logger::log("2. Back\n");

    int choice = getUserChoice(1, 2);
    handleMessagesInput(choice);
}

void CLI::handleMessagesInput(int choice) {
}

// ----------- Input Handling Helper -------------
int CLI::getUserChoice(int min, int max) {
    int choice;
    while (true) {
        Logger::log("> ");
        std::cin >> choice;
        if (std::cin.fail() || choice < min || choice > max) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            Logger::log("Invalid option. Try again.\n");
        } else {
            return choice;
        }
    }
}