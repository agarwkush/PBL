#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include <QList>
#include <QStringList>

class LogisticsSystem;
class DashboardPage;
class DeliveriesPage;
class DriversPage;
class CustomersPage;
class AssignmentsPage;
class ReportsPage;
class SearchPage;
class DsaViewerPage;
class SettingsPage;
class LocationsPage;
class TrackingPage;
class QLabel;
class QPushButton;
class QLineEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(LogisticsSystem *sys, QWidget *parent = nullptr);
    void switchToPage(int idx);
private slots:
    void openDashboard();
    void openDeliveries();
    void openTracking();
    void openDrivers();
    void openCustomers();
    void openAssign();
    void openReports();
    void openSearch();
    void openDsa();
    void openLocations();
    void openSettings();
    void doLogout();
    void refreshAll();
private:
    LogisticsSystem *system;
    QStackedWidget *stack;
    DashboardPage *dashboard = nullptr;
    DeliveriesPage *deliveries = nullptr;
    TrackingPage *tracking = nullptr;
    DriversPage *drivers = nullptr;
    CustomersPage *customers = nullptr;
    AssignmentsPage *assign = nullptr;
    ReportsPage *reports = nullptr;
    SearchPage *search = nullptr;
    DsaViewerPage *dsa = nullptr;
    SettingsPage *settings = nullptr;
    LocationsPage *locations = nullptr;
    QLabel* statusInfo = nullptr;
    QLabel* crumb = nullptr;
    QLineEdit* globalSearch = nullptr;
    QList<QPushButton*> navBtns;
    QStringList navTitles;
    void selectNav(int i);
};
