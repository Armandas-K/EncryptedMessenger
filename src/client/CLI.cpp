#include "client/CLI.h"
#include "utils/Logger.h"

CLI::CLI(std::shared_ptr<Client> client)
    : client(std::move(client)), currentPage(Page::MAIN_MENU) {}

void CLI::run() {
    while (currentPage != Page::EXIT) {
        displayCurrentPage();
    }
}

// show the page based on the currentPage enum
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
        case Page::CONVERSATIONS:
            showConversationsPage();
            break;
        case Page::VIEW_MESSAGES:
            showMessagesPage();
            break;
        default:
            currentPage = Page::EXIT;
            break;
    }
}

// Main Menu Page
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

// Login Page
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

            if (client->login(username, password)) {
                Logger::log("Login successful!\n");
                currentPage = Page::CONVERSATIONS;
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

// Create Account Page
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

// Send Message Page
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

void CLI::showConversationsPage() {
    Logger::log("\n=== Conversations ===\n");
    Logger::log("Fetching conversations...\n");

    if (!client->getConversations()) {
        Logger::log("Failed to load conversations.\n");
        currentPage = Page::MAIN_MENU;
        return;
    }

    auto conversations = client->getCachedConversations();

    if (conversations.empty()) {
        Logger::log("No conversations found.\n");
        Logger::log("1. Back\n");
        int choice = getUserChoice(1, 1);
        (void)choice;
        currentPage = Page::MAIN_MENU;
        return;
    }

    for (size_t i = 0; i < conversations.size(); ++i) {
        Logger::log(std::to_string(i + 1) + ". " + conversations[i] + "\n");
    }
    Logger::log(std::to_string(conversations.size() + 1) + ". Back\n");

    int choice = getUserChoice(1, static_cast<int>(conversations.size() + 1));

    if (choice == static_cast<int>(conversations.size() + 1)) {
        currentPage = Page::MAIN_MENU;
        return;
    }

    activeChatUser_ = conversations[choice - 1];
    currentPage = Page::VIEW_MESSAGES;
}

void CLI::handleConversationsInput(int choice) {
}

// todo still currently shows encrypted metadata
void CLI::showMessagesPage() {
    Logger::log("\n=== Messages with " + activeChatUser_ + " ===\n");
    Logger::log("Fetching messages...\n");

    if (!client->getMessages(activeChatUser_)) {
        Logger::log("Failed to load messages.\n");
        currentPage = Page::CONVERSATIONS;
        return;
    }

    auto messages = client->getCachedMessages();

    if (messages.empty()) {
        Logger::log("No messages in this conversation.\n");
    } else {
        for (const auto& m : messages) {
            std::string from = m.value("from", "");
            long ts = m.value("timestamp", 0L);

            Logger::log(from + " [" + std::to_string(ts) + "]: [encrypted]\n");
        }
    }

    Logger::log("\n1. Refresh\n");
    Logger::log("2. Send message\n");
    Logger::log("3. Back\n");

    int choice = getUserChoice(1, 3);
    handleMessagesInput(choice);
}

void CLI::handleMessagesInput(int choice) {
    switch (choice) {
        case 1:
            // refresh by re-rendering this page
            currentPage = Page::VIEW_MESSAGES;
            break;

        case 2: {
            std::string text;
            Logger::log("Message: ");
            std::cin.ignore();
            std::getline(std::cin, text);

            if (client->sendMessage(activeChatUser_, text)) {
                Logger::log("Message sent.\n");
            } else {
                Logger::log("Failed to send message.\n");
            }

            currentPage = Page::VIEW_MESSAGES;
            break;
        }

        case 3:
            currentPage = Page::CONVERSATIONS;
            break;
    }
}

// input helper
int CLI::getUserChoice(int min, int max) {
    int choice;
    while (true) {
        std::cout << "> ";
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