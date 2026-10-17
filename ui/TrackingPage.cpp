#include "TrackingPage.h"
#include "../core/LogisticsSystem.h"
#include "../core/GeoUtils.h"
#include "../core/User.h"
#include "Theme.h"
#include "Charts.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QPainter>

class MilestoneStepper : public QWidget {
public:
    explicit MilestoneStepper(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(64);
    }
    void setStep(int step) {
        m_step = qBound(0, step, 4);
        update();
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const char* labels[] = {"Pending", "Assigned", "Picked Up", "In Transit", "Delivered"};
        int n = 5;
        int pad = 48;
        int w = width() - pad * 2;
        int y = 20;

        p.setPen(QPen(QColor("#e2e8f0"), 3));
        p.drawLine(pad, y, pad + w, y);

        int activeW = (w * m_step) / (n - 1);
        p.setPen(QPen(QColor("#2563eb"), 3));
        p.drawLine(pad, y, pad + activeW, y);

        QFont lf = font(); lf.setPointSize(8); lf.setBold(false);
        p.setFont(lf);

        for (int i = 0; i < n; i++) {
            int x = pad + (w * i) / (n - 1);
            bool isDone = i <= m_step;
            bool isCurrent = i == m_step;

            p.setBrush(isDone ? QColor("#2563eb") : QColor("#ffffff"));
            p.setPen(QPen(isDone ? QColor("#2563eb") : QColor("#cbd5e1"), 2));
            p.drawEllipse(x - 8, y - 8, 16, 16);

            if (isDone) {
                p.setBrush(Qt::white); p.setPen(Qt::NoPen);
                p.drawEllipse(x - 3, y - 3, 6, 6);
            }

            p.setPen(isCurrent ? QColor("#0f172a") : QColor("#64748b"));
            QFont f = p.font(); f.setBold(isCurrent);
            p.setFont(f);
            QRect tr(x - 36, y + 12, 72, 28);
            p.drawText(tr, Qt::AlignHCenter | Qt::AlignTop, labels[i]);
            p.setFont(lf);
        }
    }
private:
    int m_step = 0;
};

TrackingPage::TrackingPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(Theme::SECTION_GAP);
    root->addWidget(Theme::pageHeader(this, "Tracking",
        "Select an order to see its status, details and route."));

    QHBoxLayout* cols = new QHBoxLayout();
    cols->setSpacing(14);

    QWidget* leftCol = new QWidget(this);
    QVBoxLayout* lLayout = new QVBoxLayout(leftCol);
    lLayout->setContentsMargins(0, 0, 0, 0);
    lLayout->setSpacing(12);

    QFrame* selCard = Theme::sectionCard(leftCol, "Select an Order");
    QVBoxLayout* scLayout = qobject_cast<QVBoxLayout*>(selCard->layout());
    QHBoxLayout* selRow = new QHBoxLayout();
    selRow->setSpacing(8);
    orderCombo = new QComboBox(selCard);
    Theme::styleInput(orderCombo);
    selRow->addWidget(orderCombo, 1);
    scLayout->addLayout(selRow);
    lLayout->addWidget(selCard);

    QFrame* detCard = Theme::sectionCard(leftCol, "Order Details");
    QVBoxLayout* dcLayout = qobject_cast<QVBoxLayout*>(detCard->layout());
    QFormLayout* form = new QFormLayout();
    form->setSpacing(10);
    form->setLabelAlignment(Qt::AlignRight);

    auto makeVal = [&]() -> QLabel* {
        QLabel* l = new QLabel("—", detCard);
        l->setStyleSheet("color:#0f172a;font-weight:600;font-size:13px;");
        return l;
    };
    custVal   = makeVal();
    pickVal   = makeVal();
    destVal   = makeVal();
    costVal   = makeVal();
    statVal   = makeVal();
    driverVal = makeVal();
    vehVal    = makeVal();
    distVal   = makeVal();

    auto addRow = [&](const QString& label, QLabel* valWidget) {
        QLabel* kl = new QLabel(label, detCard);
        kl->setStyleSheet("color:#64748b;font-weight:600;font-size:12px;");
        form->addRow(kl, valWidget);
    };

    addRow("Client Account", custVal);
    addRow("Pickup", pickVal);
    addRow("Destination", destVal);
    addRow("Cost", costVal);
    addRow("Status", statVal);
    addRow("Driver", driverVal);
    addRow("Vehicle", vehVal);
    addRow("Distance", distVal);
    dcLayout->addLayout(form);

    stepperWidget = new MilestoneStepper(detCard);
    dcLayout->addSpacing(8);
    QLabel* stepTitle = new QLabel("Progress", detCard);
    stepTitle->setStyleSheet("color:#0f172a;font-weight:700;font-size:12px;");
    dcLayout->addWidget(stepTitle);
    dcLayout->addWidget(stepperWidget);

    lLayout->addWidget(detCard);
    lLayout->addStretch();

    QFrame* rightCard = Theme::sectionCard(this, "Route", "From pickup to destination");
    QVBoxLayout* rcLayout = qobject_cast<QVBoxLayout*>(rightCard->layout());
    mapView = new RouteMapWidget(rightCard);
    mapView->setMinimumHeight(220);
    rcLayout->addWidget(mapView);

    cols->addWidget(leftCol, 4);
    cols->addWidget(rightCard, 6);
    root->addLayout(cols, 1);

    connect(orderCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &TrackingPage::onTrackClicked);
    refresh();
}

void TrackingPage::refresh() {
    if (!system) return;
    User* u = system->getAuthManager()->getCurrentUser();
    Role role = u ? u->getRole() : Role::LogisticsTeam;
    int currentId = orderCombo->currentData().toInt();
    orderCombo->blockSignals(true);
    orderCombo->clear();
    for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it) {
        Delivery* d = *it;
        if (role == Role::Customer && u && d->getCustomerId() != u->getLinkedId()) continue;
        Customer* c = system->getCustomer(d->getCustomerId());
        QString label = QString("Order #%1 — %2 (%3 → %4)")
            .arg(d->getId())
            .arg(c ? QString::fromStdString(c->getName()) : "Customer")
            .arg(QString::fromStdString(d->getPickupLocation()).split(',')[0].trimmed())
            .arg(QString::fromStdString(d->getDestination()).split(',')[0].trimmed());
        orderCombo->addItem(label, d->getId());
    }
    orderCombo->blockSignals(false);
    if (orderCombo->count() > 0) {
        int idx = orderCombo->findData(currentId);
        orderCombo->setCurrentIndex(idx >= 0 ? idx : 0);
        onTrackClicked();
    }
}

void TrackingPage::selectDelivery(int deliveryId) {
    int idx = orderCombo->findData(deliveryId);
    if (idx >= 0) {
        orderCombo->setCurrentIndex(idx);
        onTrackClicked();
    }
}

void TrackingPage::onTrackClicked() {
    if (!system || orderCombo->count() == 0) return;
    int id = orderCombo->currentData().toInt();
    Delivery* d = system->getDelivery(id);
    if (!d) return;

    Customer* c = system->getCustomer(d->getCustomerId());
    Driver*   dr = system->getDriver(d->getDriverId());

    custVal->setText(c ? QString("%1 (ID #%2)").arg(QString::fromStdString(c->getName())).arg(c->getId())
                       : QString("Client #%1").arg(d->getCustomerId()));
    pickVal->setText(QString::fromStdString(d->getPickupLocation()));
    destVal->setText(QString::fromStdString(d->getDestination()));
    costVal->setText(QString("₹%1 (%2)").arg(d->getCost(), 0, 'f', 2).arg(d->getPriority() == Priority::Urgent ? "Urgent" : "Normal"));
    statVal->setText(QString::fromStdString(d->getStatusString()));
    driverVal->setText(dr ? QString::fromStdString(dr->getName()) : "Not assigned");
    vehVal->setText(dr ? QString::fromStdString(dr->getVehicle()) : "—");

    double dist = GeoUtils::distanceKm(d->getPickupLocation(), d->getDestination());
    distVal->setText(QString("%1 km").arg(dist, 0, 'f', 2));

    int step = 0;
    double progress = 0.05;
    Status st = d->getStatus();
    if (st == Status::Pending) {
        step = 0; progress = 0.05;
    } else if (st == Status::Assigned) {
        step = 1; progress = 0.25;
    } else if (st == Status::PickedUp) {
        step = 2; progress = 0.45;
    } else if (st == Status::InTransit) {
        step = 3; progress = 0.65;
    } else if (st == Status::Delivered) {
        step = 4; progress = 1.0;
    } else {
        step = 0; progress = 0.0;
    }

    if (auto* ms = static_cast<MilestoneStepper*>(stepperWidget)) {
        ms->setStep(step);
    }

    mapView->setRoute(
        QString::fromStdString(d->getPickupLocation()),
        QString::fromStdString(d->getDestination()),
        QString::fromStdString(d->getStatusString()),
        progress
    );
}
