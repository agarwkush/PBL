#include "ReportsPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include "Charts.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <map>
#include <algorithm>
#include <vector>

ReportsPage::ReportsPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(Theme::SECTION_GAP);

    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->addWidget(Theme::pageHeader(this, "Reports",
        "Totals and breakdowns calculated from current orders. Export the order list as a CSV file."));
    topRow->addStretch();
    refreshBtn = new QPushButton("Refresh", this);
    Theme::stylePrimaryBtn(refreshBtn);
    expBtn = new QPushButton("Export CSV", this);
    Theme::styleDarkBtn(expBtn);
    topRow->addWidget(refreshBtn);
    topRow->addWidget(expBtn);
    root->addLayout(topRow);

    summary = new QLabel(this);
    summary->setWordWrap(true);
    summary->setStyleSheet(
        "background:#ffffff;border:1px solid #e2e8f0;border-radius:12px;"
        "padding:14px 18px;font-weight:600;font-size:13px;color:#0f172a;");
    root->addWidget(summary);

    QHBoxLayout* row = new QHBoxLayout();
    row->setSpacing(14);

    QFrame* c1 = Theme::sectionCard(this, "Orders by Status");
    donut = new DonutChart(c1);
    donut->setMinimumHeight(240);
    qobject_cast<QVBoxLayout*>(c1->layout())->addWidget(donut);

    QFrame* c2 = Theme::sectionCard(this, "Orders by Priority");
    priorityChart = new BarChart(c2);
    priorityChart->setMinimumHeight(240);
    qobject_cast<QVBoxLayout*>(c2->layout())->addWidget(priorityChart);

    QFrame* c3 = Theme::sectionCard(this, "Top Destinations");
    topLocs = new HBarChart(c3);
    topLocs->setMinimumHeight(240);
    qobject_cast<QVBoxLayout*>(c3->layout())->addWidget(topLocs);

    row->addWidget(c1, 1);
    row->addWidget(c2, 1);
    row->addWidget(c3, 1);
    root->addLayout(row);
    root->addStretch();

    connect(refreshBtn, &QPushButton::clicked, this, &ReportsPage::refresh);
    connect(expBtn, &QPushButton::clicked, this, [this]{
        QString path = QFileDialog::getSaveFileName(this, "Export Orders", "orders.csv", "CSV Files (*.csv)");
        if (path.isEmpty()) return;
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Theme::toast(this, "Could not write to the selected file.", false);
            return;
        }
        QTextStream s(&f);
        s << "id,customer_id,pickup_location,destination,priority,status,cost_inr,driver_id\n";
        for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it) {
            Delivery* d = *it;
            auto q = [](const std::string& v){
                QString t = QString::fromStdString(v);
                t.replace("\"", "\"\"");
                return "\"" + t + "\"";
            };
            s << d->getId() << "," << d->getCustomerId() << ","
              << q(d->getPickupLocation()) << "," << q(d->getDestination()) << ","
              << QString::fromStdString(d->getPriorityString()) << ","
              << QString::fromStdString(d->getStatusString()) << ","
              << QString::number(d->getCost(), 'f', 2) << ","
              << d->getDriverId() << "\n";
        }
        Theme::toast(this, "Orders exported to CSV.", true);
    });
    refresh();
}

void ReportsPage::refresh() {
    if (!system) return;
    int total = system->getTotalDeliveryCount();
    int done  = system->getDeliveredCount();
    double rate = total ? (100.0 * done / total) : 0;
    double revenue = system->getTotalRevenue();
    summary->setText(QString("Total orders: %1    Delivered: %2 (%3%)    Revenue: ₹%4    Pending: %5    Cancelled: %6    Batches: %7    Drivers: %8 (%9 available)    Customers: %10")
        .arg(total).arg(done).arg(rate, 0, 'f', 1).arg(revenue, 0, 'f', 2).arg(system->getPendingCount())
        .arg(system->getCancelledCount()).arg(system->getBatchCount())
        .arg(system->getTotalDriverCount()).arg(system->getAvailableDriverCount())
        .arg(system->getTotalCustomerCount()));

    donut->setData({
        {"Delivered", done,                                                     QColor(Theme::SUCCESS)},
        {"Pending",   system->getPendingCount(),                                QColor(Theme::WARN)},
        {"Active",    system->getAssignedCount() + system->getInTransitCount(), QColor(Theme::ACCENT)},
        {"Cancelled", system->getCancelledCount(),                              QColor(Theme::DANGER)},
    });

    int normalCount = 0, highCount = 0, urgentCount = 0;
    std::map<std::string, int> destCounts;
    for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it) {
        if ((*it)->getPriority() == Priority::Urgent) urgentCount++;
        else if ((*it)->getPriority() == Priority::High) highCount++;
        else normalCount++;
        std::string dest = (*it)->getDestination();
        size_t comma = dest.find(',');
        if (comma != std::string::npos) dest = dest.substr(0, comma);
        destCounts[dest]++;
    }
    priorityChart->setData({
        {"Normal", normalCount, QColor(Theme::ACCENT)},
        {"High",   highCount,   QColor(Theme::WARN)},
        {"Urgent", urgentCount, QColor(Theme::DANGER)},
    });

    std::vector<HBarChart::Item> items;
    for (const auto& kv : destCounts)
        items.push_back({QString::fromStdString(kv.first), kv.second});
    std::sort(items.begin(), items.end(), [](const HBarChart::Item& a, const HBarChart::Item& b){
        return a.count > b.count;
    });
    if (items.size() > 6) items.resize(6);
    topLocs->setData(items);
}
