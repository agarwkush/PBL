#pragma once
#include <QWidget>
#include <QListWidget>
#include <QComboBox>
#include <QLabel>

class LogisticsSystem;

class LocationsPage : public QWidget
{
    Q_OBJECT
public:
    explicit LocationsPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
private slots:
    void checkArea();
    void previewRoute();
private:
    LogisticsSystem *system;
    QListWidget *areaList = nullptr;
    QComboBox *checkBox = nullptr;
    QLabel *checkOut = nullptr;
    QComboBox *fromBox = nullptr;
    QComboBox *toBox = nullptr;
    QLabel *routeOut = nullptr;
};
