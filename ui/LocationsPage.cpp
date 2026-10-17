#include "LocationsPage.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QFrame>
#include <QCompleter>
#include <QLineEdit>

namespace {

const char* kHintIdle = "color:#475569;font-size:12px;background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;padding:10px 12px;";
const char* kHintOk   = "color:#166534;font-size:12px;font-weight:600;background:#f0fdf4;border:1px solid #bbf7d0;border-radius:8px;padding:10px 12px;";
const char* kHintBad  = "color:#991b1b;font-size:12px;font-weight:600;background:#fef2f2;border:1px solid #fecaca;border-radius:8px;padding:10px 12px;";

void makeSearchable(QComboBox* box, const QStringList& items) {
    box->setEditable(true);
    box->setInsertPolicy(QComboBox::NoInsert);
    box->addItems(items);
    box->setCurrentIndex(-1);
    box->lineEdit()->setPlaceholderText("Search area");
    QCompleter* completer = new QCompleter(items, box);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    box->setCompleter(completer);
    Theme::styleInput(box);
}

}

LocationsPage::LocationsPage(LogisticsSystem *sys, QWidget *parent)
    : QWidget(parent), system(sys)
{
    QVBoxLayout* root = new QVBoxLayout(this);
    root->setContentsMargins(Theme::PAGE_MARGIN, 20, Theme::PAGE_MARGIN, Theme::PAGE_MARGIN);
    root->setSpacing(Theme::SECTION_GAP);
    root->addWidget(Theme::pageHeader(this, "Service Areas",
        "Areas we pick up from and deliver to. Orders outside these areas cannot be created."));

    QHBoxLayout* cols = new QHBoxLayout();
    cols->setSpacing(14);

    QFrame* c1 = Theme::sectionCard(this, "Service Areas", "Double-click an area to check it");
    areaList = new QListWidget(c1);
    areaList->setMinimumHeight(380);
    areaList->setStyleSheet(
        "QListWidget{background:#f8fafc;border:1px solid #e2e8f0;border-radius:8px;font-size:13px;color:#0f172a;}"
        "QListWidget::item{padding:9px 12px;border-bottom:1px solid #f1f5f9;}"
        "QListWidget::item:selected{background:#dbeafe;color:#0f172a;}");
    qobject_cast<QVBoxLayout*>(c1->layout())->addWidget(areaList);

    QFrame* c2 = Theme::sectionCard(this, "Check a Location", "Search an area or type an address");
    QVBoxLayout* l2 = qobject_cast<QVBoxLayout*>(c2->layout());
    l2->setSpacing(10);
    checkBox = new QComboBox(c2);
    QHBoxLayout* checkRow = new QHBoxLayout();
    checkRow->setSpacing(8);
    QPushButton* cb = new QPushButton("Check", c2);
    Theme::stylePrimaryBtn(cb);
    checkRow->addWidget(checkBox, 1);
    checkRow->addWidget(cb);
    checkOut = new QLabel(c2);
    checkOut->setWordWrap(true);
    checkOut->setStyleSheet(kHintIdle);
    l2->addLayout(checkRow);
    l2->addWidget(checkOut);

    l2->addSpacing(10);
    QLabel* routeTitle = new QLabel("Route Preview", c2);
    routeTitle->setStyleSheet("font-weight:700;font-size:14px;color:#0f172a;");
    l2->addWidget(routeTitle);
    fromBox = new QComboBox(c2);
    toBox = new QComboBox(c2);
    QHBoxLayout* routeRow = new QHBoxLayout();
    routeRow->setSpacing(8);
    routeRow->addWidget(fromBox, 1);
    QLabel* arrow = new QLabel("→", c2);
    arrow->setStyleSheet("color:#64748b;font-size:16px;");
    routeRow->addWidget(arrow);
    routeRow->addWidget(toBox, 1);
    QPushButton* rb = new QPushButton("Show Route", c2);
    Theme::styleDarkBtn(rb);
    routeRow->addWidget(rb);
    l2->addLayout(routeRow);
    routeOut = new QLabel("Choose a pickup and a destination to see the route and distance.", c2);
    routeOut->setWordWrap(true);
    routeOut->setStyleSheet(kHintIdle);
    l2->addWidget(routeOut);
    l2->addStretch();

    cols->addWidget(c1, 1);
    cols->addWidget(c2, 1);
    root->addLayout(cols, 1);

    connect(cb, &QPushButton::clicked, this, &LocationsPage::checkArea);
    connect(checkBox->lineEdit(), &QLineEdit::returnPressed, this, &LocationsPage::checkArea);
    connect(rb, &QPushButton::clicked, this, &LocationsPage::previewRoute);
    connect(areaList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* it){
        checkBox->setEditText(it->text());
        checkArea();
    });
    refresh();
}

void LocationsPage::refresh() {
    QStringList names;
    for (const auto& area : system->serviceAreas()) names << QString::fromStdString(area);

    if (areaList) {
        areaList->clear();
        for (const QString& name : names) {
            auto* it = new QListWidgetItem(name);
            it->setForeground(QBrush(QColor("#0f172a")));
            areaList->addItem(it);
        }
    }
    if (checkBox && checkBox->count() == 0) makeSearchable(checkBox, names);
    if (fromBox && fromBox->count() == 0) makeSearchable(fromBox, names);
    if (toBox && toBox->count() == 0) makeSearchable(toBox, names);
}

void LocationsPage::checkArea() {
    QString q = checkBox->currentText().trimmed();
    if (q.isEmpty()) {
        checkOut->setText("Enter an area or address to check.");
        checkOut->setStyleSheet(kHintIdle);
        return;
    }
    LocationResult result = system->resolveLocation(q.toStdString());
    if (result.ok) {
        checkOut->setText(QString("Served. Orders can be picked up and delivered in %1.").arg(QString::fromStdString(result.area)));
        checkOut->setStyleSheet(kHintOk);
    } else {
        checkOut->setText(QString::fromStdString(result.error) + " Choose one of the service areas on the left.");
        checkOut->setStyleSheet(kHintBad);
    }
}

void LocationsPage::previewRoute() {
    QString a = fromBox->currentText().trimmed();
    QString b = toBox->currentText().trimmed();
    if (a.isEmpty() || b.isEmpty()) {
        routeOut->setText("Choose both a pickup and a destination.");
        routeOut->setStyleSheet(kHintIdle);
        return;
    }
    RouteResult route = system->routeBetween(a.toStdString(), b.toStdString());
    if (!route.ok) {
        routeOut->setText(QString::fromStdString(route.error));
        routeOut->setStyleSheet(kHintBad);
        return;
    }
    QStringList path;
    for (const auto& area : route.areas) path << QString::fromStdString(area);
    double r = system->getBatchRadiusKm();
    QString batching = route.distanceKm <= r
        ? QString("Within the batch radius of %1 km, so these pickups can share one driver.").arg(r, 0, 'f', 2)
        : QString("Outside the batch radius of %1 km.").arg(r, 0, 'f', 2);
    routeOut->setText(QString("Route: %1\nDistance: %2 km     Source: local service network (offline)\n%3")
        .arg(path.join(" → "))
        .arg(route.distanceKm, 0, 'f', 2)
        .arg(batching));
    routeOut->setStyleSheet(kHintOk);
}
