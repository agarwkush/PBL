#include "CustomersPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QFrame>

CustomersPage::CustomersPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(12);
    root->addWidget(Theme::pageHeader(this, "Customers",
        "Customer accounts and their orders. Customers can also create an account from the sign-in screen."));

    QFrame* tools = Theme::sectionCard(this);
    QHBoxLayout* t = new QHBoxLayout();
    t->setSpacing(8);
    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText("Search by name, phone or address");
    searchEdit->setClearButtonEnabled(true);
    Theme::styleInput(searchEdit);
    QPushButton* addBtn = new QPushButton("+ Add Customer", this);
    Theme::stylePrimaryBtn(addBtn, Theme::SUCCESS);
    QPushButton* delBtn = new QPushButton("Remove Customer", this);
    Theme::styleDangerBtn(delBtn);
    t->addWidget(searchEdit, 1);
    t->addWidget(addBtn);
    t->addWidget(delBtn);
    qobject_cast<QVBoxLayout*>(tools->layout())->addLayout(t);
    root->addWidget(tools);

    QFrame* list = Theme::sectionCard(this, "All Customers", "Number of orders each customer has placed.");
    table = new QTableWidget(list);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"ID","Name","Phone","Address","Orders"});
    Theme::styleCardTable(table);
    table->setMinimumHeight(320);
    qobject_cast<QVBoxLayout*>(list->layout())->addWidget(table);
    root->addWidget(list, 1);

    connect(addBtn, &QPushButton::clicked, this, &CustomersPage::add);
    connect(delBtn, &QPushButton::clicked, this, &CustomersPage::removeSelected);
    connect(searchEdit, &QLineEdit::textChanged, this, &CustomersPage::refresh);
    refresh();
}

void CustomersPage::add() {
    QDialog d(this);
    d.setWindowTitle("Add Customer");
    d.setFixedSize(440, 410);
    d.setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }"
    );
    QVBoxLayout* dl = new QVBoxLayout(&d);
    dl->setContentsMargins(24, 20, 24, 20);
    dl->setSpacing(12);

    QLabel* title = new QLabel("Add Customer", &d);
    QFont tf = title->font(); tf.setPointSize(15); tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet("color:#0f172a;");
    dl->addWidget(title);

    QFormLayout* f = new QFormLayout();
    f->setSpacing(10);
    QLineEdit* n  = new QLineEdit(&d); n->setPlaceholderText("Full name");
    QLineEdit* ph = new QLineEdit(&d); ph->setPlaceholderText("10-digit phone number");
    QLineEdit* ad = new QLineEdit(&d); ad->setPlaceholderText("Area or address");
    QLineEdit* un = new QLineEdit(&d); un->setPlaceholderText("Optional, for a login account");
    QLineEdit* pw = new QLineEdit(&d); pw->setPlaceholderText("Required if a username is set"); pw->setEchoMode(QLineEdit::Password);
    for (auto w : {n, ph, ad, un, pw}) Theme::styleInput(w);
    f->addRow("Name",     n);
    f->addRow("Phone",    ph);
    f->addRow("Address",  ad);
    f->addRow("Username", un);
    f->addRow("Password", pw);
    dl->addLayout(f);

    QLabel* m = new QLabel("", &d); m->setWordWrap(true); m->setVisible(false);
    dl->addWidget(m);

    QPushButton* ok = new QPushButton("Add Customer", &d);
    ok->setMinimumHeight(40);
    Theme::stylePrimaryBtn(ok, Theme::SUCCESS);
    dl->addWidget(ok);

    connect(ok, &QPushButton::clicked, &d, [&]{
        bool done = false;
        if (!un->text().trimmed().isEmpty())
            done = system->registerCustomerUser(n->text().toStdString(), ph->text().toStdString(),
                ad->text().toStdString(), un->text().trimmed().toStdString(), pw->text().toStdString());
        else
            done = system->addCustomer(n->text().toStdString(), ph->text().toStdString(), ad->text().toStdString());
        if (done) d.accept();
        else {
            m->setText("Could not add the customer. Check the name, the 10-digit phone number and the address, and make sure the username is free.");
            m->setStyleSheet("color:#dc2626;font-size:12px;font-weight:600;background:#fef2f2;border:1px solid #fecaca;border-radius:6px;padding:6px;");
            m->setVisible(true);
        }
    });
    if (d.exec() == QDialog::Accepted) { refresh(); Theme::toast(this, "Customer added.", true); }
}

void CustomersPage::removeSelected() {
    auto rows = table->selectionModel()->selectedRows();
    if (rows.isEmpty()) { Theme::toast(this, "Select a customer first.", false); return; }
    int id = table->item(rows[0].row(), 0)->text().remove('#').toInt();
    if (QMessageBox::question(this, "Remove Customer", QString("Remove customer #%1 and their login account?").arg(id),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) return;
    if (system->removeCustomer(id)) { refresh(); Theme::toast(this, "Customer removed.", true); }
    else Theme::toast(this, "A customer with existing orders cannot be removed.", false);
}

void CustomersPage::refresh() {
    if (!system) return;
    QString q = searchEdit ? searchEdit->text().trimmed().toLower() : "";
    table->setRowCount(0);
    for (auto it = system->getAllCustomers().begin(); it != system->getAllCustomers().end(); ++it) {
        Customer* c = *it;
        QString hay = QString("%1 %2 %3 %4").arg(c->getId())
            .arg(QString::fromStdString(c->getName()))
            .arg(QString::fromStdString(c->getPhone()))
            .arg(QString::fromStdString(c->getAddress())).toLower();
        if (!q.isEmpty() && !hay.contains(q)) continue;
        int orders = 0;
        for (auto jt = system->getAllDeliveries().begin(); jt != system->getAllDeliveries().end(); ++jt)
            if ((*jt)->getCustomerId() == c->getId()) orders++;
        int r = table->rowCount(); table->insertRow(r);
        auto* it0 = new QTableWidgetItem(QString("#%1").arg(c->getId()));
        it0->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 0, it0);

        auto* it1 = new QTableWidgetItem(QString::fromStdString(c->getName()));
        it1->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 1, it1);

        auto* it2 = new QTableWidgetItem(QString::fromStdString(c->getPhone()));
        it2->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 2, it2);

        auto* it3 = new QTableWidgetItem(QString::fromStdString(c->getAddress()));
        it3->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 3, it3);

        auto* o = new QTableWidgetItem(QString::number(orders));
        o->setForeground(QBrush(QColor("#0f172a")));
        o->setTextAlignment(Qt::AlignCenter);
        table->setItem(r, 4, o);
    }
}
