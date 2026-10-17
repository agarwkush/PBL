#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
class LogisticsSystem;
class DeliveriesPage : public QWidget
{
    Q_OBJECT
public:
    explicit DeliveriesPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
    void add();
private slots:
    void editSelected();
    void removeSelected();
    void advanceStatus();
    void cancelSelected();
    void assignSelected();
    void showNearby();
    void showDetails();
private:
    LogisticsSystem *system;
    QLabel *headTitle = nullptr;
    QLabel *headSub = nullptr;
    QLabel *countLabel = nullptr;
    QTableWidget *table = nullptr;
    QLineEdit *searchEdit = nullptr;
    QComboBox *statusFilter = nullptr;
    QComboBox *priorityFilter = nullptr;
    QComboBox *sortBox = nullptr;
};
