#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
class LogisticsSystem;
class SearchPage : public QWidget
{
    Q_OBJECT
public:
    explicit SearchPage(LogisticsSystem *sys, QWidget *parent = nullptr);
    void setQuery(const QString& q);
private slots:
    void find();
private:
    LogisticsSystem *system;
    QLineEdit *idEdit;
    QLineEdit *textEdit;
    QComboBox *modeBox;
    QLabel *out;
    QTableWidget *table;
};
