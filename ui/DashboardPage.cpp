#include "DashboardPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include "Charts.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QHeaderView>
#include <algorithm>
#include <vector>

DashboardPage::DashboardPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setStyleSheet("QScrollArea{background:#f8fafc;border:none;} QScrollBar:horizontal{height:0px;}");

    pageWidget = new QWidget(scroll);
    pageWidget->setStyleSheet("background:#f8fafc;");
    scroll->setWidget(pageWidget);
    outer->addWidget(scroll);

    QVBoxLayout* root = new QVBoxLayout(pageWidget);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(Theme::SECTION_GAP);
    root->addWidget(Theme::pageHeader(pageWidget, "Dashboard", "Overview of orders, drivers and deliveries."));

    kpiGrid = new QGridLayout();
    kpiGrid->setSpacing(14);
    root->addLayout(kpiGrid);

    QHBoxLayout* mid = new QHBoxLayout();
    mid->setSpacing(14);
    QFrame* c1 = Theme::sectionCard(pageWidget, "Order Status", "Share of orders in each stage");
    donut = new DonutChart(c1);
    donut->setMinimumHeight(240);
    qobject_cast<QVBoxLayout*>(c1->layout())->addWidget(donut);

    QFrame* c2 = Theme::sectionCard(pageWidget, "Latest Route", "Route of the most recent order");
    mapPreview = new RouteMapWidget(c2);
    mapPreview->setMinimumHeight(200);
    qobject_cast<QVBoxLayout*>(c2->layout())->addWidget(mapPreview);
    mid->addWidget(c1, 1);
    mid->addWidget(c2, 1);
    root->addLayout(mid);

    QFrame* c3 = Theme::sectionCard(pageWidget, "Recent Orders", "Latest orders, newest first");
    recent = new QTableWidget(c3);
    recent->setColumnCount(5);
    recent->setHorizontalHeaderLabels({"Order ID","Customer","Destination","Cost (₹)","Status"});
    Theme::styleCardTable(recent);
    recent->setMinimumHeight(240);
    qobject_cast<QVBoxLayout*>(c3->layout())->addWidget(recent);

    QHBoxLayout* bottom = new QHBoxLayout();
    bottom->setSpacing(14);
    activityCard = Theme::sectionCard(pageWidget, "Activity", "Latest changes in this session");
    activityList = new QListWidget(activityCard);
    activityList->setMinimumHeight(240);
    activityList->setStyleSheet(
        "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;font-size:12px;color:#0f172a;}"
        "QListWidget::item{padding:7px 10px;border-bottom:1px solid #f1f5f9;}");
    qobject_cast<QVBoxLayout*>(activityCard->layout())->addWidget(activityList);
    bottom->addWidget(c3, 2);
    bottom->addWidget(activityCard, 1);
    root->addLayout(bottom);

    root->addStretch();

    liveTimer = new QTimer(this);
    liveTimer->setInterval(Theme::LIVE_MS);
    connect(liveTimer, &QTimer::timeout, this, [this]{
        if (isVisible()) refresh();
    });
}

void DashboardPage::clearLayout(QLayout* l) {
    if (!l) return;
    QLayoutItem* it;
    while ((it = l->takeAt(0)) != nullptr) {
        if (QWidget* w = it->widget()) {
            w->hide();
            w->setParent(nullptr);
            delete w;
        }
        delete it;
    }
}

void DashboardPage::buildKpis(Role role) {
    clearLayout(kpiGrid);
    kpiSlots.clear();
    currentRoleInt = static_cast<int>(role);

    auto addCard = [&](int col, const QString& label, const QString& accent, const QString& caption) {
        QFrame* card = Theme::statCard(pageWidget, label, "0", accent, caption);
        QLabel* vl = card->findChild<QLabel*>("kpiValue");
        QLabel* cl = card->findChild<QLabel*>("kpiCaption");
        kpiGrid->addWidget(card, 0, col);
        kpiSlots.push_back(KpiSlot{vl, cl});
    };

    if (role == Role::LogisticsTeam) {
        addCard(0, "Total Orders",   Theme::SUCCESS, "All orders");
        addCard(1, "Pending",        Theme::WARN,    "Waiting for dispatch");
        addCard(2, "Delivered",      Theme::ACCENT,  "Completed orders");
        addCard(3, "Urgent Pending", Theme::DANGER,  "Urgent orders waiting");
    } else if (role == Role::DeliveryMan) {
        addCard(0, "My Orders",      Theme::ACCENT,  "Assigned to you");
        addCard(1, "Active",         Theme::WARN,    "Assigned or in transit");
        addCard(2, "Delivered",      Theme::SUCCESS, "Completed by you");
        addCard(3, "Unassigned",     Theme::VIOLET,  "Pending across the city");
    } else {
        addCard(0, "My Orders",      Theme::ACCENT,  "All your orders");
        addCard(1, "Pending",        Theme::WARN,    "Waiting for a driver");
        addCard(2, "Delivered",      Theme::SUCCESS, "Completed orders");
        addCard(3, "Total Spent",    Theme::SUCCESS, "Sum of your order costs");
    }
}

void DashboardPage::showEvent(QShowEvent* e) {
    QWidget::showEvent(e);
    refresh();
    if (liveTimer && !liveTimer->isActive()) liveTimer->start();
}

void DashboardPage::hideEvent(QHideEvent* e) {
    QWidget::hideEvent(e);
    if (liveTimer) liveTimer->stop();
}

void DashboardPage::refresh() {
    if (!system) return;
    User* u = system->getAuthManager()->getCurrentUser();
    Role role = u ? u->getRole() : Role::LogisticsTeam;

    if (currentRoleInt != static_cast<int>(role) || kpiSlots.isEmpty()) {
        buildKpis(role);
    }

    Driver* me = (role == Role::DeliveryMan && u) ? system->getDriverByUserId(u->getId()) : nullptr;
    std::vector<Delivery*> visible;
    for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it) {
        Delivery* d = *it;
        if (role == Role::Customer && u && d->getCustomerId() != u->getLinkedId()) continue;
        if (role == Role::DeliveryMan && (!me || d->getDriverId() != me->getId())) continue;
        visible.push_back(d);
    }

    int pending = 0, active = 0, done = 0, cancelled = 0, urgentPending = 0;
    double spend = 0.0;
    for (Delivery* d : visible) {
        spend += d->getCost();
        if (d->getStatus() == Status::Pending) {
            pending++;
            if (d->getPriority() == Priority::Urgent) urgentPending++;
        }
        if (d->getStatus() == Status::Assigned || d->getStatus() == Status::PickedUp || d->getStatus() == Status::InTransit) active++;
        if (d->getStatus() == Status::Delivered) done++;
        if (d->getStatus() == Status::Cancelled) cancelled++;
    }

    if (kpiSlots.size() >= 4) {
        if (role == Role::LogisticsTeam) {
            kpiSlots[0].val->setText(QString::number(visible.size()));
            kpiSlots[1].val->setText(QString::number(pending));
            kpiSlots[2].val->setText(QString::number(done));
            kpiSlots[3].val->setText(QString::number(urgentPending));
        } else if (role == Role::DeliveryMan) {
            kpiSlots[0].val->setText(QString::number(visible.size()));
            kpiSlots[1].val->setText(QString::number(active));
            kpiSlots[2].val->setText(QString::number(done));
            kpiSlots[3].val->setText(QString::number(system->getPendingCount()));
        } else {
            kpiSlots[0].val->setText(QString::number(visible.size()));
            kpiSlots[1].val->setText(QString::number(pending));
            kpiSlots[2].val->setText(QString::number(done));
            kpiSlots[3].val->setText(QString("₹%1").arg(spend, 0, 'f', 0));
        }
    }

    donut->setData({
        {"Delivered",  done,                      QColor(Theme::SUCCESS)},
        {"Pending",    pending,                   QColor(Theme::WARN)},
        {"Active",     active,                    QColor(Theme::ACCENT)},
        {"Cancelled",  cancelled,                 QColor(Theme::DANGER)},
    });

    std::sort(visible.begin(), visible.end(), [](Delivery* a, Delivery* b){ return a->getId() > b->getId(); });

    recent->setRowCount(0);
    int n = qMin(6, (int)visible.size());
    for (int i = 0; i < n; i++) {
        Delivery* d = visible[i];
        int r = recent->rowCount();
        recent->insertRow(r);

        auto* idItem = new QTableWidgetItem(QString("#%1").arg(d->getId()));
        idItem->setForeground(QBrush(QColor("#0f172a")));
        recent->setItem(r, 0, idItem);

        Customer* c = system->getCustomer(d->getCustomerId());
        auto* custItem = new QTableWidgetItem(c ? QString::fromStdString(c->getName()) : QString("Unknown"));
        custItem->setForeground(QBrush(QColor("#0f172a")));
        recent->setItem(r, 1, custItem);

        auto* destItem = new QTableWidgetItem(QString::fromStdString(d->getDestination()));
        destItem->setForeground(QBrush(QColor("#0f172a")));
        recent->setItem(r, 2, destItem);

        auto* costItem = new QTableWidgetItem(QString("₹%1").arg(d->getCost(), 0, 'f', 2));
        costItem->setForeground(QBrush(QColor("#047857")));
        QFont cf = costItem->font(); cf.setBold(true); costItem->setFont(cf);
        costItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        recent->setItem(r, 3, costItem);

        auto* statusItem = new QTableWidgetItem(QString::fromStdString(d->getStatusString()));
        Theme::setPill(statusItem, QString::fromStdString(d->getStatusString()));
        recent->setItem(r, 4, statusItem);
    }

    activityCard->setVisible(role == Role::LogisticsTeam);
    activityList->clear();
    if (role == Role::LogisticsTeam) {
        for (const auto& ev : system->getRecentEvents(60)) {
            QString when = QString::fromStdString(ev.time).mid(11, 5);
            auto* item = new QListWidgetItem(QString("%1    %2").arg(when, QString::fromStdString(ev.text)));
            item->setForeground(QBrush(QColor("#0f172a")));
            activityList->addItem(item);
        }
        if (activityList->count() == 0) {
            auto* empty = new QListWidgetItem("No activity yet. Orders, assignments and status changes appear here.");
            empty->setForeground(QBrush(QColor("#64748b")));
            activityList->addItem(empty);
        }
    }

    if (!visible.empty()) {
        Delivery* top = visible[0];
        double progress = top->getStatus() == Status::Delivered ? 1.0 :
                          top->getStatus() == Status::InTransit ? 0.65 :
                          top->getStatus() == Status::PickedUp ? 0.45 :
                          top->getStatus() == Status::Assigned  ? 0.30 :
                          top->getStatus() == Status::Pending   ? 0.10 : 0.0;
        mapPreview->setRoute(QString::fromStdString(top->getPickupLocation()),
                             QString::fromStdString(top->getDestination()),
                             QString::fromStdString(top->getStatusString()),
                             progress);
    } else {
        mapPreview->clearRoute();
    }
}
