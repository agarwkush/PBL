#include "DeliveryDetails.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QFont>
#include <QFrame>
#include <QPushButton>
#include <QListWidget>

static QString statusName(Status s) {
    switch (s) {
        case Status::Pending:   return "Pending";
        case Status::Assigned:  return "Assigned";
        case Status::PickedUp:  return "Picked Up";
        case Status::InTransit: return "In Transit";
        case Status::Delivered: return "Delivered";
        case Status::Cancelled: return "Cancelled";
    }
    return "Unknown";
}

DeliveryDetails::DeliveryDetails(LogisticsSystem* sys, int deliveryId, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(QString("Delivery Record — Order #%1").arg(deliveryId));
    setMinimumSize(480, 560);
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setSpacing(12);
    root->setContentsMargins(22, 18, 22, 18);

    Delivery* d = sys ? sys->getDelivery(deliveryId) : nullptr;
    if (!d) {
        root->addWidget(new QLabel("Delivery record not found.", this));
        QPushButton* c = new QPushButton("Close", this);
        Theme::styleDarkBtn(c);
        root->addWidget(c, 0, Qt::AlignRight);
        connect(c, &QPushButton::clicked, this, &QDialog::accept);
        return;
    }

    QWidget* headRow = new QWidget(this);
    QHBoxLayout* hr = new QHBoxLayout(headRow);
    hr->setContentsMargins(0, 0, 0, 0);
    QLabel* head = new QLabel(QString("Order #%1").arg(d->getId()), headRow);
    QFont hf = head->font(); hf.setPointSize(18); hf.setBold(true);
    head->setFont(hf);
    head->setStyleSheet("color:#0f172a;");
    QLabel* prio = new QLabel(QString::fromStdString(d->getPriorityString()), headRow);
    prio->setStyleSheet(QString("font-weight:700;font-size:11px;padding:4px 12px;border-radius:6px;background:%1;color:%2;")
        .arg(d->getPriority() == Priority::Urgent ? "#fee2e2" : "#f1f5f9",
             d->getPriority() == Priority::Urgent ? "#991b1b" : "#334155"));
    hr->addWidget(head);
    hr->addStretch();
    hr->addWidget(prio);
    root->addWidget(headRow);

    QFrame* card = new QFrame(this);
    card->setStyleSheet("QFrame{background:#ffffff;border:1px solid #e2e8f0;border-radius:10px;}");
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(16, 14, 16, 14);
    cardLayout->setSpacing(8);

    QFormLayout* f = new QFormLayout();
    f->setSpacing(8);
    f->setLabelAlignment(Qt::AlignRight);
    Customer* c = sys->getCustomer(d->getCustomerId());
    Driver* dr   = sys->getDriver(d->getDriverId());
    auto addRow = [&](const QString& k, const QString& v){
        QLabel* kl = new QLabel(k, card);
        kl->setStyleSheet("color:#64748b;font-weight:600;font-size:12px;");
        QLabel* vl = new QLabel(v, card);
        vl->setStyleSheet("color:#0f172a;font-weight:600;font-size:13px;");
        vl->setWordWrap(true);
        f->addRow(kl, vl);
    };
    addRow("Customer",    c  ? QString("%1 (#%2)").arg(QString::fromStdString(c->getName())).arg(c->getId())
                                 : QString("Customer #%1").arg(d->getCustomerId()));
    addRow("Pickup",      QString::fromStdString(d->getPickupLocation()));
    addRow("Destination", QString::fromStdString(d->getDestination()));
    addRow("Cost",        QString("₹%1 (%2)").arg(d->getCost(), 0, 'f', 2).arg(d->getPriority() == Priority::Urgent ? "urgent" : "normal"));
    addRow("Status",      QString::fromStdString(d->getStatusString()));
    double km = sys->routeDistanceKm(d);
    addRow("Distance",    km >= 0 ? QString("%1 km (service network)").arg(km, 0, 'f', 2) : QString("Not available"));
    addRow("Created",     QString::fromStdString(d->getCreatedAt()));
    addRow("Driver",      dr ? QString("%1 (#%2)").arg(QString::fromStdString(dr->getName())).arg(dr->getId())
                             : "Not assigned");
    cardLayout->addLayout(f);
    root->addWidget(card);

    QFrame* hist = Theme::sectionCard(this, "Status History", "Latest change first");
    QListWidget* histList = new QListWidget(hist);
    histList->setStyleSheet(
        "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;font-size:12px;color:#0f172a;}"
        "QListWidget::item{padding:6px 10px;border-bottom:1px solid #f1f5f9;}"
        "QListWidget::item:selected{background:#dbeafe;color:#0f172a;}");
    StatusHistoryNode* h = d->getStatusHistoryHead();
    int n = 0;
    while (h) {
        auto* it = new QListWidgetItem(QString("%1      %2").arg(statusName(h->status), -16).arg(QString::fromStdString(h->timestamp)));
        it->setForeground(QBrush(QColor("#0f172a")));
        histList->addItem(it);
        h = h->next;
        n++;
    }
    if (n == 0) {
        auto* it = new QListWidgetItem("No recorded state transitions.");
        it->setForeground(QBrush(QColor("#64748b")));
        histList->addItem(it);
    }
    histList->setMaximumHeight(130);
    qobject_cast<QVBoxLayout*>(hist->layout())->addWidget(histList);
    root->addWidget(hist);

    if (d->getStatus() == Status::Pending) {
        QFrame* bc = Theme::sectionCard(this, "Nearby Orders", "Other pending orders close to this pickup");
        QListWidget* near = new QListWidget(bc);
        near->setStyleSheet(
            "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;font-size:12px;color:#0f172a;}"
            "QListWidget::item{padding:6px 10px;border-bottom:1px solid #f1f5f9;}");
        auto mates = sys->findNearbyDeliveries(d->getId());
        int cnt = 0;
        for (auto it = mates.begin(); it != mates.end(); ++it) {
            Delivery* m = *it;
            double dist = sys->deliveryDistanceKm(d->getId(), m->getId());
            auto* nit = new QListWidgetItem(QString("Order #%1  ·  %2  (%3 km away)").arg(m->getId())
                .arg(QString::fromStdString(m->getPickupLocation())).arg(dist, 0, 'f', 2));
            nit->setForeground(QBrush(QColor("#0f172a")));
            near->addItem(nit);
            cnt++;
        }
        if (cnt == 0) {
            auto* nit = new QListWidgetItem(QString("No nearby pending orders within %1 km.").arg(sys->getBatchRadiusKm(), 0, 'f', 2));
            nit->setForeground(QBrush(QColor("#64748b")));
            near->addItem(nit);
        }
        near->setMaximumHeight(110);
        qobject_cast<QVBoxLayout*>(bc->layout())->addWidget(near);
        root->addWidget(bc);
    }

    QPushButton* close = new QPushButton("Close", this);
    Theme::styleDarkBtn(close);
    root->addWidget(close, 0, Qt::AlignRight);
    connect(close, &QPushButton::clicked, this, &QDialog::accept);
}
