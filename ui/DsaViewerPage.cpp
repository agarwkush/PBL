#include "DsaViewerPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QFrame>
#include <QScrollArea>
#include <algorithm>
#include <vector>

DsaViewerPage::DsaViewerPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    QScrollArea* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setStyleSheet("QScrollArea{background:#f1f5f9;border:none;}");

    QWidget* page = new QWidget(scroll);
    page->setStyleSheet("background:#f1f5f9;");
    scroll->setWidget(page);
    outer->addWidget(scroll);

    QVBoxLayout* root = new QVBoxLayout(page);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(12);
    root->addWidget(Theme::pageHeader(page, "Queues & Sorting",
        "Order queues, batch groups and sorting of the order list."));

    QFrame* c0 = Theme::sectionCard(page, "Summary");
    statsLabel = new QLabel(c0);
    statsLabel->setWordWrap(true);
    statsLabel->setStyleSheet("color:#1e293b;font-size:13px;");
    qobject_cast<QVBoxLayout*>(c0->layout())->addWidget(statsLabel);
    root->addWidget(c0);

    QHBoxLayout* cols = new QHBoxLayout();
    cols->setSpacing(12);
    auto mkCol = [&](const QString& t, const QString& s, QListWidget*& w){
        QFrame* c = Theme::sectionCard(page);
        QVBoxLayout* cl = qobject_cast<QVBoxLayout*>(c->layout());
        QLabel* title = new QLabel(t, c);
        QFont tf = title->font(); tf.setBold(true); tf.setPointSize(13);
        title->setFont(tf);
        title->setStyleSheet("color:#0f172a;");
        QLabel* sub = new QLabel(s, c);
        sub->setStyleSheet("color:#64748b;font-size:11px;");
        sub->setWordWrap(true);
        cl->addWidget(title);
        cl->addWidget(sub);
        w = new QListWidget(c);
        w->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        w->setTextElideMode(Qt::ElideRight);
        w->setStyleSheet(
            "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;font-size:12px;color:#0f172a;}"
            "QListWidget::item{padding:8px 10px;border-bottom:1px solid #f1f5f9;}"
            "QListWidget::item:selected{background:#dbeafe;color:#0f172a;}");
        w->setMinimumHeight(180);
        cl->addWidget(w);
        cols->addWidget(c, 1);
    };
    mkCol("Pending Queue", "Normal pending orders, oldest first", queueView);
    mkCol("Priority Queue", "High and urgent pending orders, highest priority first", urgentView);
    mkCol("Batch Groups", "Pending pickups located near each other", batchView);
    root->addLayout(cols);

    QFrame* c1 = Theme::sectionCard(page, "Sort Orders", "Sorts the order list used by the Orders page");
    QVBoxLayout* c1l = qobject_cast<QVBoxLayout*>(c1->layout());
    sequenceLabel = new QLabel(c1);
    sequenceLabel->setWordWrap(true);
    sequenceLabel->setStyleSheet("color:#475569;font-size:13px;background:#f8fafc;border:1px solid #e2e8f0;border-radius:6px;padding:8px 12px;");
    c1l->addWidget(sequenceLabel);
    QHBoxLayout* btns = new QHBoxLayout();
    btns->setSpacing(8);
    QPushButton* b1 = new QPushButton("Sort by ID", c1);
    QPushButton* b2 = new QPushButton("Sort by Priority", c1);
    Theme::styleDarkBtn(b1);
    Theme::styleDarkBtn(b2);
    btns->addWidget(b1);
    btns->addWidget(b2);
    btns->addStretch();
    c1l->addLayout(btns);
    root->addWidget(c1);
    root->addStretch();

    connect(b1, &QPushButton::clicked, this, [this]{
        system->sortDeliveriesByID();
        refresh();
        Theme::toast(this, "Orders sorted by ID.", true);
    });
    connect(b2, &QPushButton::clicked, this, [this]{
        system->sortDeliveriesByPriority();
        refresh();
        Theme::toast(this, "Orders sorted by priority.", true);
    });
    refresh();
}

void DsaViewerPage::refresh() {
    if (!system) return;

    statsLabel->setText(QString("Orders: %1    Pending: %2    Urgent pending: %3    Batch groups: %4")
        .arg(system->getTotalDeliveryCount())
        .arg(system->getPendingCount())
        .arg(system->getUrgentCount())
        .arg(system->getBatchCount()));

    auto fillList = [](QListWidget* list, const std::vector<Delivery*>& items, const QString& emptyText, auto formatter) {
        list->clear();
        if (items.empty()) {
            auto* it = new QListWidgetItem(emptyText);
            it->setForeground(QBrush(QColor("#64748b")));
            list->addItem(it);
            return;
        }
        for (Delivery* d : items) {
            auto* it = new QListWidgetItem(formatter(d));
            it->setForeground(QBrush(QColor("#0f172a")));
            list->addItem(it);
        }
    };

    std::vector<Delivery*> normal, urgent;
    for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it) {
        Delivery* d = *it;
        if (d->getStatus() != Status::Pending) continue;
        if (d->getPriority() != Priority::Normal) urgent.push_back(d);
        else normal.push_back(d);
    }
    auto byId = [](Delivery* a, Delivery* b){ return a->getId() < b->getId(); };
    std::sort(normal.begin(), normal.end(), byId);
    std::sort(urgent.begin(), urgent.end(), [](Delivery* a, Delivery* b){
        if (a->getPriorityValue() != b->getPriorityValue()) return a->getPriorityValue() > b->getPriorityValue();
        return a->getId() < b->getId();
    });

    fillList(queueView, normal, "No normal pending orders.", [](Delivery* d){
        return QString("#%1   %2").arg(d->getId()).arg(QString::fromStdString(d->getPickupLocation()));
    });
    fillList(urgentView, urgent, "No high or urgent pending orders.", [](Delivery* d){
        return QString("#%1   %2").arg(d->getId()).arg(QString::fromStdString(d->getPickupLocation()));
    });

    batchView->clear();
    auto groups = system->getBatchGroups();
    if (groups.empty()) {
        auto* it = new QListWidgetItem(QString("No pickups within %1 km of each other.").arg(system->getBatchRadiusKm(), 0, 'f', 2));
        it->setForeground(QBrush(QColor("#64748b")));
        batchView->addItem(it);
    }
    for (size_t i = 0; i < groups.size(); i++) {
        QStringList ids;
        for (int m : groups[i].memberIds) ids << QString("#%1").arg(m);
        Delivery* a = system->getDelivery(groups[i].anchorId);
        auto* it = new QListWidgetItem(QString("Group %1: %2   %3")
            .arg(i + 1).arg(ids.join(", "))
            .arg(a ? QString::fromStdString(a->getPickupLocation()) : ""));
        it->setForeground(QBrush(QColor("#0f172a")));
        batchView->addItem(it);
    }

    QStringList ids;
    int n = 0;
    for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end() && n < 8; ++it, ++n)
        ids << QString("#%1").arg((*it)->getId());
    sequenceLabel->setText(ids.isEmpty() ? "No orders yet." : QString("Current order: %1").arg(ids.join("  ·  ")));
}
