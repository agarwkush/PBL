#pragma once

#include <string>

/**
 * Customer class representing a customer in the logistics system
 * Uses only standard library types (no Qt)
 */
class Customer {
private:
    int id;
    std::string name;
    std::string phone;
    std::string address;

public:
    Customer();
    Customer(int id, std::string name, std::string phone, std::string address);

    // Getters
    int getId() const;
    std::string getName() const;
    std::string getPhone() const;
    std::string getAddress() const;

    // Setters
    void setId(int id);
    void setName(std::string name);
    void setPhone(std::string phone);
    void setAddress(std::string address);
};
