#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLabel>
#include <QListWidget>
class LogisticsSystem;
class AssignmentsPage : public QWidget
{
    Q_OBJECT
public:
    explicit AssignmentsPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
private slots:
    void assign();
    void autoAssign();
    void assignBatch();
private:
    LogisticsSystem *system;
    QComboBox *deliveryBox = nullptr;
    QComboBox *driverBox = nullptr;
    QLabel *msg = nullptr;
    QListWidget *pendingList = nullptr;
    QListWidget *batchList = nullptr;
    QListWidget *driverList = nullptr;
};
