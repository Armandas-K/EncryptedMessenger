#include "client/CLI.h"
#include "utils/Logger.h"

CLI::CLI(asio::io_context& io,
         const std::string& host,
         unsigned short port)
    : currentPage_(Page::MAIN_MENU) {
    connection_ = TcpConnection::create(io, nullptr);

    if (!connection_->connect(host, port)) {
        throw std::runtime_error("Failed to connect to server");
    }

    connection_->beginRead();
    client_ = std::make_shared<Client>(connection_);

    Logger::log("[CLI] Connected to server");
}

void CLI::run() {
    while (currentPage_ != Page::EXIT) {
        displayCurrentPage();
    }
}

// show the page based on the currentPage enum
void CLI::displayCurrentPage() {
    switch (currentPage_) {
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
            currentPage_ = Page::EXIT;
            break;
    }
}

// Main Menu Page
void CLI::showMainMenu() {
    Logger::log("\n=== Encrypted Messenger ===");
    Logger::log("1. Log in");
    Logger::log("2. Create Account");
    Logger::log("3. Exit");
    int choice = getUserChoice(1, 3);
    handleMainMenuInput(choice);
}

void CLI::handleMainMenuInput(int choice) {
    switch (choice) {
        case 1: currentPage_ = Page::LOGIN; break;
        case 2: currentPage_ = Page::CREATE_ACCOUNT; break;
        case 3: currentPage_ = Page::EXIT; break;
    }
}

// Login Page
void CLI::showLoginPage() {
    Logger::log("\n=== Login ===");
    Logger::log("1. Enter credentials");
    Logger::log("2. Back");
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

            if (client_->login(username, password)) {
                Logger::log("Login successful");
                currentPage_ = Page::CONVERSATIONS;
            } else {
                Logger::log("Login failed");
                currentPage_ = Page::MAIN_MENU;
            }
            break;
        }
        case 2:
            currentPage_ = Page::MAIN_MENU;
            break;
    }
}

// Create Account Page
void CLI::showCreateAccountPage() {
    Logger::log("\n=== Create Account ===");
    Logger::log("1. Enter details");
    Logger::log("2. Back");
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
            if (client_->createAccount(username, password)) {
                Logger::log("Account created");
            } else {
                Logger::log("Account creation failed");
            }
            currentPage_ = Page::MAIN_MENU;
            break;
        }
        case 2:
            currentPage_ = Page::MAIN_MENU;
            break;
    }
}

// Send Message Page
void CLI::showSendMessagePage() {
    Logger::log("\n=== Send Message ===");
    Logger::log("1. Send a message");
    Logger::log("2. Log out");
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
            client_->sendMessage(recipient, message);
            Logger::log("Message sent");
            break;
        }
        case 2:
            currentPage_ = Page::MAIN_MENU;
            break;
    }
}

void CLI::showConversationsPage() {
    Logger::log("\n=== Conversations ===");
    Logger::log("Fetching conversations...");

    if (!client_->getConversations()) {
        Logger::log("Failed to load conversations");
        currentPage_ = Page::MAIN_MENU;
        return;
    }

    auto conversations = client_->getCachedConversations();

    if (conversations.empty()) {
        Logger::log("No conversations found");
        Logger::log("1. Back");
        int choice = getUserChoice(1, 1);
        (void)choice;
        currentPage_ = Page::MAIN_MENU;
        return;
    }

    for (size_t i = 0; i < conversations.size(); ++i) {
        Logger::log(std::to_string(i + 1) + ". " + conversations[i] + "");
    }
    Logger::log(std::to_string(conversations.size() + 1) + ". Back");

    int choice = getUserChoice(1, static_cast<int>(conversations.size() + 1));

    if (choice == static_cast<int>(conversations.size() + 1)) {
        currentPage_ = Page::MAIN_MENU;
        return;
    }

    activeChatUser_ = conversations[choice - 1];
    currentPage_ = Page::VIEW_MESSAGES;
}

void CLI::handleConversationsInput(int choice) {
}

void CLI::showMessagesPage() {
    Logger::log("\n=== Messages with " + activeChatUser_ + " ===");
    Logger::log("Fetching messages...");

    if (!client_->getMessages(activeChatUser_)) {
        Logger::log("Failed to load messages");
        currentPage_ = Page::CONVERSATIONS;
        return;
    }

    // messages in plaintext
    auto messages = client_->getDecryptedMessages();

    if (messages.empty()) {
        Logger::log("No messages in this conversation");
    } else {
        for (const auto& text : messages) {
            Logger::log(text);
        }
    }

    Logger::log("\n1. Refresh");
    Logger::log("2. Send message");
    Logger::log("3. Back");

    int choice = getUserChoice(1, 3);
    handleMessagesInput(choice);
}

void CLI::handleMessagesInput(int choice) {
    switch (choice) {
        case 1:
            // refresh by re-rendering this page
            currentPage_ = Page::VIEW_MESSAGES;
            break;

        case 2: {
            std::string text;
            Logger::log("Message: ");
            std::cin.ignore();
            std::getline(std::cin, text);

            if (client_->sendMessage(activeChatUser_, text)) {
                Logger::log("Message sent");
            } else {
                Logger::log("Failed to send message");
            }

            currentPage_ = Page::VIEW_MESSAGES;
            break;
        }

        case 3:
            currentPage_ = Page::CONVERSATIONS;
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
            Logger::log("Invalid option. Try again");
        } else {
            return choice;
        }
    }
}