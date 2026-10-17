#pragma once
#include <QWidget>
#include <QListWidget>
#include <QLabel>

class LogisticsSystem;

class DsaViewerPage : public QWidget
{
    Q_OBJECT
public:
    explicit DsaViewerPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void refresh();
private:
    LogisticsSystem *system;
    QLabel *statsLabel = nullptr;
    QListWidget *queueView = nullptr;
    QListWidget *urgentView = nullptr;
    QListWidget *batchView = nullptr;
    QLabel *sequenceLabel = nullptr;
};
