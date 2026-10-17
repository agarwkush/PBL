#include "SearchPage.h"
#include "DeliveryDetails.h"
#include "../core/LogisticsSystem.h"
#include "../logic/Sorting.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QFrame>
#include <algorithm>
#include <vector>

SearchPage::SearchPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(12);
    root->addWidget(Theme::pageHeader(this, "Search",
        "Find orders by ID or by any word in their locations or status."));

    QFrame* tools = Theme::sectionCard(this);
    QHBoxLayout* row = new QHBoxLayout();
    row->setSpacing(8);
    idEdit   = new QLineEdit(this);
    idEdit->setPlaceholderText("Order ID");
    textEdit = new QLineEdit(this);
    textEdit->setPlaceholderText("Keyword, e.g. Katraj, Baner or Delivered");
    textEdit->setClearButtonEnabled(true);
    modeBox  = new QComboBox(this);
    modeBox->addItems({"Linear search", "Binary search (by ID)"});
    for (auto w : {(QWidget*)idEdit, (QWidget*)textEdit, (QWidget*)modeBox}) Theme::styleInput(w);
    QPushButton* btn    = new QPushButton("Search", this);
    Theme::stylePrimaryBtn(btn);
    QPushButton* sortId = new QPushButton("Sort by ID", this);
    QPushButton* sortPr = new QPushButton("Sort by Priority", this);
    Theme::styleGhostBtn(sortId);
    Theme::styleGhostBtn(sortPr);
    row->addWidget(idEdit);
    row->addWidget(textEdit, 2);
    row->addWidget(modeBox, 1);
    row->addWidget(btn);
    row->addWidget(sortId);
    row->addWidget(sortPr);
    qobject_cast<QVBoxLayout*>(tools->layout())->addLayout(row);

    out = new QLabel("Enter an order ID or a keyword to search.", tools);
    out->setWordWrap(true);
    out->setStyleSheet("color:#475569;font-size:12px;background:#f8fafc;border:1px solid #e2e8f0;border-radius:6px;padding:8px 12px;");
    qobject_cast<QVBoxLayout*>(tools->layout())->addWidget(out);
    root->addWidget(tools);

    QFrame* list = Theme::sectionCard(this, "Results", "Double-click a result to see the order details.");
    table = new QTableWidget(list);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"ID","Pickup","Destination","Priority","Status"});
    Theme::styleCardTable(table);
    table->setMinimumHeight(300);
    qobject_cast<QVBoxLayout*>(list->layout())->addWidget(table);
    root->addWidget(list, 1);

    connect(btn,     &QPushButton::clicked,      this, &SearchPage::find);
    connect(idEdit,  &QLineEdit::returnPressed,  this, &SearchPage::find);
    connect(textEdit,&QLineEdit::returnPressed,  this, &SearchPage::find);
    connect(table,   &QTableWidget::itemDoubleClicked, this, [this](QTableWidgetItem* it){
        if (!it) return;
        int id = table->item(it->row(), 0)->text().remove('#').toInt();
        DeliveryDetails dlg(system, id, this);
        dlg.exec();
    });
    connect(sortId, &QPushButton::clicked, this, [this]{
        system->sortDeliveriesByID();
        out->setText("Orders sorted by ID.");
        find();
    });
    connect(sortPr, &QPushButton::clicked, this, [this]{
        system->sortDeliveriesByPriority();
        out->setText("Orders sorted by priority, urgent first.");
        find();
    });
}

void SearchPage::setQuery(const QString& q) {
    idEdit->clear();
    textEdit->setText(q);
    find();
}

void SearchPage::find() {
    table->setRowCount(0);
    QString q  = textEdit->text().trimmed().toLower();
    int id     = idEdit->text().trimmed().remove('#').toInt();
    std::vector<Delivery*> hits;

    User* u    = system->getAuthManager()->getCurrentUser();
    Role role  = u ? u->getRole() : Role::LogisticsTeam;
    auto visible = [&](Delivery* d){
        if (role == Role::Customer && u && d->getCustomerId() != u->getLinkedId()) return false;
        if (role == Role::DeliveryMan && u) {
            Driver* dd = system->getDriverByUserId(u->getId());
            if (!dd || d->getDriverId() != dd->getId()) return false;
        }
        return true;
    };

    if (id > 0 && q.isEmpty()) {
        Delivery* d = nullptr;
        if (modeBox->currentText().contains("Binary")) {
            std::vector<Delivery*> all;
            for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it)
                all.push_back(*it);
            Sorting<Delivery*>::sort(all.data(), (int)all.size(), [](Delivery* a, Delivery* b){ return a->getId() < b->getId(); });
            int lo = 0, hi = (int)all.size() - 1;
            while (lo <= hi) {
                int mid = lo + (hi - lo) / 2;
                if (all[mid]->getId() == id) { d = all[mid]; break; }
                else if (all[mid]->getId() < id) lo = mid + 1;
                else                             hi = mid - 1;
            }
            out->setText(d ? QString("Binary search: order #%1 found.").arg(id)
                           : QString("Binary search: order #%1 not found.").arg(id));
        } else {
            d = system->searchDelivery(id);
            out->setText(d ? QString("Order #%1: %2 → %3 (%4).").arg(id)
                    .arg(QString::fromStdString(d->getPickupLocation()))
                    .arg(QString::fromStdString(d->getDestination()))
                    .arg(QString::fromStdString(d->getStatusString()))
                : QString("Order #%1 not found.").arg(id));
        }
        if (d && visible(d)) hits.push_back(d);
    } else {
        for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it) {
            Delivery* d = *it;
            if (!visible(d)) continue;
            if (q.isEmpty()) { hits.push_back(d); continue; }
            QString hay = QString("%1 %2 %3 %4").arg(d->getId())
                .arg(QString::fromStdString(d->getPickupLocation()))
                .arg(QString::fromStdString(d->getDestination()))
                .arg(QString::fromStdString(d->getStatusString())).toLower();
            if (hay.contains(q)) hits.push_back(d);
        }
        out->setText(QString("%1 matching order(s).").arg(hits.size()));
    }
    for (auto* d : hits) {
        int r = table->rowCount(); table->insertRow(r);
        auto* it0 = new QTableWidgetItem(QString("#%1").arg(d->getId()));
        it0->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 0, it0);

        auto* it1 = new QTableWidgetItem(QString::fromStdString(d->getPickupLocation()));
        it1->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 1, it1);

        auto* it2 = new QTableWidgetItem(QString::fromStdString(d->getDestination()));
        it2->setForeground(QBrush(QColor("#0f172a")));
        table->setItem(r, 2, it2);

        auto* p = new QTableWidgetItem(QString::fromStdString(d->getPriorityString()));
        Theme::setPill(p, QString::fromStdString(d->getPriorityString()));
        table->setItem(r, 3, p);

        auto* s = new QTableWidgetItem(QString::fromStdString(d->getStatusString()));
        Theme::setPill(s, QString::fromStdString(d->getStatusString()));
        table->setItem(r, 4, s);
    }
}
