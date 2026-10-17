#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>

class LogisticsSystem;
class DonutChart;
class BarChart;
class HBarChart;

class ReportsPage : public QWidget
{
    Q_OBJECT
public:
    explicit ReportsPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
private:
    LogisticsSystem *system;
    QPushButton *refreshBtn = nullptr;
    QPushButton *expBtn = nullptr;
    DonutChart *donut = nullptr;
    BarChart *priorityChart = nullptr;
    HBarChart *topLocs = nullptr;
    QLabel *summary = nullptr;
};
