#pragma once
#include <QWidget>

class QDoubleSpinBox;
class QLabel;
class LogisticsSystem;

class SettingsPage : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
private slots:
    void applyRadius();
private:
    LogisticsSystem *system;
    QDoubleSpinBox *radiusSpin = nullptr;
    QLabel *info = nullptr;
};
