#pragma once
#include <QWidget>
#include <QColor>
#include <QString>
#include <vector>

class DonutChart : public QWidget {
    Q_OBJECT
public:
    struct Slice { QString label; int value; QColor color; };
    explicit DonutChart(QWidget* parent = nullptr);
    void setData(const std::vector<Slice>& slices);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<Slice> m_slices;
};

class BarChart : public QWidget {
    Q_OBJECT
public:
    struct Bar { QString label; int value; QColor color; };
    explicit BarChart(QWidget* parent = nullptr);
    void setData(const std::vector<Bar>& bars);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<Bar> m_bars;
};

class HBarChart : public QWidget {
    Q_OBJECT
public:
    struct Item { QString name; int count; };
    explicit HBarChart(QWidget* parent = nullptr);
    void setData(const std::vector<Item>& items);
protected:
    void paintEvent(QPaintEvent*) override;
private:
    std::vector<Item> m_items;
};

class RouteMapWidget : public QWidget {
    Q_OBJECT
public:
    explicit RouteMapWidget(QWidget* parent = nullptr);
    void setRoute(const QString& origin, const QString& destination,
                  const QString& status, double progress);
    void clearRoute();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    QString m_origin;
    QString m_destination;
    QString m_status;
    double m_progress = 0.0;
};
