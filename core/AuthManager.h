#pragma once

#include "User.h"
#include "../logic/LinkedList.h"

/**
 * AuthManager handles user authentication and management
 */
class AuthManager {
private:
    LinkedList<User*> users;
    User* currentUser;

public:
    AuthManager();
    ~AuthManager();

    // User management
    bool addUser(std::string username, std::string password, Role role, int linkedId);
    User* findUserByUsername(std::string username);
    bool removeUser(int id);
    LinkedList<User*>& getAllUsers();

    // Authentication
    User* authenticate(std::string username, std::string password);
    User* getCurrentUser() const;
    void logout();

    // Validation helpers
    bool usernameExists(std::string username);
};
