#pragma once
#include "Customer.h"
#include "Delivery.h"
#include "Driver.h"
#include "User.h"
#include "AuthManager.h"
#include "FileManager.h"
#include "Validator.h"
#include "RouteService.h"
#include "../logic/LinkedList.h"
#include "../logic/Queue.h"
#include "../logic/PriorityQueue.h"
#include "../logic/Sorting.h"
#include <string>
#include <vector>

class LogisticsSystem {
public:
    struct BatchGroup {
        int anchorId = 0;
        std::vector<int> memberIds;
    };

    struct ActivityEvent {
        std::string time;
        std::string text;
    };

    struct LoginOutcome {
        bool ok = false;
        User* user = nullptr;
        std::string error;
    };

    explicit LogisticsSystem(std::string dataPath = "data/");
    ~LogisticsSystem();

    AuthManager* getAuthManager();
    User* login(std::string username, std::string password);
    LoginOutcome loginAs(const std::string& username, const std::string& password, Role expected);
    void logout();
    static std::string roleLabel(Role role);

    bool addCustomer(std::string name, std::string phone, std::string address);
    Customer* getCustomer(int id);
    LinkedList<Customer*>& getAllCustomers();
    bool removeCustomer(int id);
    bool registerCustomerUser(const std::string& name, const std::string& phone,
                              const std::string& address, const std::string& username,
                              const std::string& password);

    bool addDelivery(int customerId, std::string pickup, std::string destination, Priority priority);
    bool updateDelivery(int deliveryId, std::string pickup, std::string destination, Priority priority);
    Delivery* getDelivery(int id);
    LinkedList<Delivery*>& getAllDeliveries();
    bool updateDeliveryStatus(int id, Status status);
    bool removeDelivery(int id);
    Delivery* searchDelivery(int id);
    LinkedList<Delivery*> getDeliveriesByCustomer(int customerId);
    LinkedList<Delivery*> getDeliveriesByDriver(int driverId);

    bool addDriver(std::string name, std::string vehicle, int userId);
    bool updateDriver(int driverId, std::string name, std::string vehicle);
    Driver* getDriver(int id);
    Driver* getDriverByUserId(int userId);
    LinkedList<Driver*>& getAllDrivers();
    bool removeDriver(int id);
    bool registerDriverUser(const std::string& name, const std::string& vehicle,
                            const std::string& username, const std::string& password);
    bool setDriverAvailable(int driverId, bool available);
    bool hasActiveDelivery(int driverId) const;

    bool assignNextDelivery();
    bool assignManually(int deliveryId, int driverId);
    bool assignBatch(const std::vector<int>& deliveryIds, int driverId);
    bool assignNextBatch(double radiusKm = -1);
    Delivery* getNextPendingDelivery();
    Driver* getAvailableDriver();

    LocationResult resolveLocation(const std::string& query) const;
    RouteResult routeBetween(const std::string& origin, const std::string& destination) const;
    std::vector<std::string> serviceAreas() const;
    double quoteCost(const std::string& pickup, const std::string& destination, Priority priority) const;
    double routeDistanceKm(const Delivery* delivery) const;

    double getBatchRadiusKm() const;
    void setBatchRadiusKm(double r);
    double deliveryDistanceKm(int id1, int id2);
    LinkedList<Delivery*> findNearbyDeliveries(int deliveryId, double radiusKm = -1);
    std::vector<BatchGroup> getBatchGroups(double radiusKm = -1);
    int getBatchCount(double radiusKm = -1);

    void sortDeliveriesByID();
    void sortDeliveriesByPriority();

    std::vector<ActivityEvent> getRecentEvents(int limit) const;
    std::string lastError() const;

    bool saveData();
    bool loadData();

    int getPendingCount() const;
    int getAssignedCount() const;
    int getPickedUpCount() const;
    int getInTransitCount() const;
    int getDeliveredCount() const;
    int getCancelledCount() const;
    int getUrgentCount() const;
    int getTotalDeliveryCount() const;
    int getTotalDriverCount() const;
    int getTotalCustomerCount() const;
    int getAvailableDriverCount() const;
    double getTotalRevenue() const;

private:
    LinkedList<Delivery*> allDeliveries;
    LinkedList<Driver*> allDrivers;
    LinkedList<Customer*> allCustomers;
    Queue<Delivery*> normalQueue;
    PriorityQueue<Delivery*> urgentQueue;
    AuthManager authManager;
    FileManager fileManager;
    RouteService routes;

    int nextDeliveryId;
    int nextDriverId;
    int nextCustomerId;
    int nextUserId;
    double batchRadiusKm;
    std::string lastErrorText;
    std::vector<ActivityEvent> events;

    void rebuildQueues();
    void ensureAdminAccount();
    void syncDriverAvailability();
    void assignDeliveryTo(Delivery* delivery, int driverId);
    void logEvent(const std::string& text);
    void setError(const std::string& text);
};
