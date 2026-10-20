#include "LogisticsSystem.h"
#include "GeoUtils.h"
#include "Logger.h"
#include <fstream>
#include <string>
#include <vector>

LogisticsSystem::LogisticsSystem(std::string dataPath)
    : fileManager(dataPath),
      nextDeliveryId(1),
      nextDriverId(1),
      nextCustomerId(1),
      nextUserId(1),
      batchRadiusKm(GeoUtils::DEFAULT_BATCH_RADIUS_KM) {
    loadData();
    ensureAdminAccount();
}

LogisticsSystem::~LogisticsSystem() {
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) delete *it;
    allDeliveries.clear();
    for (auto it = allDrivers.begin(); it != allDrivers.end(); ++it) delete *it;
    allDrivers.clear();
    for (auto it = allCustomers.begin(); it != allCustomers.end(); ++it) delete *it;
    allCustomers.clear();
}

void LogisticsSystem::rebuildQueues() {
    normalQueue.clear();
    urgentQueue.clear();
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) {
        Delivery* d = *it;
        if (d->getStatus() != Status::Pending) continue;
        if (d->getPriority() == Priority::Normal) normalQueue.enqueue(d);
        else urgentQueue.insert(d);
    }
}

void LogisticsSystem::ensureAdminAccount() {
    bool hasAdmin = false;
    for (auto it = authManager.getAllUsers().begin(); it != authManager.getAllUsers().end(); ++it)
        if ((*it)->getRole() == Role::LogisticsTeam) hasAdmin = true;
    if (!hasAdmin) {
        if (authManager.usernameExists("admin")) {
            Logger::error("No admin account exists and the username admin is taken; admin sign-in is unavailable.");
        } else if (authManager.addUser("admin", "admin123", Role::LogisticsTeam, 0)) {
            Logger::info("Created the default admin account.");
            int mx = 0;
            for (auto it = authManager.getAllUsers().begin(); it != authManager.getAllUsers().end(); ++it)
                if ((*it)->getId() > mx) mx = (*it)->getId();
            nextUserId = mx + 1;
        }
    }
    rebuildQueues();
    saveData();
}

void LogisticsSystem::syncDriverAvailability() {
    for (auto it = allDrivers.begin(); it != allDrivers.end(); ++it) {
        Driver* d = *it;
        if (hasActiveDelivery(d->getId())) {
            d->setAvailability(DriverAvailability::Assigned);
        } else if (d->getAvailability() == DriverAvailability::Assigned) {
            d->setAvailability(DriverAvailability::Available);
        }
    }
}

void LogisticsSystem::assignDeliveryTo(Delivery* delivery, int driverId) {
    delivery->setStatus(Status::Assigned);
    delivery->setDriverId(driverId);
    syncDriverAvailability();
    rebuildQueues();
    Driver* driver = getDriver(driverId);
    if (driver) {
        logEvent("Order #" + std::to_string(delivery->getId()) + " assigned to " + driver->getName());
    } else {
        logEvent("Order #" + std::to_string(delivery->getId()) + " assigned to driver #" + std::to_string(driverId));
    }
}

void LogisticsSystem::logEvent(const std::string& text) {
    events.push_back(ActivityEvent{Delivery::currentTimestamp(), text});
    if (events.size() > 300) events.erase(events.begin());
}

void LogisticsSystem::setError(const std::string& text) {
    lastErrorText = text;
}

std::string LogisticsSystem::lastError() const {
    return lastErrorText;
}

std::vector<LogisticsSystem::ActivityEvent> LogisticsSystem::getRecentEvents(int limit) const {
    std::vector<ActivityEvent> out;
    for (int i = (int)events.size() - 1; i >= 0 && (int)out.size() < limit; i--)
        out.push_back(events[i]);
    return out;
}

AuthManager* LogisticsSystem::getAuthManager() {
    return &authManager;
}

User* LogisticsSystem::login(std::string username, std::string password) {
    return authManager.authenticate(username, password);
}

LogisticsSystem::LoginOutcome LogisticsSystem::loginAs(const std::string& username,
                                                       const std::string& password, Role expected) {
    LoginOutcome outcome;
    User* user = authManager.authenticate(username, password);
    if (!user) {
        outcome.error = "Incorrect username or password.";
        return outcome;
    }
    if (user->getRole() != expected) {
        std::string actual = roleLabel(user->getRole());
        authManager.logout();
        outcome.error = "This account is registered as " + actual + ". Select " + actual + " to sign in.";
        return outcome;
    }
    if (expected == Role::DeliveryMan && !getDriverByUserId(user->getId())) {
        authManager.logout();
        outcome.error = "This driver login is not linked to a driver profile.";
        return outcome;
    }
    if (expected == Role::Customer && !getCustomer(user->getLinkedId())) {
        authManager.logout();
        outcome.error = "This customer login is not linked to a customer profile.";
        return outcome;
    }
    outcome.ok = true;
    outcome.user = user;
    return outcome;
}

void LogisticsSystem::logout() {
    authManager.logout();
}

std::string LogisticsSystem::roleLabel(Role role) {
    if (role == Role::LogisticsTeam) return "Admin";
    if (role == Role::DeliveryMan) return "Driver";
    return "Customer";
}

bool LogisticsSystem::addCustomer(std::string name, std::string phone, std::string address) {
    if (!Validator::isValidName(name)) {
        setError("Enter a name with at least 2 letters.");
        return false;
    }
    if (!Validator::isValidPhone(phone)) {
        setError("Enter a 10-digit phone number.");
        return false;
    }
    if (!Validator::isValidLocation(address)) {
        setError("Enter an address with at least 3 characters.");
        return false;
    }

    Customer* customer = new Customer(nextCustomerId, name, phone, address);
    allCustomers.insertBack(customer);
    nextCustomerId++;
    logEvent("Customer added: " + name);
    saveData();
    return true;
}

Customer* LogisticsSystem::getCustomer(int id) {
    return allCustomers.find(id);
}

LinkedList<Customer*>& LogisticsSystem::getAllCustomers() {
    return allCustomers;
}

bool LogisticsSystem::removeCustomer(int id) {
    Customer* customer = allCustomers.find(id);
    if (customer == nullptr) {
        setError("Customer not found.");
        return false;
    }
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) {
        if ((*it)->getCustomerId() == id) {
            setError("This customer has orders and cannot be removed.");
            return false;
        }
    }
    {
        auto& users = authManager.getAllUsers();
        std::vector<int> stale;
        for (auto it = users.begin(); it != users.end(); ++it) {
            User* usr = *it;
            if (usr->getRole() == Role::Customer && usr->getLinkedId() == id)
                stale.push_back(usr->getId());
        }
        for (int x : stale) authManager.removeUser(x);
    }
    allCustomers.remove(id);
    delete customer;
    logEvent("Customer #" + std::to_string(id) + " removed");
    saveData();
    return true;
}

bool LogisticsSystem::registerCustomerUser(const std::string& name, const std::string& phone,
                                           const std::string& address, const std::string& username,
                                           const std::string& password) {
    if (authManager.usernameExists(username)) {
        setError("That username is already taken.");
        return false;
    }
    if (!addCustomer(name, phone, address)) return false;
    int cid = nextCustomerId - 1;
    if (!authManager.addUser(username, password, Role::Customer, cid)) {
        removeCustomer(cid);
        nextCustomerId--;
        setError("Enter a username of 3 to 20 letters, digits or _, and a password of at least 4 characters.");
        return false;
    }
    saveData();
    return true;
}

LocationResult LogisticsSystem::resolveLocation(const std::string& query) const {
    return routes.resolve(query);
}

RouteResult LogisticsSystem::routeBetween(const std::string& origin, const std::string& destination) const {
    return routes.calculateRoute(origin, destination);
}

std::vector<std::string> LogisticsSystem::serviceAreas() const {
    return routes.serviceAreas();
}

double LogisticsSystem::quoteCost(const std::string& pickup, const std::string& destination, Priority priority) const {
    RouteResult route = routes.calculateRoute(pickup, destination);
    if (!route.ok) return -1.0;
    return Delivery::calculateCost(route.distanceKm, priority);
}

double LogisticsSystem::routeDistanceKm(const Delivery* delivery) const {
    RouteResult route = routes.calculateRoute(delivery->getPickupLocation(), delivery->getDestination());
    return route.ok ? route.distanceKm : -1.0;
}

bool LogisticsSystem::addDelivery(int customerId, std::string pickup, std::string destination, Priority priority) {
    if (!getCustomer(customerId)) {
        setError("Select a valid customer.");
        return false;
    }
    LocationResult from = resolveLocation(pickup);
    if (!from.ok) {
        setError("Pickup: " + from.error);
        return false;
    }
    LocationResult to = resolveLocation(destination);
    if (!to.ok) {
        setError("Destination: " + to.error);
        return false;
    }
    RouteResult route = routeBetween(pickup, destination);
    if (!route.ok) {
        setError(route.error);
        return false;
    }

    double cost = Delivery::calculateCost(route.distanceKm, priority);
    Delivery* delivery = new Delivery(nextDeliveryId, customerId, pickup, destination, priority, cost);
    allDeliveries.insertBack(delivery);
    logEvent("Order #" + std::to_string(nextDeliveryId) + " created: " + from.area + " to " + to.area);
    nextDeliveryId++;
    rebuildQueues();
    saveData();
    return true;
}

bool LogisticsSystem::updateDelivery(int deliveryId, std::string pickup, std::string destination, Priority priority) {
    Delivery* delivery = getDelivery(deliveryId);
    if (!delivery) {
        setError("Order not found.");
        return false;
    }
    if (delivery->getStatus() != Status::Pending) {
        setError("Only pending orders can be edited.");
        return false;
    }
    LocationResult from = resolveLocation(pickup);
    if (!from.ok) {
        setError("Pickup: " + from.error);
        return false;
    }
    LocationResult to = resolveLocation(destination);
    if (!to.ok) {
        setError("Destination: " + to.error);
        return false;
    }
    RouteResult route = routeBetween(pickup, destination);
    if (!route.ok) {
        setError(route.error);
        return false;
    }
    delivery->setPickupLocation(pickup);
    delivery->setDestination(destination);
    delivery->setPriority(priority);
    delivery->setCost(Delivery::calculateCost(route.distanceKm, priority));
    rebuildQueues();
    logEvent("Order #" + std::to_string(deliveryId) + " updated");
    saveData();
    return true;
}

Delivery* LogisticsSystem::getDelivery(int id) {
    return allDeliveries.find(id);
}

LinkedList<Delivery*>& LogisticsSystem::getAllDeliveries() {
    return allDeliveries;
}

bool LogisticsSystem::updateDeliveryStatus(int id, Status status) {
    Delivery* delivery = getDelivery(id);
    if (delivery == nullptr) {
        setError("Order not found.");
        return false;
    }
    if (!Validator::isValidStatusTransition(delivery->getStatus(), status)) {
        setError("A " + delivery->getStatusString() + " order cannot become " +
                 Delivery::statusToString(status) + ".");
        return false;
    }
    if (status == Status::Assigned && delivery->getDriverId() == 0) {
        setError("Assign a driver before marking the order as Assigned.");
        return false;
    }

    delivery->setStatus(status);
    if (status == Status::Cancelled) delivery->setDriverId(0);
    syncDriverAvailability();
    rebuildQueues();
    logEvent("Order #" + std::to_string(id) + " is now " + Delivery::statusToString(status));
    saveData();
    return true;
}

bool LogisticsSystem::removeDelivery(int id) {
    Delivery* delivery = getDelivery(id);
    if (delivery == nullptr) {
        setError("Order not found.");
        return false;
    }
    Status s = delivery->getStatus();
    if (s == Status::Assigned || s == Status::PickedUp || s == Status::InTransit) {
        setError("Active orders cannot be deleted. Cancel the order first.");
        return false;
    }
    allDeliveries.remove(id);
    delete delivery;
    rebuildQueues();
    logEvent("Order #" + std::to_string(id) + " deleted");
    saveData();
    return true;
}

Delivery* LogisticsSystem::searchDelivery(int id) {
    return allDeliveries.find(id);
}

bool LogisticsSystem::addDriver(std::string name, std::string vehicle, int userId) {
    if (!Validator::isValidName(name)) {
        setError("Enter a driver name with at least 2 letters.");
        return false;
    }
    if (!Validator::isValidVehicle(vehicle)) {
        setError("Enter a vehicle number.");
        return false;
    }
    Driver* driver = new Driver(nextDriverId, userId, name, vehicle, DriverAvailability::Available);
    allDrivers.insertBack(driver);
    logEvent("Driver added: " + name);
    nextDriverId++;
    saveData();
    return true;
}

bool LogisticsSystem::updateDriver(int driverId, std::string name, std::string vehicle) {
    Driver* driver = getDriver(driverId);
    if (!driver) {
        setError("Driver not found.");
        return false;
    }
    if (!Validator::isValidName(name)) {
        setError("Enter a driver name with at least 2 letters.");
        return false;
    }
    if (!Validator::isValidVehicle(vehicle)) {
        setError("Enter a vehicle number.");
        return false;
    }
    driver->setName(name);
    driver->setVehicle(vehicle);
    logEvent("Driver #" + std::to_string(driverId) + " details updated");
    saveData();
    return true;
}

Driver* LogisticsSystem::getDriver(int id) {
    return allDrivers.find(id);
}

Driver* LogisticsSystem::getDriverByUserId(int userId) {
    for (auto it = allDrivers.begin(); it != allDrivers.end(); ++it) {
        if ((*it)->getUserId() == userId) return *it;
    }
    return nullptr;
}

LinkedList<Driver*>& LogisticsSystem::getAllDrivers() {
    return allDrivers;
}

bool LogisticsSystem::hasActiveDelivery(int driverId) const {
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) {
        Delivery* d = *it;
        if (d->getDriverId() != driverId) continue;
        Status s = d->getStatus();
        if (s == Status::Assigned || s == Status::PickedUp || s == Status::InTransit) return true;
    }
    return false;
}

bool LogisticsSystem::setDriverAvailable(int driverId, bool available) {
    Driver* driver = getDriver(driverId);
    if (!driver) {
        setError("Driver not found.");
        return false;
    }
    if (hasActiveDelivery(driverId)) {
        setError("This driver has active orders and cannot change availability.");
        return false;
    }
    driver->setAvailability(available ? DriverAvailability::Available : DriverAvailability::Unavailable);
    logEvent("Driver #" + std::to_string(driverId) + (available ? " is available" : " is unavailable"));
    saveData();
    return true;
}

bool LogisticsSystem::removeDriver(int id) {
    Driver* driver = getDriver(id);
    if (driver == nullptr) {
        setError("Driver not found.");
        return false;
    }
    if (hasActiveDelivery(id)) {
        setError("This driver has active orders and cannot be removed.");
        return false;
    }
    {
        int uid = driver->getUserId();
        auto& users = authManager.getAllUsers();
        std::vector<int> stale;
        for (auto it = users.begin(); it != users.end(); ++it) {
            User* usr = *it;
            if ((uid != 0 && usr->getId() == uid) ||
                (usr->getRole() == Role::DeliveryMan && usr->getLinkedId() == id))
                stale.push_back(usr->getId());
        }
        for (int x : stale) authManager.removeUser(x);
    }
    allDrivers.remove(id);
    delete driver;
    logEvent("Driver #" + std::to_string(id) + " removed");
    saveData();
    return true;
}

bool LogisticsSystem::registerDriverUser(const std::string& name, const std::string& vehicle,
                                         const std::string& username, const std::string& password) {
    if (authManager.usernameExists(username)) {
        setError("That username is already taken.");
        return false;
    }
    if (!authManager.addUser(username, password, Role::DeliveryMan, -1)) {
        setError("Enter a username of 3 to 20 letters, digits or _, and a password of at least 4 characters.");
        return false;
    }
    User* u = authManager.findUserByUsername(username);
    if (!u) return false;
    if (!addDriver(name, vehicle, u->getId())) {
        authManager.removeUser(u->getId());
        nextDriverId--;
        return false;
    }
    int did = nextDriverId - 1;
    u->setLinkedId(did);
    saveData();
    return true;
}

bool LogisticsSystem::assignNextDelivery() {
    Delivery* next = getNextPendingDelivery();
    if (!next) {
        setError("There are no pending orders.");
        return false;
    }
    Driver* driver = getAvailableDriver();
    if (!driver) {
        setError("No driver is available.");
        return false;
    }
    assignDeliveryTo(next, driver->getId());
    saveData();
    return true;
}

bool LogisticsSystem::assignManually(int deliveryId, int driverId) {
    Delivery* delivery = getDelivery(deliveryId);
    if (delivery == nullptr) {
        setError("Order not found.");
        return false;
    }
    if (delivery->getStatus() != Status::Pending) {
        setError("Only pending orders can be assigned.");
        return false;
    }
    Driver* driver = getDriver(driverId);
    if (driver == nullptr) {
        setError("Driver not found.");
        return false;
    }
    if (!driver->isAvailable()) {
        setError("This driver is not available.");
        return false;
    }
    assignDeliveryTo(delivery, driverId);
    saveData();
    return true;
}

bool LogisticsSystem::assignBatch(const std::vector<int>& deliveryIds, int driverId) {
    if (deliveryIds.empty()) {
        setError("The batch is empty.");
        return false;
    }
    Driver* driver = getDriver(driverId);
    if (!driver || !driver->isAvailable()) {
        setError("This driver is not available.");
        return false;
    }
    std::vector<Delivery*> deliveries;
    for (int id : deliveryIds) {
        Delivery* d = getDelivery(id);
        if (!d || d->getStatus() != Status::Pending) {
            setError("Every order in the batch must be pending.");
            return false;
        }
        deliveries.push_back(d);
    }
    for (Delivery* d : deliveries) {
        d->setStatus(Status::Assigned);
        d->setDriverId(driverId);
    }
    syncDriverAvailability();
    rebuildQueues();
    logEvent(std::string("Batch of ") + std::to_string(deliveryIds.size()) +
             " orders assigned to " + driver->getName());
    saveData();
    return true;
}

bool LogisticsSystem::assignNextBatch(double radiusKm) {
    if (radiusKm < 0) radiusKm = batchRadiusKm;
    Delivery* next = getNextPendingDelivery();
    if (!next) {
        setError("There are no pending orders.");
        return false;
    }
    Driver* driver = getAvailableDriver();
    if (!driver) {
        setError("No driver is available.");
        return false;
    }
    std::vector<int> ids;
    ids.push_back(next->getId());
    LinkedList<Delivery*> near = findNearbyDeliveries(next->getId(), radiusKm);
    for (auto it = near.begin(); it != near.end(); ++it) ids.push_back((*it)->getId());
    return assignBatch(ids, driver->getId());
}

Delivery* LogisticsSystem::getNextPendingDelivery() {
    if (!urgentQueue.isEmpty()) return urgentQueue.peek();
    if (!normalQueue.isEmpty()) return normalQueue.front();
    return nullptr;
}

Driver* LogisticsSystem::getAvailableDriver() {
    for (auto it = allDrivers.begin(); it != allDrivers.end(); ++it) {
        if ((*it)->isAvailable()) return *it;
    }
    return nullptr;
}

double LogisticsSystem::getBatchRadiusKm() const {
    return batchRadiusKm;
}

void LogisticsSystem::setBatchRadiusKm(double r) {
    if (r > 0 && r < 50) batchRadiusKm = r;
}

double LogisticsSystem::deliveryDistanceKm(int id1, int id2) {
    Delivery* a = getDelivery(id1);
    Delivery* b = getDelivery(id2);
    if (!a || !b) return -1;
    return GeoUtils::distanceKm(a->getPickupLocation(), b->getPickupLocation());
}

LinkedList<Delivery*> LogisticsSystem::findNearbyDeliveries(int deliveryId, double radiusKm) {
    LinkedList<Delivery*> out;
    if (radiusKm < 0) radiusKm = batchRadiusKm;
    Delivery* anchor = getDelivery(deliveryId);
    if (!anchor) return out;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) {
        Delivery* d = *it;
        if (d->getId() == deliveryId) continue;
        if (d->getStatus() != Status::Pending) continue;
        double dist = GeoUtils::distanceKm(anchor->getPickupLocation(), d->getPickupLocation());
        if (dist >= 0 && dist <= radiusKm) out.insertBack(d);
    }
    return out;
}

std::vector<LogisticsSystem::BatchGroup> LogisticsSystem::getBatchGroups(double radiusKm) {
    if (radiusKm < 0) radiusKm = batchRadiusKm;
    std::vector<BatchGroup> groups;
    std::vector<int> visited;
    auto isVisited = [&](int id) {
        for (int v : visited) if (v == id) return true;
        return false;
    };
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) {
        Delivery* a = *it;
        if (a->getStatus() != Status::Pending) continue;
        if (isVisited(a->getId())) continue;
        BatchGroup g;
        g.anchorId = a->getId();
        g.memberIds.push_back(a->getId());
        visited.push_back(a->getId());
        for (auto jt = allDeliveries.begin(); jt != allDeliveries.end(); ++jt) {
            Delivery* b = *jt;
            if (b->getId() == a->getId()) continue;
            if (b->getStatus() != Status::Pending) continue;
            if (isVisited(b->getId())) continue;
            double dist = GeoUtils::distanceKm(a->getPickupLocation(), b->getPickupLocation());
            if (dist >= 0 && dist <= radiusKm) {
                g.memberIds.push_back(b->getId());
                visited.push_back(b->getId());
            }
        }
        if (g.memberIds.size() > 1) groups.push_back(g);
    }
    return groups;
}

int LogisticsSystem::getBatchCount(double radiusKm) {
    return static_cast<int>(getBatchGroups(radiusKm).size());
}

void LogisticsSystem::sortDeliveriesByID() {
    int size = allDeliveries.size();
    if (size == 0) return;
    std::vector<Delivery*> arr;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) arr.push_back(*it);
    auto compareById = [](const Delivery* a, const Delivery* b) -> bool {
        return a->getId() < b->getId();
    };
    Sorting<Delivery*>::sort(arr.data(), size, compareById);
    allDeliveries.clear();
    for (Delivery* d : arr) allDeliveries.insertBack(d);
    rebuildQueues();
}

void LogisticsSystem::sortDeliveriesByPriority() {
    int size = allDeliveries.size();
    if (size == 0) return;
    std::vector<Delivery*> arr;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it) arr.push_back(*it);
    auto compareByPriority = [](const Delivery* a, const Delivery* b) -> bool {
        if (a->getPriorityValue() != b->getPriorityValue()) {
            return a->getPriorityValue() > b->getPriorityValue();
        }
        return a->getId() < b->getId();
    };
    Sorting<Delivery*>::sort(arr.data(), size, compareByPriority);
    allDeliveries.clear();
    for (Delivery* d : arr) allDeliveries.insertBack(d);
    rebuildQueues();
}

bool LogisticsSystem::saveData() {
    bool success = true;
    if (!fileManager.saveDeliveries(allDeliveries)) success = false;
    if (!fileManager.saveDrivers(allDrivers)) success = false;
    if (!fileManager.saveCustomers(allCustomers)) success = false;
    if (!fileManager.saveUsers(authManager.getAllUsers())) success = false;
    {
        std::ofstream cfg(fileManager.getDataPath() + "batch.cfg", std::ios::trunc);
        if (cfg.is_open()) cfg << batchRadiusKm << "\n";
    }
    fileManager.clearErrors();
    return success;
}

bool LogisticsSystem::loadData() {
    bool success = true;
    if (!fileManager.loadCustomers(allCustomers)) success = false;
    if (!fileManager.loadDrivers(allDrivers)) success = false;
    if (!fileManager.loadDeliveries(allDeliveries)) success = false;
    if (!fileManager.loadUsers(authManager.getAllUsers())) success = false;
    {
        std::ifstream cfg(fileManager.getDataPath() + "batch.cfg");
        double r = 0;
        if (cfg.is_open() && (cfg >> r) && r > 0 && r < 50) batchRadiusKm = r;
    }

    nextCustomerId = 1;
    for (auto it = allCustomers.begin(); it != allCustomers.end(); ++it)
        if ((*it)->getId() >= nextCustomerId) nextCustomerId = (*it)->getId() + 1;
    nextDriverId = 1;
    for (auto it = allDrivers.begin(); it != allDrivers.end(); ++it)
        if ((*it)->getId() >= nextDriverId) nextDriverId = (*it)->getId() + 1;
    nextDeliveryId = 1;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getId() >= nextDeliveryId) nextDeliveryId = (*it)->getId() + 1;
    nextUserId = 1;
    for (auto it = authManager.getAllUsers().begin(); it != authManager.getAllUsers().end(); ++it)
        if ((*it)->getId() >= nextUserId) nextUserId = (*it)->getId() + 1;

    syncDriverAvailability();
    rebuildQueues();
    fileManager.clearErrors();
    return success;
}

int LogisticsSystem::getPendingCount() const {
    int count = 0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getStatus() == Status::Pending) count++;
    return count;
}

int LogisticsSystem::getAssignedCount() const {
    int count = 0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getStatus() == Status::Assigned) count++;
    return count;
}

int LogisticsSystem::getPickedUpCount() const {
    int count = 0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getStatus() == Status::PickedUp) count++;
    return count;
}

int LogisticsSystem::getInTransitCount() const {
    int count = 0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getStatus() == Status::InTransit) count++;
    return count;
}

int LogisticsSystem::getDeliveredCount() const {
    int count = 0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getStatus() == Status::Delivered) count++;
    return count;
}

int LogisticsSystem::getTotalDeliveryCount() const {
    return allDeliveries.size();
}

int LogisticsSystem::getTotalDriverCount() const {
    return allDrivers.size();
}

int LogisticsSystem::getTotalCustomerCount() const {
    return allCustomers.size();
}

int LogisticsSystem::getAvailableDriverCount() const {
    int count = 0;
    for (auto it = allDrivers.begin(); it != allDrivers.end(); ++it)
        if ((*it)->isAvailable()) count++;
    return count;
}

int LogisticsSystem::getCancelledCount() const {
    int count = 0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getStatus() == Status::Cancelled) count++;
    return count;
}

int LogisticsSystem::getUrgentCount() const {
    int count = 0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getPriority() == Priority::Urgent && (*it)->getStatus() == Status::Pending) count++;
    return count;
}

double LogisticsSystem::getTotalRevenue() const {
    double total = 0.0;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        total += (*it)->getCost();
    return total;
}

LinkedList<Delivery*> LogisticsSystem::getDeliveriesByCustomer(int customerId) {
    LinkedList<Delivery*> result;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getCustomerId() == customerId) result.insertBack(*it);
    return result;
}

LinkedList<Delivery*> LogisticsSystem::getDeliveriesByDriver(int driverId) {
    LinkedList<Delivery*> result;
    for (auto it = allDeliveries.begin(); it != allDeliveries.end(); ++it)
        if ((*it)->getDriverId() == driverId) result.insertBack(*it);
    return result;
}
