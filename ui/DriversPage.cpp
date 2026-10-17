#include "DriversPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include "Charts.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QFrame>
#include <vector>

namespace {

QString activeOrderList(LogisticsSystem* sys, int driverId) {
    QStringList ids;
    for (auto it = sys->getAllDeliveries().begin(); it != sys->getAllDeliveries().end(); ++it) {
        Delivery* d = *it;
        Status s = d->getStatus();
        if (d->getDriverId() == driverId && (s == Status::Assigned || s == Status::PickedUp || s == Status::InTransit))
            ids << QString("#%1").arg(d->getId());
    }
    return ids.isEmpty() ? QString("—") : ids.join(", ");
}

}

DriversPage::DriversPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(Theme::SECTION_GAP);

    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->addWidget(Theme::pageHeader(this, "Drivers",
        "Manage drivers. Only drivers who are Available can be given a new order."));
    topRow->addStretch();
    QPushButton* addBtn = new QPushButton("Add Driver", this);
    Theme::stylePrimaryBtn(addBtn, Theme::ACCENT);
    addBtn->setMinimumHeight(Theme::BTN_H);
    topRow->addWidget(addBtn, 0, Qt::AlignTop);
    root->addLayout(topRow);

    QFrame* tools = Theme::sectionCard(this);
    QHBoxLayout* t = new QHBoxLayout();
    t->setSpacing(8);
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Search by name, vehicle or driver ID");
    searchEdit->setClearButtonEnabled(true);
    Theme::styleInput(searchEdit);
    statusFilter = new QComboBox(this);
    statusFilter->addItems({"All Drivers", "Available", "Assigned", "Unavailable"});
    Theme::styleInput(statusFilter);
    QPushButton* editBtn = new QPushButton("Edit", this);
    toggleBtn = new QPushButton("Mark Unavailable", this);
    QPushButton* delBtn = new QPushButton("Remove", this);
    Theme::styleGhostBtn(editBtn);
    Theme::styleGhostBtn(toggleBtn);
    Theme::styleDangerBtn(delBtn);
    t->addWidget(searchEdit, 3);
    t->addWidget(statusFilter, 1);
    t->addWidget(editBtn);
    t->addWidget(toggleBtn);
    t->addWidget(delBtn);
    qobject_cast<QVBoxLayout*>(tools->layout())->addLayout(t);
    root->addWidget(tools);

    QFrame* list = Theme::sectionCard(this, "Driver List", "Double-click a driver to see their active orders.");
    table = new QTableWidget(list);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"ID","Driver Name","Vehicle","Availability","Active Orders"});
    Theme::styleCardTable(table);
    table->setMinimumHeight(260);
    qobject_cast<QVBoxLayout*>(list->layout())->addWidget(table);
    root->addWidget(list);

    QHBoxLayout* botRow = new QHBoxLayout();
    botRow->setSpacing(14);
    QFrame* c1 = Theme::sectionCard(this, "Active Orders per Driver", "Orders currently assigned to each driver");
    perfChart = new BarChart(c1);
    perfChart->setMinimumHeight(200);
    qobject_cast<QVBoxLayout*>(c1->layout())->addWidget(perfChart);

    QFrame* c2 = Theme::sectionCard(this, "Summary", "Current availability");
    QVBoxLayout* c2l = qobject_cast<QVBoxLayout*>(c2->layout());
    c2l->setSpacing(12);

    auto makeMetric = [&](const QString& label, const QString& dotColor) -> QLabel* {
        QWidget* row = new QWidget(c2);
        QHBoxLayout* rl = new QHBoxLayout(row);
        rl->setContentsMargins(0, 0, 0, 0);
        QLabel* dot = new QLabel("●", row);
        dot->setStyleSheet(QString("color:%1;font-size:14px;").arg(dotColor));
        QLabel* lbl = new QLabel(label, row);
        lbl->setStyleSheet("color:#334155;font-weight:600;font-size:13px;");
        QLabel* val = new QLabel("0", row);
        val->setStyleSheet("color:#0f172a;font-weight:800;font-size:14px;");
        rl->addWidget(dot);
        rl->addWidget(lbl);
        rl->addStretch();
        rl->addWidget(val);
        c2l->addWidget(row);
        return val;
    };
    totVal      = makeMetric("Total drivers", "#2563eb");
    actVal      = makeMetric("Available", "#16a34a");
    assignedVal = makeMetric("Assigned", "#7c3aed");
    busyVal     = makeMetric("Unavailable", "#64748b");
    c2l->addStretch();

    botRow->addWidget(c1, 3);
    botRow->addWidget(c2, 2);
    root->addLayout(botRow);
    root->addStretch();

    connect(addBtn, &QPushButton::clicked, this, &DriversPage::add);
    connect(editBtn, &QPushButton::clicked, this, &DriversPage::editSelected);
    connect(toggleBtn, &QPushButton::clicked, this, &DriversPage::toggleAvail);
    connect(delBtn, &QPushButton::clicked, this, &DriversPage::removeSelected);
    connect(searchEdit, &QLineEdit::textChanged, this, &DriversPage::refresh);
    connect(statusFilter, &QComboBox::currentTextChanged, this, &DriversPage::refresh);
    connect(table, &QTableWidget::itemSelectionChanged, this, [this]{
        int id = selectedDriverId();
        Driver* d = id > 0 ? system->getDriver(id) : nullptr;
        if (d && d->getAvailability() == DriverAvailability::Unavailable) toggleBtn->setText("Mark Available");
        else toggleBtn->setText("Mark Unavailable");
    });
    connect(table, &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem* it){
        if (!it) return;
        viewAssignment(table->item(it->row(), 0)->text().remove('#').toInt());
    });
    refresh();
}

int DriversPage::selectedDriverId() const {
    auto rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty()) return -1;
    return table->item(rows[0].row(), 0)->text().remove('#').toInt();
}

void DriversPage::viewAssignment(int driverId) {
    Driver* d = system->getDriver(driverId);
    if (!d) return;
    QDialog dlg(this);
    dlg.setWindowTitle(QString("Driver #%1").arg(driverId));
    dlg.setMinimumSize(480, 380);
    dlg.setStyleSheet("QDialog { background-color: #ffffff; } QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }");
    QVBoxLayout* r2 = new QVBoxLayout(&dlg);
    r2->setContentsMargins(24, 20, 24, 20);
    r2->setSpacing(12);

    QHBoxLayout* th = new QHBoxLayout();
    QLabel* head = new QLabel(QString::fromStdString(d->getName()), &dlg);
    QFont hf = head->font(); hf.setPointSize(16); hf.setBold(true);
    head->setFont(hf);
    head->setStyleSheet("color:#0f172a;");
    QString availText = QString::fromStdString(Driver::availabilityLabel(d->getAvailability()));
    QLabel* st = new QLabel(availText, &dlg);
    st->setStyleSheet(QString("font-weight:700;font-size:11px;padding:4px 10px;border-radius:6px;color:%1;background:%2;")
        .arg(Theme::pillForeground(availText), Theme::pillBackground(availText)));
    th->addWidget(head);
    th->addStretch();
    th->addWidget(st);
    r2->addLayout(th);

    QLabel* sub = new QLabel(QString("Vehicle: %1   |   Driver ID: #%2")
        .arg(QString::fromStdString(d->getVehicle())).arg(driverId), &dlg);
    sub->setStyleSheet("color:#64748b;font-size:13px;");
    r2->addWidget(sub);

    QLabel* listTitle = new QLabel("Orders", &dlg);
    listTitle->setStyleSheet("color:#0f172a;font-weight:700;font-size:13px;margin-top:6px;");
    r2->addWidget(listTitle);

    QListWidget* jobs = new QListWidget(&dlg);
    jobs->setStyleSheet(
        "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;font-size:12px;color:#0f172a;}"
        "QListWidget::item{padding:8px 12px;border-bottom:1px solid #f1f5f9;}");
    int n = 0;
    for (auto jt = system->getAllDeliveries().begin(); jt != system->getAllDeliveries().end(); ++jt) {
        Delivery* del = *jt;
        if (del->getDriverId() != driverId) continue;
        auto* jit = new QListWidgetItem(QString("#%1   %2 → %3   ·   %4")
            .arg(del->getId())
            .arg(QString::fromStdString(del->getPickupLocation()))
            .arg(QString::fromStdString(del->getDestination()))
            .arg(QString::fromStdString(del->getStatusString())));
        jit->setForeground(QBrush(QColor("#0f172a")));
        jobs->addItem(jit);
        n++;
    }
    if (n == 0) {
        auto* jit = new QListWidgetItem("No orders assigned.");
        jit->setForeground(QBrush(QColor("#64748b")));
        jobs->addItem(jit);
    }
    r2->addWidget(jobs);

    QPushButton* c = new QPushButton("Close", &dlg);
    Theme::styleDarkBtn(c);
    r2->addWidget(c, 0, Qt::AlignRight);
    connect(c, &QPushButton::clicked, &dlg, &QDialog::accept);
    dlg.exec();
}

void DriversPage::add() {
    QDialog d(this);
    d.setWindowTitle("Add Driver");
    d.setMinimumWidth(440);
    d.setStyleSheet("QDialog { background-color: #ffffff; } QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }");
    QVBoxLayout* dl = new QVBoxLayout(&d);
    dl->setContentsMargins(24, 20, 24, 20);
    dl->setSpacing(12);

    QLabel* title = new QLabel("Add Driver", &d);
    QFont tf = title->font(); tf.setPointSize(15); tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet("color:#0f172a;");
    dl->addWidget(title);

    QFormLayout* f = new QFormLayout();
    f->setSpacing(10);
    QLineEdit* n  = new QLineEdit(&d); n->setPlaceholderText("Full name");
    QLineEdit* v  = new QLineEdit(&d); v->setPlaceholderText("Vehicle number, e.g. MH12 AB 1234");
    QLineEdit* un = new QLineEdit(&d); un->setPlaceholderText("Optional: lets the driver sign in");
    QLineEdit* pw = new QLineEdit(&d); pw->setPlaceholderText("Required if a username is set"); pw->setEchoMode(QLineEdit::Password);
    for (auto w : {n, v, un, pw}) Theme::styleInput(w);
    f->addRow("Name",     n);
    f->addRow("Vehicle",  v);
    f->addRow("Username", un);
    f->addRow("Password", pw);
    dl->addLayout(f);

    QLabel* m = new QLabel("", &d); m->setWordWrap(true); m->setVisible(false);
    dl->addWidget(m);

    QPushButton* ok = new QPushButton("Add Driver", &d);
    ok->setMinimumHeight(40);
    Theme::stylePrimaryBtn(ok, Theme::SUCCESS);
    dl->addWidget(ok);

    connect(ok, &QPushButton::clicked, &d, [&]{
        QString usu = un->text().trimmed();
        bool done = usu.isEmpty()
            ? system->addDriver(n->text().toStdString(), v->text().toStdString(), 0)
            : system->registerDriverUser(n->text().toStdString(), v->text().toStdString(),
                                         usu.toStdString(), pw->text().toStdString());
        if (done) { d.accept(); return; }
        m->setText(QString::fromStdString(system->lastError()));
        m->setStyleSheet("color:#dc2626;font-size:12px;font-weight:600;background:#fef2f2;border:1px solid #fecaca;border-radius:6px;padding:6px;");
        m->setVisible(true);
    });
    if (d.exec() == QDialog::Accepted) { refresh(); Theme::toast(this, "Driver added.", true); }
}

void DriversPage::editSelected() {
    int id = selectedDriverId();
    if (id < 0) { Theme::toast(this, "Select a driver first.", false); return; }
    Driver* dr = system->getDriver(id);
    if (!dr) return;

    QDialog d(this);
    d.setWindowTitle(QString("Edit Driver #%1").arg(id));
    d.setMinimumWidth(440);
    d.setStyleSheet("QDialog { background-color: #ffffff; } QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }");
    QVBoxLayout* dl = new QVBoxLayout(&d);
    dl->setContentsMargins(24, 20, 24, 20);
    dl->setSpacing(12);
    QFormLayout* f = new QFormLayout();
    QLineEdit* n = new QLineEdit(QString::fromStdString(dr->getName()), &d);
    QLineEdit* v = new QLineEdit(QString::fromStdString(dr->getVehicle()), &d);
    Theme::styleInput(n);
    Theme::styleInput(v);
    f->addRow("Name", n);
    f->addRow("Vehicle", v);
    dl->addLayout(f);
    QLabel* m = new QLabel("", &d);
    m->setWordWrap(true);
    m->setVisible(false);
    dl->addWidget(m);
    QPushButton* ok = new QPushButton("Save Changes", &d);
    ok->setMinimumHeight(40);
    Theme::stylePrimaryBtn(ok);
    dl->addWidget(ok);
    connect(ok, &QPushButton::clicked, &d, [&]{
        if (system->updateDriver(id, n->text().toStdString(), v->text().toStdString())) { d.accept(); return; }
        m->setText(QString::fromStdString(system->lastError()));
        m->setStyleSheet("color:#dc2626;font-size:12px;font-weight:600;background:#fef2f2;border:1px solid #fecaca;border-radius:6px;padding:6px;");
        m->setVisible(true);
    });
    if (d.exec() == QDialog::Accepted) { refresh(); Theme::toast(this, "Driver updated.", true); }
}

void DriversPage::toggleAvail() {
    int id = selectedDriverId();
    if (id < 0) { Theme::toast(this, "Select a driver first.", false); return; }
    Driver* dr = system->getDriver(id);
    if (!dr) return;
    bool makeAvailable = dr->getAvailability() == DriverAvailability::Unavailable;
    if (!system->setDriverAvailable(id, makeAvailable)) {
        Theme::toast(this, QString::fromStdString(system->lastError()), false);
        return;
    }
    refresh();
    Theme::toast(this, makeAvailable ? "Driver is now available." : "Driver is now unavailable.", true);
}

void DriversPage::removeSelected() {
    int id = selectedDriverId();
    if (id < 0) { Theme::toast(this, "Select a driver first.", false); return; }
    if (QMessageBox::question(this, "Remove Driver",
                              QString("Remove driver #%1 and their login account?").arg(id),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    if (system->removeDriver(id)) { refresh(); Theme::toast(this, "Driver removed.", true); }
    else Theme::toast(this, QString::fromStdString(system->lastError()), false);
}

void DriversPage::refresh() {
    if (!system) return;
    int selected = selectedDriverId();
    QString q = searchEdit ? searchEdit->text().trimmed().toLower() : "";
    QString sf = statusFilter ? statusFilter->currentText() : "All Drivers";
    table->setRowCount(0);

    int total = system->getTotalDriverCount();
    int available = 0, assigned = 0, unavailable = 0;
    for (auto it = system->getAllDrivers().begin(); it != system->getAllDrivers().end(); ++it) {
        switch ((*it)->getAvailability()) {
            case DriverAvailability::Available:   available++; break;
            case DriverAvailability::Assigned:    assigned++; break;
            case DriverAvailability::Unavailable: unavailable++; break;
        }
    }
    if (totVal)      totVal->setText(QString::number(total));
    if (actVal)      actVal->setText(QString::number(available));
    if (assignedVal) assignedVal->setText(QString::number(assigned));
    if (busyVal)     busyVal->setText(QString::number(unavailable));

    std::vector<BarChart::Bar> bars;
    for (auto it = system->getAllDrivers().begin(); it != system->getAllDrivers().end(); ++it) {
        Driver* d = *it;
        int active = 0;
        for (auto jt = system->getAllDeliveries().begin(); jt != system->getAllDeliveries().end(); ++jt) {
            Status s = (*jt)->getStatus();
            if ((*jt)->getDriverId() == d->getId() &&
                (s == Status::Assigned || s == Status::PickedUp || s == Status::InTransit)) active++;
        }
        bars.push_back({QString::fromStdString(d->getName()).split(' ').first(), active, QColor("#2563eb")});
    }
    perfChart->setData(bars);

    for (auto it = system->getAllDrivers().begin(); it != system->getAllDrivers().end(); ++it) {
        Driver* d = *it;
        QString avail = QString::fromStdString(Driver::availabilityLabel(d->getAvailability()));
        QString hay = QString("%1 %2 %3").arg(d->getId())
            .arg(QString::fromStdString(d->getName())).arg(QString::fromStdString(d->getVehicle())).toLower();
        if (!q.isEmpty() && !hay.contains(q)) continue;
        if (sf != "All Drivers" && avail != sf) continue;

        int r = table->rowCount();
        table->insertRow(r);
        auto* idItem = new QTableWidgetItem(QString("#%1").arg(d->getId()));
        idItem->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 0, idItem);

        auto* nameItem = new QTableWidgetItem(QString::fromStdString(d->getName()));
        nameItem->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 1, nameItem);

        auto* vehItem = new QTableWidgetItem(QString::fromStdString(d->getVehicle()));
        vehItem->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 2, vehItem);

        auto* stItem = new QTableWidgetItem(avail);
        Theme::setPill(stItem, avail);
        table->setItem(r, 3, stItem);

        auto* jobItem = new QTableWidgetItem(activeOrderList(system, d->getId()));
        jobItem->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 4, jobItem);

        if (d->getId() == selected) table->selectRow(r);
    }
}
