#include "User.h"
#include <functional>

User::User()
    : id(0), username(""), passwordHash(""), role(Role::LogisticsTeam), linkedId(0) {}

User::User(int id, std::string username, std::string passwordHash, Role role, int linkedId)
    : id(id), username(username), passwordHash(passwordHash), role(role), linkedId(linkedId) {}

int User::getId() const {
    return id;
}

std::string User::getUsername() const {
    return username;
}

std::string User::getPasswordHash() const {
    return passwordHash;
}

Role User::getRole() const {
    return role;
}

int User::getLinkedId() const {
    return linkedId;
}

void User::setId(int id) {
    this->id = id;
}

void User::setUsername(std::string username) {
    this->username = username;
}

void User::setPasswordHash(std::string passwordHash) {
    this->passwordHash = passwordHash;
}

void User::setRole(Role role) {
    this->role = role;
}

void User::setLinkedId(int linkedId) {
    this->linkedId = linkedId;
}

std::string User::getRoleString() const {
    switch (role) {
        case Role::LogisticsTeam: return "LogisticsTeam";
        case Role::Customer: return "Customer";
        case Role::DeliveryMan: return "DeliveryMan";
        default: return "Unknown";
    }
}

// Simple hash function using std::hash (for demo purposes only - not secure)
std::string User::hashPassword(std::string password) {
    std::hash<std::string> hasher;
    size_t hashValue = hasher(password);
    return std::to_string(hashValue);
}

// Verify password by comparing hashes
bool User::verifyPassword(std::string password, std::string hash) {
    return hashPassword(password) == hash;
}
