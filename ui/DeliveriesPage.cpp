#include "DeliveriesPage.h"
#include "DeliveryDetails.h"
#include "../core/LogisticsSystem.h"
#include "../logic/Sorting.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QHeaderView>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QListWidget>
#include <algorithm>
#include <vector>

namespace {

const char* kHintIdle = "color:#475569;font-size:12px;background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;padding:10px 12px;";
const char* kHintOk   = "color:#166534;font-size:12px;font-weight:600;background:#f0fdf4;border:1px solid #bbf7d0;border-radius:8px;padding:10px 12px;";
const char* kHintBad  = "color:#991b1b;font-size:12px;font-weight:600;background:#fef2f2;border:1px solid #fecaca;border-radius:8px;padding:10px 12px;";

void refreshRouteHint(LogisticsSystem* sys, const QString& pickup, const QString& destination,
                      Priority priority, QLabel* hint) {
    if (pickup.trimmed().isEmpty() || destination.trimmed().isEmpty()) {
        hint->setText("Enter a pickup and a destination to see the route and the price.");
        hint->setStyleSheet(kHintIdle);
        return;
    }
    LocationResult from = sys->resolveLocation(pickup.toStdString());
    if (!from.ok) {
        hint->setText("Pickup: " + QString::fromStdString(from.error));
        hint->setStyleSheet(kHintBad);
        return;
    }
    LocationResult to = sys->resolveLocation(destination.toStdString());
    if (!to.ok) {
        hint->setText("Destination: " + QString::fromStdString(to.error));
        hint->setStyleSheet(kHintBad);
        return;
    }
    RouteResult route = sys->routeBetween(pickup.toStdString(), destination.toStdString());
    if (!route.ok) {
        hint->setText(QString::fromStdString(route.error));
        hint->setStyleSheet(kHintBad);
        return;
    }
    QStringList path;
    for (const auto& area : route.areas) path << QString::fromStdString(area);
    double cost = Delivery::calculateCost(route.distanceKm, priority);
    hint->setText(QString("Route: %1\nDistance: %2 km     Estimated cost: ₹%3")
        .arg(path.join(" → "))
        .arg(route.distanceKm, 0, 'f', 2)
        .arg(cost, 0, 'f', 2));
    hint->setStyleSheet(kHintOk);
}

int selectedId(QTableWidget* t) {
    auto rows = t->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    QString txt = t->item(rows[0].row(), 0)->text();
    return txt.remove('#').trimmed().toInt();
}

Priority priorityFromBox(QComboBox* box) {
    QString text = box->currentText();
    if (text == "Urgent") return Priority::Urgent;
    if (text == "High") return Priority::High;
    return Priority::Normal;
}

void selectPriority(QComboBox* box, Priority p) {
    box->setCurrentIndex(p == Priority::Urgent ? 2 : p == Priority::High ? 1 : 0);
}

}

DeliveriesPage::DeliveriesPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    User* u = system->getAuthManager()->getCurrentUser();
    Role role = u ? u->getRole() : Role::LogisticsTeam;
    bool isAdmin    = role == Role::LogisticsTeam;
    bool isCustomer = role == Role::Customer;
    bool isDriver   = role == Role::DeliveryMan;

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(Theme::SECTION_GAP);

    QHBoxLayout* topRow = new QHBoxLayout();
    QWidget* header = Theme::pageHeader(this, "Orders",
        "Create orders, assign drivers, update status and find nearby pickups.");
    QList<QLabel*> labs = header->findChildren<QLabel*>();
    if (labs.size() > 0) headTitle = labs[0];
    if (labs.size() > 1) headSub   = labs[1];
    topRow->addWidget(header);
    topRow->addStretch();

    QPushButton* addBtn = new QPushButton("New Order", this);
    Theme::stylePrimaryBtn(addBtn, Theme::ACCENT);
    addBtn->setMinimumHeight(Theme::BTN_H);
    addBtn->setVisible(isAdmin || isCustomer);
    topRow->addWidget(addBtn, 0, Qt::AlignTop);
    root->addLayout(topRow);

    QFrame* tools = Theme::sectionCard(this);
    QVBoxLayout* tl = qobject_cast<QVBoxLayout*>(tools->layout());
    QHBoxLayout* t1 = new QHBoxLayout();
    t1->setSpacing(8);
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Search by order ID, customer, location or status");
    searchEdit->setClearButtonEnabled(true);
    statusFilter = new QComboBox(this);
    statusFilter->addItems({"All Statuses","Pending","Assigned","Picked Up","In Transit","Delivered","Cancelled"});
    priorityFilter = new QComboBox(this);
    priorityFilter->addItems({"All Priorities","Normal","High","Urgent","Batchable Only"});
    sortBox = new QComboBox(this);
    sortBox->addItems({"Newest First","Oldest First","Priority First"});
    for (auto w : {(QWidget*)searchEdit, (QWidget*)statusFilter, (QWidget*)priorityFilter, (QWidget*)sortBox})
        Theme::styleInput(w);
    t1->addWidget(searchEdit, 3);
    t1->addWidget(statusFilter, 1);
    t1->addWidget(priorityFilter, 1);
    t1->addWidget(sortBox, 1);
    tl->addLayout(t1);

    QHBoxLayout* t2 = new QHBoxLayout();
    t2->setSpacing(8);
    QPushButton* advBtn    = new QPushButton("Update Status",   this);
    Theme::stylePrimaryBtn(advBtn, Theme::ACCENT);
    QPushButton* assignBtn = new QPushButton("Assign Driver",   this);
    Theme::stylePrimaryBtn(assignBtn, Theme::VIOLET);
    QPushButton* editBtn   = new QPushButton("Edit",            this);
    QPushButton* cancelBtn = new QPushButton("Cancel Order",    this);
    QPushButton* nearBtn   = new QPushButton("Nearby Orders",   this);
    QPushButton* detBtn    = new QPushButton("View Details",    this);
    QPushButton* delBtn    = new QPushButton("Delete",          this);
    QPushButton* refBtn    = new QPushButton("Refresh",         this);
    for (auto b : {editBtn, cancelBtn, nearBtn, detBtn, refBtn}) Theme::styleGhostBtn(b);
    Theme::styleDangerBtn(delBtn);
    advBtn->setVisible(isAdmin || isDriver);
    assignBtn->setVisible(isAdmin);
    editBtn->setVisible(isAdmin);
    cancelBtn->setVisible(isAdmin || isCustomer);
    nearBtn->setVisible(isAdmin);
    delBtn->setVisible(isAdmin);
    t2->addWidget(detBtn);
    t2->addWidget(advBtn);
    t2->addWidget(assignBtn);
    t2->addWidget(editBtn);
    t2->addWidget(cancelBtn);
    t2->addWidget(nearBtn);
    t2->addWidget(delBtn);
    t2->addStretch();
    t2->addWidget(refBtn);
    tl->addLayout(t2);
    root->addWidget(tools);

    QFrame* list = Theme::sectionCard(this, "All Orders", "Double-click an order to see its details and history.");
    QVBoxLayout* ll = qobject_cast<QVBoxLayout*>(list->layout());
    table = new QTableWidget(list);
    table->setColumnCount(11);
    table->setHorizontalHeaderLabels({"Order","Customer","Pickup","Destination","Priority","Driver",
                                      "Status","Distance","Created","Cost (₹)","Batch"});
    Theme::styleCardTable(table);
    table->setMinimumHeight(340);
    ll->addWidget(table);

    QHBoxLayout* pageRow = new QHBoxLayout();
    pageRow->setSpacing(6);
    countLabel = new QLabel(list);
    countLabel->setStyleSheet("color:#64748b;font-size:12px;");
    pageRow->addWidget(countLabel);
    pageRow->addStretch();
    ll->addLayout(pageRow);

    root->addWidget(list, 1);

    connect(addBtn,    &QPushButton::clicked, this, &DeliveriesPage::add);
    connect(editBtn,   &QPushButton::clicked, this, &DeliveriesPage::editSelected);
    connect(delBtn,    &QPushButton::clicked, this, &DeliveriesPage::removeSelected);
    connect(advBtn,    &QPushButton::clicked, this, &DeliveriesPage::advanceStatus);
    connect(assignBtn, &QPushButton::clicked, this, &DeliveriesPage::assignSelected);
    connect(cancelBtn, &QPushButton::clicked, this, &DeliveriesPage::cancelSelected);
    connect(nearBtn,   &QPushButton::clicked, this, &DeliveriesPage::showNearby);
    connect(detBtn,    &QPushButton::clicked, this, &DeliveriesPage::showDetails);
    connect(refBtn,    &QPushButton::clicked, this, &DeliveriesPage::refresh);
    connect(table,     &QTableWidget::itemDoubleClicked, this, &DeliveriesPage::showDetails);
    connect(searchEdit,     &QLineEdit::textChanged,          this, &DeliveriesPage::refresh);
    connect(statusFilter,   &QComboBox::currentTextChanged,   this, &DeliveriesPage::refresh);
    connect(priorityFilter, &QComboBox::currentTextChanged,   this, &DeliveriesPage::refresh);
    connect(sortBox,        &QComboBox::currentTextChanged,   this, &DeliveriesPage::refresh);
    refresh();
}

void DeliveriesPage::add() {
    User* u = system->getAuthManager()->getCurrentUser();
    bool isCustomer = u && u->getRole() == Role::Customer;

    QDialog d(this);
    d.setWindowTitle("New Order");
    d.setMinimumWidth(480);
    d.setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }"
    );
    QVBoxLayout* dl = new QVBoxLayout(&d);
    dl->setContentsMargins(24, 20, 24, 20);
    dl->setSpacing(12);

    QLabel* title = new QLabel("New Order", &d);
    QFont tf = title->font(); tf.setPointSize(15); tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet("color:#0f172a;");
    dl->addWidget(title);

    QFormLayout* f = new QFormLayout();
    f->setSpacing(10);
    QComboBox* cust = new QComboBox(&d);
    for (auto it = system->getAllCustomers().begin(); it != system->getAllCustomers().end(); ++it)
        cust->addItem(QString("#%1  ·  %2").arg((*it)->getId()).arg(QString::fromStdString((*it)->getName())), (*it)->getId());
    QLineEdit* pk = new QLineEdit(&d);
    pk->setPlaceholderText("Pickup area, e.g. Katraj");
    QLineEdit* dr = new QLineEdit(&d);
    dr->setPlaceholderText("Destination area, e.g. Swargate");
    QComboBox* pr = new QComboBox(&d);
    pr->addItems({"Normal","High","Urgent"});
    for (auto w : {(QWidget*)cust, (QWidget*)pk, (QWidget*)dr, (QWidget*)pr}) Theme::styleInput(w);
    if (isCustomer) {
        cust->setCurrentIndex(cust->findData(u->getLinkedId()));
        cust->setEnabled(false);
    }
    f->addRow("Customer", cust);
    f->addRow("Pickup", pk);
    f->addRow("Destination", dr);
    f->addRow("Priority", pr);
    dl->addLayout(f);

    QLabel* hint = new QLabel(&d);
    hint->setWordWrap(true);
    dl->addWidget(hint);

    QLabel* m = new QLabel("", &d);
    m->setWordWrap(true);
    m->setVisible(false);
    dl->addWidget(m);

    QLabel* note = new QLabel("We deliver within the service areas listed on the Service Areas page.", &d);
    note->setStyleSheet("color:#64748b;font-size:11px;font-weight:400;");
    note->setWordWrap(true);
    dl->addWidget(note);

    QPushButton* ok = new QPushButton("Create Order", &d);
    ok->setMinimumHeight(40);
    Theme::stylePrimaryBtn(ok, Theme::SUCCESS);
    dl->addWidget(ok);

    auto updateHint = [&]() {
        refreshRouteHint(system, pk->text(), dr->text(), priorityFromBox(pr), hint);
    };
    connect(pk, &QLineEdit::textChanged, &d, updateHint);
    connect(dr, &QLineEdit::textChanged, &d, updateHint);
    connect(pr, &QComboBox::currentTextChanged, &d, updateHint);
    updateHint();

    connect(ok, &QPushButton::clicked, &d, [&]{
        if (!system->addDelivery(cust->currentData().toInt(), pk->text().toStdString(),
                                 dr->text().toStdString(), priorityFromBox(pr))) {
            m->setText(QString::fromStdString(system->lastError()));
            m->setStyleSheet("color:#dc2626;font-size:12px;font-weight:600;background:#fef2f2;border:1px solid #fecaca;border-radius:6px;padding:6px;");
            m->setVisible(true);
            return;
        }
        d.accept();
    });
    if (d.exec() == QDialog::Accepted) {
        refresh();
        Theme::toast(this, "Order created.", true);
    }
}

void DeliveriesPage::editSelected() {
    int id = selectedId(table);
    if (id < 0) { Theme::toast(this, "Select an order first.", false); return; }
    Delivery* dd = system->getDelivery(id);
    if (!dd) return;
    if (dd->getStatus() != Status::Pending) { Theme::toast(this, "Only pending orders can be edited.", false); return; }

    QDialog d(this);
    d.setWindowTitle(QString("Edit Order #%1").arg(id));
    d.setMinimumWidth(480);
    d.setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }"
    );
    QVBoxLayout* dl = new QVBoxLayout(&d);
    dl->setContentsMargins(24, 20, 24, 20);
    dl->setSpacing(12);

    QLabel* title = new QLabel(QString("Edit Order #%1").arg(id), &d);
    QFont tf = title->font(); tf.setPointSize(14); tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet("color:#0f172a;");
    dl->addWidget(title);

    QFormLayout* f = new QFormLayout();
    f->setSpacing(10);
    QLineEdit* pk = new QLineEdit(QString::fromStdString(dd->getPickupLocation()), &d);
    QLineEdit* dr = new QLineEdit(QString::fromStdString(dd->getDestination()), &d);
    QComboBox* pr = new QComboBox(&d);
    pr->addItems({"Normal","High","Urgent"});
    selectPriority(pr, dd->getPriority());
    for (auto w : {(QWidget*)pk, (QWidget*)dr, (QWidget*)pr}) Theme::styleInput(w);
    f->addRow("Pickup", pk);
    f->addRow("Destination", dr);
    f->addRow("Priority", pr);
    dl->addLayout(f);

    QLabel* hint = new QLabel(&d);
    hint->setWordWrap(true);
    dl->addWidget(hint);

    QPushButton* ok = new QPushButton("Save Changes", &d);
    ok->setMinimumHeight(40);
    Theme::stylePrimaryBtn(ok);
    dl->addWidget(ok);

    auto updateHint = [&]() {
        refreshRouteHint(system, pk->text(), dr->text(), priorityFromBox(pr), hint);
    };
    connect(pk, &QLineEdit::textChanged, &d, updateHint);
    connect(dr, &QLineEdit::textChanged, &d, updateHint);
    connect(pr, &QComboBox::currentTextChanged, &d, updateHint);
    updateHint();

    connect(ok, &QPushButton::clicked, &d, [&]{
        if (!system->updateDelivery(id, pk->text().toStdString(), dr->text().toStdString(), priorityFromBox(pr))) {
            Theme::toast(&d, QString::fromStdString(system->lastError()), false);
            return;
        }
        d.accept();
    });
    if (d.exec() == QDialog::Accepted) { refresh(); Theme::toast(this, "Order updated.", true); }
}

void DeliveriesPage::removeSelected() {
    User* u = system->getAuthManager()->getCurrentUser();
    if (!u || u->getRole() != Role::LogisticsTeam) return;
    int id = selectedId(table);
    if (id < 0) { Theme::toast(this, "Select an order first.", false); return; }
    if (QMessageBox::question(this, "Delete Order",
                              QString("Delete order #%1 permanently?").arg(id),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    if (system->removeDelivery(id)) { refresh(); Theme::toast(this, "Order deleted.", true); }
    else Theme::toast(this, QString::fromStdString(system->lastError()), false);
}

void DeliveriesPage::advanceStatus() {
    int id = selectedId(table);
    if (id < 0) { Theme::toast(this, "Select an order first.", false); return; }
    Delivery* dd = system->getDelivery(id);
    if (!dd) return;
    Status current = dd->getStatus();
    Status next;
    if (current == Status::Assigned)       next = Status::PickedUp;
    else if (current == Status::PickedUp)  next = Status::InTransit;
    else if (current == Status::InTransit) next = Status::Delivered;
    else if (current == Status::Pending) {
        Theme::toast(this, "Assign a driver before updating this order.", false);
        return;
    } else {
        Theme::toast(this, "This order is already finished.", false);
        return;
    }
    if (system->updateDeliveryStatus(id, next)) {
        refresh();
        Theme::toast(this, QString("Order #%1 is now %2.").arg(id).arg(QString::fromStdString(dd->getStatusString())), true);
    } else {
        Theme::toast(this, QString::fromStdString(system->lastError()), false);
    }
}

void DeliveriesPage::cancelSelected() {
    int id = selectedId(table);
    if (id < 0) { Theme::toast(this, "Select an order first.", false); return; }
    if (system->updateDeliveryStatus(id, Status::Cancelled)) { refresh(); Theme::toast(this, "Order cancelled. The driver is released.", true); }
    else Theme::toast(this, QString::fromStdString(system->lastError()), false);
}

void DeliveriesPage::assignSelected() {
    int id = selectedId(table);
    if (id < 0) { Theme::toast(this, "Select an order first.", false); return; }
    Delivery* dd = system->getDelivery(id);
    if (!dd) return;
    if (dd->getStatus() != Status::Pending) { Theme::toast(this, "Only pending orders can be assigned.", false); return; }

    QDialog d(this);
    d.setWindowTitle(QString("Assign Order #%1").arg(id));
    d.setMinimumWidth(420);
    d.setStyleSheet("QDialog { background-color: #ffffff; } QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }");
    QVBoxLayout* dl = new QVBoxLayout(&d);
    dl->setContentsMargins(24, 20, 24, 20);
    dl->setSpacing(12);
    QLabel* title = new QLabel(QString("Choose an available driver for order #%1").arg(id), &d);
    title->setWordWrap(true);
    dl->addWidget(title);

    QComboBox* drivers = new QComboBox(&d);
    for (auto it = system->getAllDrivers().begin(); it != system->getAllDrivers().end(); ++it) {
        if (!(*it)->isAvailable()) continue;
        drivers->addItem(QString("#%1  ·  %2 (%3)").arg((*it)->getId())
            .arg(QString::fromStdString((*it)->getName())).arg(QString::fromStdString((*it)->getVehicle())),
            (*it)->getId());
    }
    Theme::styleInput(drivers);
    dl->addWidget(drivers);
    if (drivers->count() == 0) {
        QLabel* none = new QLabel("No drivers are available right now.", &d);
        none->setStyleSheet("color:#991b1b;font-weight:600;");
        dl->addWidget(none);
    }
    QPushButton* ok = new QPushButton("Assign", &d);
    ok->setMinimumHeight(40);
    Theme::stylePrimaryBtn(ok, Theme::VIOLET);
    ok->setEnabled(drivers->count() > 0);
    dl->addWidget(ok);
    connect(ok, &QPushButton::clicked, &d, &QDialog::accept);

    if (d.exec() != QDialog::Accepted) return;
    int driverId = drivers->currentData().toInt();
    if (system->assignManually(id, driverId)) {
        refresh();
        Theme::toast(this, QString("Order #%1 assigned.").arg(id), true);
    } else {
        Theme::toast(this, QString::fromStdString(system->lastError()), false);
    }
}

void DeliveriesPage::showNearby() {
    int id = selectedId(table);
    if (id < 0) { Theme::toast(this, "Select an order first.", false); return; }
    auto near = system->findNearbyDeliveries(id);
    QStringList lines;
    lines << QString("Pending orders near #%1 (within %2 km):").arg(id).arg(system->getBatchRadiusKm(), 0, 'f', 2);
    int cnt = 0;
    for (auto it = near.begin(); it != near.end(); ++it) {
        Delivery* d = *it;
        double dist = system->deliveryDistanceKm(id, d->getId());
        lines << QString("  • #%1   %2   (%3 km)").arg(d->getId())
            .arg(QString::fromStdString(d->getPickupLocation())).arg(dist, 0, 'f', 2);
        cnt++;
    }
    if (cnt == 0) lines << "  No nearby pending orders within the current radius.";
    else          lines << QString("\nThese %1 orders can be assigned to one driver together.").arg(cnt + 1);
    QMessageBox::information(this, "Nearby Orders", lines.join("\n"));
}

void DeliveriesPage::showDetails() {
    int id = selectedId(table);
    if (id < 0) { Theme::toast(this, "Select an order first.", false); return; }
    DeliveryDetails dlg(system, id, this);
    dlg.exec();
}

void DeliveriesPage::refresh() {
    if (!system) return;
    User* u = system->getAuthManager()->getCurrentUser();
    Role role = u ? u->getRole() : Role::LogisticsTeam;
    if (headTitle && headSub) {
        if (role == Role::Customer) {
            headTitle->setText("My Orders");
            headSub->setText("Track your orders and cancel any that are still pending.");
        } else if (role == Role::DeliveryMan) {
            headTitle->setText("My Deliveries");
            headSub->setText("Orders assigned to you. Update the status as you complete each delivery.");
        } else {
            headTitle->setText("Orders");
            headSub->setText("Create orders, assign drivers, update status and find nearby pickups.");
        }
    }

    QString q  = searchEdit     ? searchEdit->text().trimmed().toLower()     : "";
    QString sf = statusFilter   ? statusFilter->currentText()                : "All Statuses";
    QString pf = priorityFilter ? priorityFilter->currentText()              : "All Priorities";

    Driver* me = (role == Role::DeliveryMan && u) ? system->getDriverByUserId(u->getId()) : nullptr;
    std::vector<Delivery*> v;
    for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it) {
        Delivery* d = *it;
        if (role == Role::Customer && u && d->getCustomerId() != u->getLinkedId()) continue;
        if (role == Role::DeliveryMan && (!me || d->getDriverId() != me->getId())) continue;
        if (!q.isEmpty()) {
            Customer* hc = system->getCustomer(d->getCustomerId());
            QString hay = QString("%1 %2 %3 %4 %5").arg(d->getId())
                .arg(hc ? QString::fromStdString(hc->getName()) : QString())
                .arg(QString::fromStdString(d->getPickupLocation()))
                .arg(QString::fromStdString(d->getDestination()))
                .arg(QString::fromStdString(d->getStatusString())).toLower();
            if (!hay.contains(q)) continue;
        }
        QString st = QString::fromStdString(d->getStatusString());
        if (sf != "All Statuses" && st != sf) continue;
        QString pr = QString::fromStdString(d->getPriorityString());
        if (pf == "Normal" || pf == "High" || pf == "Urgent") { if (pr != pf) continue; }
        if (pf == "Batchable Only") {
            auto near = system->findNearbyDeliveries(d->getId());
            bool any = false;
            for (auto it2 = near.begin(); it2 != near.end(); ++it2) { (void)*it2; any = true; break; }
            if (!any || d->getStatus() != Status::Pending) continue;
        }
        v.push_back(d);
    }

    QString sort = sortBox ? sortBox->currentText() : "";
    int count = (int)v.size();
    if (sort.contains("Priority")) {
        Sorting<Delivery*>::sort(v.data(), count, [](Delivery* a, Delivery* b){
            if (a->getPriorityValue() != b->getPriorityValue())
                return a->getPriorityValue() > b->getPriorityValue();
            return a->getId() < b->getId();
        });
    } else if (sort.contains("Oldest")) {
        Sorting<Delivery*>::sort(v.data(), count, [](Delivery* a, Delivery* b){ return a->getId() < b->getId(); });
    } else {
        Sorting<Delivery*>::sort(v.data(), count, [](Delivery* a, Delivery* b){ return a->getId() > b->getId(); });
    }

    auto groups  = system->getBatchGroups();
    auto batchOf = [&](int id) -> QString {
        for (size_t i = 0; i < groups.size(); i++)
            for (int m : groups[i].memberIds)
                if (m == id) return QString("Group %1").arg(i + 1);
        return QString();
    };

    if (countLabel) countLabel->setText(QString("Showing %1 order(s)").arg((int)v.size()));

    table->setRowCount(0);
    for (Delivery* d : v) {
        int r = table->rowCount();
        table->insertRow(r);
        Customer* c  = system->getCustomer(d->getCustomerId());
        Driver*   dr = system->getDriver(d->getDriverId());

        auto* idItem = new QTableWidgetItem(QString("#%1").arg(d->getId()));
        idItem->setForeground(QBrush(QColor("#0f172a")));
        QFont idf = idItem->font(); idf.setBold(true); idItem->setFont(idf);
        table->setItem(r, 0, idItem);

        auto* custItem = new QTableWidgetItem(c ? QString::fromStdString(c->getName()) : QString("Customer #%1").arg(d->getCustomerId()));
        custItem->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 1, custItem);

        auto* pickItem = new QTableWidgetItem(QString::fromStdString(d->getPickupLocation()));
        pickItem->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 2, pickItem);

        auto* destItem = new QTableWidgetItem(QString::fromStdString(d->getDestination()));
        destItem->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 3, destItem);

        auto* prItem = new QTableWidgetItem(QString::fromStdString(d->getPriorityString()));
        Theme::setPill(prItem, QString::fromStdString(d->getPriorityString()));
        table->setItem(r, 4, prItem);

        auto* drItem = new QTableWidgetItem(dr ? QString::fromStdString(dr->getName()) : QString("Not assigned"));
        drItem->setForeground(QBrush(QColor(dr ? "#0f172a" : "#64748b")));
        table->setItem(r, 5, drItem);

        auto* stItem = new QTableWidgetItem(QString::fromStdString(d->getStatusString()));
        Theme::setPill(stItem, QString::fromStdString(d->getStatusString()));
        table->setItem(r, 6, stItem);

        double km = system->routeDistanceKm(d);
        auto* kmItem = new QTableWidgetItem(km >= 0 ? QString("%1").arg(km, 0, 'f', 2) : QString("—"));
        kmItem->setForeground(QBrush(QColor("#0f172a")));
        kmItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(r, 7, kmItem);

        auto* createdItem = new QTableWidgetItem(QString::fromStdString(d->getCreatedAt()));
        createdItem->setForeground(QBrush(QColor("#475569")));
        table->setItem(r, 8, createdItem);

        auto* costItem = new QTableWidgetItem(QString("₹%1").arg(d->getCost(), 0, 'f', 2));
        costItem->setForeground(QBrush(QColor("#047857")));
        QFont cf = costItem->font(); cf.setBold(true); costItem->setFont(cf);
        costItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        table->setItem(r, 9, costItem);

        QString batch = batchOf(d->getId());
        auto* batchItem = new QTableWidgetItem(batch.isEmpty() ? QString("—") : batch);
        batchItem->setTextAlignment(Qt::AlignCenter);
        batchItem->setForeground(QBrush(QColor(batch.isEmpty() ? "#94a3b8" : "#1d4ed8")));
        table->setItem(r, 10, batchItem);
    }
}
