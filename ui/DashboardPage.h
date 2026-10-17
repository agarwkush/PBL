#pragma once
#include <QWidget>
#include <QLabel>
#include <QTableWidget>
#include <QGridLayout>
#include <QTimer>
#include <QListWidget>
#include <QFrame>
#include <QList>
#include "../core/User.h"

class LogisticsSystem;
class DonutChart;
class RouteMapWidget;

class DashboardPage : public QWidget
{
    Q_OBJECT
public:
    explicit DashboardPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
protected:
    void showEvent(QShowEvent* e) override;
    void hideEvent(QHideEvent* e) override;
private:
    struct KpiSlot {
        QLabel* val = nullptr;
        QLabel* cap = nullptr;
    };
    LogisticsSystem *system;
    QWidget *pageWidget = nullptr;
    QGridLayout *kpiGrid = nullptr;
    QList<KpiSlot> kpiSlots;
    int currentRoleInt = -1;
    DonutChart *donut = nullptr;
    RouteMapWidget *mapPreview = nullptr;
    QTableWidget *recent = nullptr;
    QTimer *liveTimer = nullptr;
    QListWidget *activityList = nullptr;
    QFrame *activityCard = nullptr;
    void clearLayout(QLayout* l);
    void buildKpis(Role role);
};
