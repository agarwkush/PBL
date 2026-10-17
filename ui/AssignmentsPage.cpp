#include "AssignmentsPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QFrame>
#include <QListWidget>
#include <algorithm>
#include <vector>

static int idFromListText(const QString& t, bool batchOnly) {
    if (batchOnly && !t.startsWith("Batch")) return 0;
    int h = t.indexOf('#');
    if (h < 0) return 0;
    QString num;
    for (int i = h + 1; i < t.size() && t[i].isDigit(); i++) num += t[i];
    return num.toInt();
}

static void selectById(QComboBox* box, int id) {
    int idx = box->findData(id);
    if (idx >= 0) box->setCurrentIndex(idx);
}

AssignmentsPage::AssignmentsPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(12);
    root->addWidget(Theme::pageHeader(this, "Assignments",
        "Urgent orders are assigned first, then the oldest pending orders. Nearby pickups can go to one driver as a batch."));

    QHBoxLayout* cols = new QHBoxLayout();
    cols->setSpacing(12);

    QFrame* c1 = Theme::sectionCard(this, "Pending Orders", "Urgent first, then oldest first");
    pendingList = new QListWidget(c1);
    pendingList->setMinimumHeight(260);
    pendingList->setStyleSheet(
        "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;color:#0f172a;}"
        "QListWidget::item{padding:7px 10px;border-bottom:1px solid #f1f5f9;font-size:12px;}"
        "QListWidget::item:selected{background:#dbeafe;color:#0f172a;}");
    qobject_cast<QVBoxLayout*>(c1->layout())->addWidget(pendingList);

    QFrame* c2 = Theme::sectionCard(this, "Batch Groups", "Pending pickups near each other");
    batchList = new QListWidget(c2);
    batchList->setMinimumHeight(260);
    batchList->setStyleSheet(
        "QListWidget{background:#ffffff;border:1px solid #e2e8f0;border-radius:8px;color:#0f172a;}"
        "QListWidget::item{padding:7px 10px;border-bottom:1px solid #f1f5f9;font-size:12px;}"
        "QListWidget::item:selected{background:#dbeafe;color:#0f172a;}");
    qobject_cast<QVBoxLayout*>(c2->layout())->addWidget(batchList);

    QFrame* c3 = Theme::sectionCard(this, "Available Drivers", "Drivers ready for a new order");
    driverList = new QListWidget(c3);
    driverList->setMinimumHeight(260);
    driverList->setStyleSheet(
        "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;color:#0f172a;}"
        "QListWidget::item{padding:7px 10px;border-bottom:1px solid #f1f5f9;font-size:12px;}"
        "QListWidget::item:selected{background:#dcfce7;color:#0f172a;}");
    qobject_cast<QVBoxLayout*>(c3->layout())->addWidget(driverList);

    cols->addWidget(c1, 1);
    cols->addWidget(c2, 1);
    cols->addWidget(c3, 1);
    root->addLayout(cols, 1);

    QFrame* card = Theme::sectionCard(this, "Assign");
    QVBoxLayout* c = qobject_cast<QVBoxLayout*>(card->layout());
    QHBoxLayout* ids = new QHBoxLayout();
    ids->setSpacing(8);
    deliveryBox = new QComboBox(card);
    deliveryBox->addItem("Select a pending order", 0);
    driverBox = new QComboBox(card);
    driverBox->addItem("Select an available driver", 0);
    for (auto w : {(QWidget*)deliveryBox, (QWidget*)driverBox}) Theme::styleInput(w);
    ids->addWidget(deliveryBox, 1);
    ids->addWidget(driverBox, 1);
    c->addLayout(ids);

    QHBoxLayout* btns = new QHBoxLayout();
    btns->setSpacing(8);
    QPushButton* autoBtn  = new QPushButton("Assign Next", card);
    Theme::stylePrimaryBtn(autoBtn, Theme::SUCCESS);
    QPushButton* batchBtn = new QPushButton("Assign Batch", card);
    Theme::stylePrimaryBtn(batchBtn, Theme::VIOLET);
    QPushButton* btn      = new QPushButton("Assign Selected Order", card);
    Theme::stylePrimaryBtn(btn, Theme::ACCENT);
    btns->addWidget(autoBtn);
    btns->addWidget(batchBtn);
    btns->addWidget(btn);
    btns->addStretch();
    c->addLayout(btns);

    msg = new QLabel("", card);
    msg->setWordWrap(true);
    msg->setVisible(false);
    c->addWidget(msg);
    root->addWidget(card);

    connect(btn,      &QPushButton::clicked, this, &AssignmentsPage::assign);
    connect(autoBtn,  &QPushButton::clicked, this, &AssignmentsPage::autoAssign);
    connect(batchBtn, &QPushButton::clicked, this, &AssignmentsPage::assignBatch);
    connect(pendingList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it){
        selectById(deliveryBox, idFromListText(it->text(), false));
    });
    connect(driverList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it){
        selectById(driverBox, idFromListText(it->text(), false));
    });
    connect(batchList, &QListWidget::itemClicked, this, [this](QListWidgetItem* it){
        selectById(deliveryBox, idFromListText(it->text(), true));
    });
    refresh();
}

void AssignmentsPage::refresh() {
    if (!system) return;
    int prevDelivery = deliveryBox->currentData().toInt();
    int prevDriver   = driverBox->currentData().toInt();

    std::vector<Delivery*> pending;
    for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it)
        if ((*it)->getStatus() == Status::Pending) pending.push_back(*it);
    std::sort(pending.begin(), pending.end(), [](Delivery* a, Delivery* b){
        if (a->getPriorityValue() != b->getPriorityValue())
            return a->getPriorityValue() > b->getPriorityValue();
        return a->getId() < b->getId();
    });

    pendingList->clear();
    deliveryBox->clear();
    deliveryBox->addItem("Select a pending order", 0);
    for (auto* d : pending) {
        QString label = QString("#%1  ·  %2 → %3  (%4)")
            .arg(d->getId())
            .arg(QString::fromStdString(d->getPickupLocation()).split(',').first().trimmed())
            .arg(QString::fromStdString(d->getDestination()).split(',').first().trimmed())
            .arg(QString::fromStdString(d->getPriorityString()));
        auto* pit = new QListWidgetItem(label);
        pit->setForeground(QBrush(QColor("#0f172a")));
        pendingList->addItem(pit);
        deliveryBox->addItem(label, d->getId());
    }
    if (pending.empty()) {
        auto* pit = new QListWidgetItem("No pending orders.");
        pit->setForeground(QBrush(QColor("#64748b")));
        pendingList->addItem(pit);
    }
    selectById(deliveryBox, prevDelivery);

    batchList->clear();
    auto groups = system->getBatchGroups();
    if (groups.empty()) {
        auto* bit = new QListWidgetItem(QString("No batch groups within %1 km.").arg(system->getBatchRadiusKm(), 0, 'f', 2));
        bit->setForeground(QBrush(QColor("#64748b")));
        batchList->addItem(bit);
    }
    for (size_t i = 0; i < groups.size(); i++) {
        QStringList ids;
        for (int m : groups[i].memberIds) ids << QString("#%1").arg(m);
        Delivery* a = system->getDelivery(groups[i].anchorId);
        auto* bit = new QListWidgetItem(QString("Batch %1: %2  ·  %3").arg(i + 1).arg(ids.join(" + "))
            .arg(a ? QString::fromStdString(a->getPickupLocation()) : ""));
        bit->setForeground(QBrush(QColor("#0f172a")));
        batchList->addItem(bit);
    }

    driverList->clear();
    driverBox->clear();
    driverBox->addItem("Select an available driver", 0);
    for (auto it = system->getAllDrivers().begin(); it != system->getAllDrivers().end(); ++it) {
        Driver* d = *it;
        if (!d->isAvailable()) continue;
        QString label = QString("#%1  ·  %2 (%3)").arg(d->getId())
            .arg(QString::fromStdString(d->getName()))
            .arg(QString::fromStdString(d->getVehicle()));
        auto* dit = new QListWidgetItem(label);
        dit->setForeground(QBrush(QColor("#0f172a")));
        driverList->addItem(dit);
        driverBox->addItem(label, d->getId());
    }
    if (driverList->count() == 0) {
        auto* dit = new QListWidgetItem("No drivers are available.");
        dit->setForeground(QBrush(QColor("#64748b")));
        driverList->addItem(dit);
    }
    selectById(driverBox, prevDriver);
}

void AssignmentsPage::assign() {
    int did  = deliveryBox->currentData().toInt();
    int drid = driverBox->currentData().toInt();
    if (did <= 0 || drid <= 0) {
        msg->setVisible(true);
        Theme::setMsg(msg, false, "Select both a pending order and an available driver.");
        return;
    }
    if (system->assignManually(did, drid)) {
        msg->setVisible(true);
        Theme::setMsg(msg, true, QString("Order #%1 assigned to driver #%2.").arg(did).arg(drid));
        refresh();
    } else {
        msg->setVisible(true);
        Theme::setMsg(msg, false, "Not assigned. The order must be pending and the driver must be available.");
    }
}

void AssignmentsPage::autoAssign() {
    msg->setVisible(true);
    if (system->assignNextBatch()) {
        Theme::setMsg(msg, true, "Batch assigned to an available driver.");
        refresh();
    } else if (system->assignNextDelivery()) {
        Theme::setMsg(msg, true, "Order assigned. Urgent orders are assigned first.");
        refresh();
    } else {
        Theme::setMsg(msg, false, "Nothing to assign. There are no pending orders or no available drivers.");
    }
}

void AssignmentsPage::assignBatch() {
    int anchor = deliveryBox->currentData().toInt();
    int drid   = driverBox->currentData().toInt();
    if (anchor <= 0 || drid <= 0) {
        msg->setVisible(true);
        Theme::setMsg(msg, false, "Select a pending order and an available driver.");
        return;
    }
    std::vector<int> ids; ids.push_back(anchor);
    auto near = system->findNearbyDeliveries(anchor);
    for (auto it = near.begin(); it != near.end(); ++it) ids.push_back((*it)->getId());
    if (system->assignBatch(ids, drid)) {
        msg->setVisible(true);
        Theme::setMsg(msg, true, QString("Batch of %1 orders assigned to driver #%2.").arg(ids.size()).arg(drid));
        refresh();
    } else {
        msg->setVisible(true);
        Theme::setMsg(msg, false, "Batch not assigned. All orders must be pending and the driver must be available.");
    }
}
