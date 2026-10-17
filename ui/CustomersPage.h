#pragma once
#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
class LogisticsSystem;
class CustomersPage : public QWidget
{
    Q_OBJECT
public:
    explicit CustomersPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
private slots:
    void add();
    void removeSelected();
private:
    LogisticsSystem *system;
    QTableWidget *table;
    QLineEdit *searchEdit;
};
