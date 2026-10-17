#pragma once
#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class LogisticsSystem;
class RouteMapWidget;

class TrackingPage : public QWidget
{
    Q_OBJECT
public:
    explicit TrackingPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
    void selectDelivery(int deliveryId);
private slots:
    void onTrackClicked();
private:
    LogisticsSystem *system;
    QComboBox *orderCombo = nullptr;
    QLabel *custVal = nullptr;
    QLabel *pickVal = nullptr;
    QLabel *destVal = nullptr;
    QLabel *costVal = nullptr;
    QLabel *statVal = nullptr;
    QLabel *driverVal = nullptr;
    QLabel *vehVal = nullptr;
    QLabel *distVal = nullptr;
    QWidget *stepperWidget = nullptr;
    RouteMapWidget *mapView = nullptr;
};
