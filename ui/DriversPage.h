#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>

class LogisticsSystem;
class BarChart;

class DriversPage : public QWidget
{
    Q_OBJECT
public:
    explicit DriversPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
private slots:
    void add();
    void editSelected();
    void toggleAvail();
    void removeSelected();
    void viewAssignment(int driverId);
private:
    LogisticsSystem *system;
    QTableWidget *table = nullptr;
    QLineEdit *searchEdit = nullptr;
    QComboBox *statusFilter = nullptr;
    QPushButton *toggleBtn = nullptr;
    QLabel *totVal = nullptr;
    QLabel *actVal = nullptr;
    QLabel *assignedVal = nullptr;
    QLabel *busyVal = nullptr;
    BarChart *perfChart = nullptr;
    int selectedDriverId() const;
};
