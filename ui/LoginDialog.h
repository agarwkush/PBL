#pragma once
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QButtonGroup>
#include "../core/User.h"

class LogisticsSystem;

class LoginDialog : public QDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(LogisticsSystem *sys, QWidget *parent = nullptr);
private slots:
    void tryLogin();
    void registerCustomer();
    void roleChanged();
private:
    LogisticsSystem *system;
    QButtonGroup *roleGroup = nullptr;
    QPushButton *adminBtn = nullptr;
    QPushButton *driverBtn = nullptr;
    QPushButton *customerBtn = nullptr;
    QLabel *roleHint = nullptr;
    QLineEdit *userEdit = nullptr;
    QLineEdit *passEdit = nullptr;
    QLabel *msg = nullptr;
    Role selectedRole() const;
};
