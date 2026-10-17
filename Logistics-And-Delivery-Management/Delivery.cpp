#include "Delivery.h"
#include <ctime>
#include <sstream>
#include <iomanip>
#include <cmath>

StatusHistoryNode::StatusHistoryNode(Status status, std::string timestamp)
    : status(status), timestamp(timestamp), next(nullptr) {}

Delivery::Delivery()
    : id(0), customerId(0), pickupLocation(""), destination(""),
      priority(Priority::Normal), status(Status::Pending), driverId(0),
      cost(0.0), createdAt(""), statusHistoryHead(nullptr) {}

Delivery::Delivery(int id, int customerId, std::string pickup,
                   std::string destination, Priority priority, double cost)
    : id(id), customerId(customerId), pickupLocation(pickup),
      destination(destination), priority(priority), status(Status::Pending),
      driverId(0), cost(cost), createdAt(currentTimestamp()),
      statusHistoryHead(nullptr) {
    addStatusHistory(Status::Pending, createdAt);
}

Delivery::~Delivery() {
    clearStatusHistory();
}

int Delivery::getId() const { return id; }
int Delivery::getCustomerId() const { return customerId; }
std::string Delivery::getPickupLocation() const { return pickupLocation; }
std::string Delivery::getDestination() const { return destination; }
Priority Delivery::getPriority() const { return priority; }
Status Delivery::getStatus() const { return status; }
int Delivery::getDriverId() const { return driverId; }
double Delivery::getCost() const { return cost; }
std::string Delivery::getCreatedAt() const { return createdAt; }
StatusHistoryNode* Delivery::getStatusHistoryHead() const { return statusHistoryHead; }

void Delivery::setId(int id) { this->id = id; }
void Delivery::setCustomerId(int customerId) { this->customerId = customerId; }
void Delivery::setPickupLocation(std::string pickup) { this->pickupLocation = pickup; }
void Delivery::setDestination(std::string destination) { this->destination = destination; }
void Delivery::setPriority(Priority priority) { this->priority = priority; }

void Delivery::setStatus(Status status) {
    this->status = status;
    addStatusHistory(status, currentTimestamp());
}

void Delivery::setDriverId(int driverId) { this->driverId = driverId; }
void Delivery::setCost(double cost) { this->cost = cost; }
void Delivery::setCreatedAt(std::string timestamp) { createdAt = timestamp; }

void Delivery::addStatusHistory(Status status, std::string timestamp) {
    StatusHistoryNode* newNode = new StatusHistoryNode(status, timestamp);
    newNode->next = statusHistoryHead;
    statusHistoryHead = newNode;
}

void Delivery::clearStatusHistory() {
    StatusHistoryNode* current = statusHistoryHead;
    while (current != nullptr) {
        StatusHistoryNode* next = current->next;
        delete current;
        current = next;
    }
    statusHistoryHead = nullptr;
}

int Delivery::getPriorityValue() const {
    switch (priority) {
        case Priority::Urgent: return 3;
        case Priority::High:   return 2;
        case Priority::Normal: return 1;
    }
    return 1;
}

std::string Delivery::statusToString(Status status) {
    switch (status) {
        case Status::Pending:   return "Pending";
        case Status::Assigned:  return "Assigned";
        case Status::PickedUp:  return "Picked Up";
        case Status::InTransit: return "In Transit";
        case Status::Delivered: return "Delivered";
        case Status::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

std::string Delivery::priorityToString(Priority priority) {
    switch (priority) {
        case Priority::Normal: return "Normal";
        case Priority::High:   return "High";
        case Priority::Urgent: return "Urgent";
    }
    return "Normal";
}

std::string Delivery::getStatusString() const { return statusToString(status); }
std::string Delivery::getPriorityString() const { return priorityToString(priority); }

double Delivery::calculateCost(double distanceKm, Priority priority) {
    const double baseFare = 50.0;
    const double perKm = 12.0;
    double surcharge = 0.0;
    if (priority == Priority::High) surcharge = 50.0;
    if (priority == Priority::Urgent) surcharge = 100.0;
    return std::round((baseFare + perKm * distanceKm + surcharge) * 100.0) / 100.0;
}

std::string Delivery::currentTimestamp() {
    time_t now = time(nullptr);
    tm* localTime = localtime(&now);

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (localTime->tm_year + 1900) << "-"
        << std::setw(2) << (localTime->tm_mon + 1) << "-"
        << std::setw(2) << localTime->tm_mday << " "
        << std::setw(2) << localTime->tm_hour << ":"
        << std::setw(2) << localTime->tm_min << ":"
        << std::setw(2) << localTime->tm_sec;
    return oss.str();
}
