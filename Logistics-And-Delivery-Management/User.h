#pragma once

#include <string>

/**
 * User Role Enum
 */
enum class Role {
    LogisticsTeam,
    Customer,
    DeliveryMan
};

/**
 * User class representing a system user with authentication
 */
class User {
private:
    int id;
    std::string username;
    std::string passwordHash;  // Simple hash (not secure, for demo only)
    Role role;
    int linkedId;  // customerId for Customer role, driverId for DeliveryMan

public:
    User();
    User(int id, std::string username, std::string passwordHash, Role role, int linkedId);

    // Getters
    int getId() const;
    std::string getUsername() const;
    std::string getPasswordHash() const;
    Role getRole() const;
    int getLinkedId() const;

    // Setters
    void setId(int id);
    void setUsername(std::string username);
    void setPasswordHash(std::string passwordHash);
    void setRole(Role role);
    void setLinkedId(int linkedId);

    // Utility methods
    std::string getRoleString() const;

    // Simple hash function (for demo purposes only - not secure)
    static std::string hashPassword(std::string password);
    static bool verifyPassword(std::string password, std::string hash);
};
