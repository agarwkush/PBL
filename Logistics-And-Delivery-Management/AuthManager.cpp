#include "AuthManager.h"
#include "Validator.h"

AuthManager::AuthManager() : currentUser(nullptr) {}

AuthManager::~AuthManager() {
    // Clean up all users
    for (auto it = users.begin(); it != users.end(); ++it) {
        delete *it;
    }
    users.clear();
}

bool AuthManager::addUser(std::string username, std::string password, Role role, int linkedId) {
    if (!Validator::isValidUsername(username)) {
        return false;
    }
    if (!Validator::isValidPassword(password)) {
        return false;
    }
    if (usernameExists(username)) {
        return false;  // Username already exists
    }

    // Generate next ID (simple increment)
    int nextId = users.size() + 1;
    std::string passwordHash = User::hashPassword(password);

    User* newUser = new User(nextId, username, passwordHash, role, linkedId);
    users.insertBack(newUser);
    return true;
}

User* AuthManager::findUserByUsername(std::string username) {
    for (auto it = users.begin(); it != users.end(); ++it) {
        if ((*it)->getUsername() == username) {
            return *it;
        }
    }
    return nullptr;
}

bool AuthManager::removeUser(int id) {
    User* user = users.findPtr(id);
    if (user == nullptr) {
        return false;
    }
    users.remove(id);
    delete user;
    return true;
}

LinkedList<User*>& AuthManager::getAllUsers() {
    return users;
}

User* AuthManager::authenticate(std::string username, std::string password) {
    User* user = findUserByUsername(username);
    if (user == nullptr) {
        return nullptr;
    }

    if (User::verifyPassword(password, user->getPasswordHash())) {
        currentUser = user;
        return user;
    }

    return nullptr;
}

User* AuthManager::getCurrentUser() const {
    return currentUser;
}

void AuthManager::logout() {
    currentUser = nullptr;
}

bool AuthManager::usernameExists(std::string username) {
    return findUserByUsername(username) != nullptr;
}
