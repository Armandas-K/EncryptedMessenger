#include "client/CLI.h"

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
    std::cout << "\n=== Encrypted Messenger ===\n";
    std::cout << "1. Log in\n";
    std::cout << "2. Create Account\n";
    std::cout << "3. Exit\n";
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
    std::cout << "\n=== Login ===\n";
    std::cout << "1. Enter credentials\n";
    std::cout << "2. Back\n";
    int choice = getUserChoice(1, 2);
    handleLoginInput(choice);
}

void CLI::handleLoginInput(int choice) {
    switch (choice) {
        case 1: {
            std::string username, password;
            std::cout << "Username: ";
            std::cin >> username;
            std::cout << "Password: ";
            std::cin >> password;
            // Call your client’s login function
            if (client->login(username, password)) {
                std::cout << "Login successful!\n";
                currentPage = Page::SEND_MESSAGE;
            } else {
                std::cout << "Login failed.\n";
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
    std::cout << "\n=== Create Account ===\n";
    std::cout << "1. Enter details\n";
    std::cout << "2. Back\n";
    int choice = getUserChoice(1, 2);
    handleCreateAccountInput(choice);
}

void CLI::handleCreateAccountInput(int choice) {
    switch (choice) {
        case 1: {
            std::string username, password;
            std::cout << "New username: ";
            std::cin >> username;
            std::cout << "New password: ";
            std::cin >> password;
            if (client->createAccount(username, password)) {
                std::cout << "Account created!\n";
            } else {
                std::cout << "Account creation failed.\n";
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
    std::cout << "\n=== Send Message ===\n";
    std::cout << "1. Send a message\n";
    std::cout << "2. Log out\n";
    int choice = getUserChoice(1, 2);
    handleSendMessageInput(choice);
}

void CLI::handleSendMessageInput(int choice) {
    switch (choice) {
        case 1: {
            std::string recipient, message;
            std::cout << "Recipient: ";
            std::cin >> recipient;
            std::cin.ignore();
            std::cout << "Message: ";
            std::getline(std::cin, message);
            client->sendMessage(recipient, message);
            std::cout << "Message sent!\n";
            break;
        }
        case 2:
            currentPage = Page::MAIN_MENU;
            break;
    }
}

// ----------- Input Handling Helper -------------
int CLI::getUserChoice(int min, int max) {
    int choice;
    while (true) {
        std::cout << "> ";
        std::cin >> choice;
        if (std::cin.fail() || choice < min || choice > max) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            std::cout << "Invalid option. Try again.\n";
        } else {
            return choice;
        }
    }
}