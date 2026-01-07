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

void CLI::showConversationsPage() {
    Logger::log("\n=== Conversations ===");
    Logger::log("1. Start new conversation");
    Logger::log("2. Refresh");
    Logger::log("3. Log out");

    if (!client_->getConversations()) {
            Logger::log("Failed to load conversations");
            currentPage_ = Page::MAIN_MENU;
            return;
    }

    auto conversations = client_->getCachedConversations();

    for (size_t i = 0; i < conversations.size(); ++i) {
        Logger::log(std::to_string(i + 4) + ". " + conversations[i]);
    }

    int choice = getUserChoice(1, static_cast<int>(conversations.size() + 3));
    handleConversationsInput(choice);
}

void CLI::handleConversationsInput(int choice) {
    auto conversations = client_->getCachedConversations();

    if (choice == 1) {
        std::string user;
        Logger::log("Enter username to chat with: ");
        std::cin >> user;

        if (user == client_->getUsername()) {
            Logger::log("Cannot start conversation with yourself");
            return;
        }

        if (!client_->userExists(user)) {
            Logger::log("User does not exist");
            currentPage_ = Page::CONVERSATIONS;
            return;
        }

        activeChatUser_ = user;
        currentPage_ = Page::VIEW_MESSAGES;
        return;
    }

    if (choice == 2) {
        client_->getConversations();
        currentPage_ = Page::CONVERSATIONS;
        return;
    }

    if (choice == 3) {
        client_->logout();
        currentPage_ = Page::MAIN_MENU;
        return;
    }

    // existing conversation
    size_t index = choice - 4;
    if (index < conversations.size()) {
        stopMessagePolling();
        activeChatUser_ = conversations[index];
        client_->clearCachedMessages();
        currentPage_ = Page::VIEW_MESSAGES;
    }
}

void CLI::showMessagesPage() {
    Logger::log("\n=== Messages with " + activeChatUser_ + " ===");

    if (activeChatUser_.empty()) {
        Logger::log("Invalid conversation");
        currentPage_ = Page::CONVERSATIONS;
        return;
    }

    // for safety
    stopMessagePolling();

    // initial synchronous fetch
    Logger::log("Fetching messages...");
    if (!client_->getMessages(activeChatUser_)) {
        Logger::log("Failed to load messages");
        currentPage_ = Page::CONVERSATIONS;
        return;
    }

    renderMessagesOnce();

    // start background polling
    startMessagePolling();

    Logger::log("\n1. Send message");
    Logger::log("2. Back");

    int choice = getUserChoice(1, 2);
    handleMessagesInput(choice);
}

void CLI::renderMessagesOnce() {
    auto messages = client_->getDecryptedMessages();

    Logger::log("-------------------------");

    if (messages.empty()) {
        Logger::log("No messages");
    } else {
        for (const auto& text : messages) {
            Logger::log(text);
        }
    }

    Logger::log("-------------------------");
}

void CLI::startMessagePolling() {
    if (pollingMessages_) return;

    pollingMessages_ = true;

    messagePoller_ = std::thread([this]() {
        while (pollingMessages_) {
            if (!client_->getMessages(activeChatUser_)) {
                Logger::log("Message poll failed");
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(fetchIntervalMs_));
        }
    });
}

void CLI::stopMessagePolling() {
    pollingMessages_ = false;

    if (messagePoller_.joinable()) {
        messagePoller_.join();
    }
}

void CLI::handleMessagesInput(int choice) {
    switch (choice) {
        case 1: {
            // pause polling to avoid racing get_messages against background thread
            stopMessagePolling();

            std::string text;
            Logger::log("Message: ");
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, text);

            if (!client_->sendMessage(activeChatUser_, text)) {
                Logger::log("Failed to send message");
            }

            // synchronous fetch to include users sent message
            if (!client_->getMessages(activeChatUser_)) {
                Logger::log("Failed to refresh messages after send");
            }

            renderMessagesOnce();

            // resume polling
            startMessagePolling();

            break;
        }

        case 2:
            stopMessagePolling();
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