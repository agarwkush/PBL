#pragma once
#include <string>

enum class Status {
    Pending,
    Assigned,
    PickedUp,
    InTransit,
    Delivered,
    Cancelled
};

enum class Priority {
    Normal,
    High,
    Urgent
};

class StatusHistoryNode {
public:
    Status status;
    std::string timestamp;
    StatusHistoryNode* next;

    StatusHistoryNode(Status status, std::string timestamp);
};

class Delivery {
private:
    int id;
    int customerId;
    std::string pickupLocation;
    std::string destination;
    Priority priority;
    Status status;
    int driverId;
    double cost;
    std::string createdAt;
    StatusHistoryNode* statusHistoryHead;

public:
    Delivery();
    Delivery(int id, int customerId, std::string pickup,
             std::string destination, Priority priority, double cost);
    ~Delivery();

    Delivery(const Delivery&) = delete;
    Delivery& operator=(const Delivery&) = delete;

    int getId() const;
    int getCustomerId() const;
    std::string getPickupLocation() const;
    std::string getDestination() const;
    Priority getPriority() const;
    Status getStatus() const;
    int getDriverId() const;
    double getCost() const;
    std::string getCreatedAt() const;
    StatusHistoryNode* getStatusHistoryHead() const;

    void setId(int id);
    void setCustomerId(int customerId);
    void setPickupLocation(std::string pickup);
    void setDestination(std::string destination);
    void setPriority(Priority priority);
    void setStatus(Status status);
    void setDriverId(int driverId);
    void setCost(double cost);
    void setCreatedAt(std::string timestamp);

    void addStatusHistory(Status status, std::string timestamp);
    void clearStatusHistory();

    int getPriorityValue() const;
    std::string getStatusString() const;
    std::string getPriorityString() const;

    static double calculateCost(double distanceKm, Priority priority);
    static std::string currentTimestamp();
    static std::string statusToString(Status status);
    static std::string priorityToString(Priority priority);
};
