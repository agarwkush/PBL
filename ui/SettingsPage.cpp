#include "SettingsPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QPushButton>

SettingsPage::SettingsPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    User* cu = system->getAuthManager()->getCurrentUser();
    bool isAdmin = cu && cu->getRole() == Role::LogisticsTeam;

    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(12);
    root->addWidget(Theme::pageHeader(this, "Settings", "Batching options, saving and account information."));

    QFrame* c1 = Theme::sectionCard(this, "Batch Radius",
        "Pickups within this distance of each other can be grouped for one driver.");
    QHBoxLayout* row = new QHBoxLayout();
    row->setSpacing(8);
    radiusSpin = new QDoubleSpinBox(c1);
    radiusSpin->setRange(0.01, 20.0);
    radiusSpin->setSingleStep(0.10);
    radiusSpin->setDecimals(2);
    radiusSpin->setSuffix(" km");
    Theme::styleInput(radiusSpin);
    QPushButton* ap  = new QPushButton("Apply", c1);
    Theme::stylePrimaryBtn(ap);
    QPushButton* m10 = new QPushButton("Set to 10 m", c1);
    Theme::styleGhostBtn(m10);
    row->addWidget(radiusSpin, 1);
    row->addWidget(ap);
    row->addWidget(m10);
    qobject_cast<QVBoxLayout*>(c1->layout())->addLayout(row);
    root->addWidget(c1);
    c1->setVisible(isAdmin);
    connect(ap,  &QPushButton::clicked, this, &SettingsPage::applyRadius);
    connect(m10, &QPushButton::clicked, this, [this]{ radiusSpin->setValue(0.01); applyRadius(); });

    QFrame* c2 = Theme::sectionCard(this, "Data", "Orders, drivers and customers are saved automatically. Use this to save them now.");
    QVBoxLayout* l2 = qobject_cast<QVBoxLayout*>(c2->layout());
    info = new QLabel(c2);
    info->setWordWrap(true);
    info->setStyleSheet("color:#334155;font-size:13px;");
    QPushButton* save = new QPushButton("Save Data", c2);
    Theme::styleDarkBtn(save);
    l2->addWidget(info);
    l2->addWidget(save, 0, Qt::AlignLeft);
    root->addWidget(c2);
    connect(save, &QPushButton::clicked, this, [this]{
        system->saveData();
        Theme::toast(this, "Data saved.", true);
        refresh();
    });

    QFrame* about = Theme::sectionCard(this, "About", "LogiCore — Logistics and Delivery Management");
    QVBoxLayout* al = qobject_cast<QVBoxLayout*>(about->layout());
    QLabel* aboutText = new QLabel(
        "Version 1.0\n"
        "Works without an internet connection. Orders, drivers, customers and service areas are saved on this computer.\n"
        "Sign-in types: Admin (dispatch and management), Driver (assigned deliveries) and Customer (own orders).", about);
    aboutText->setWordWrap(true);
    aboutText->setStyleSheet("color:#334155;font-size:13px;");
    al->addWidget(aboutText);
    root->addWidget(about);

    root->addStretch();
    refresh();
}

void SettingsPage::refresh() {
    if (!system) return;
    if (radiusSpin) radiusSpin->setValue(system->getBatchRadiusKm());
    if (info) {
        User* u    = system->getAuthManager()->getCurrentUser();
        Role role  = u ? u->getRole() : Role::LogisticsTeam;
        QString scope;
        if (role == Role::Customer && u) {
            int mine = 0;
            for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it)
                if ((*it)->getCustomerId() == u->getLinkedId()) mine++;
            scope = QString("Your orders: %1").arg(mine);
        } else if (role == Role::DeliveryMan && u) {
            Driver* dd = system->getDriverByUserId(u->getId());
            int mine = 0;
            if (dd) for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it)
                if ((*it)->getDriverId() == dd->getId()) mine++;
            scope = QString("Your assigned orders: %1").arg(mine);
        } else {
            scope = QString("Orders: %1   •   Drivers: %2   •   Customers: %3")
                .arg(system->getTotalDeliveryCount()).arg(system->getTotalDriverCount())
                .arg(system->getTotalCustomerCount());
        }
        info->setText(QString("Signed in as %1 (%2)   •   %3   •   Batch radius: %4 km")
            .arg(u ? QString::fromStdString(u->getUsername()) : "Operator")
            .arg(u ? QString::fromStdString(LogisticsSystem::roleLabel(u->getRole())) : "Admin")
            .arg(scope)
            .arg(system->getBatchRadiusKm(), 0, 'f', 2));
    }
}

void SettingsPage::applyRadius() {
    system->setBatchRadiusKm(radiusSpin->value());
    system->saveData();
    Theme::toast(this, QString("Batch radius set to %1 km.").arg(radiusSpin->value(), 0, 'f', 2), true);
    refresh();
}
