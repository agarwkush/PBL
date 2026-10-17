#include "MainWindow.h"
#include "DashboardPage.h"
#include "DeliveriesPage.h"
#include "TrackingPage.h"
#include "DriversPage.h"
#include "CustomersPage.h"
#include "AssignmentsPage.h"
#include "ReportsPage.h"
#include "SearchPage.h"
#include "DsaViewerPage.h"
#include "SettingsPage.h"
#include "LocationsPage.h"
#include "Theme.h"
#include "../core/LogisticsSystem.h"
#include <QWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QFrame>
#include <QFont>
#include <QStatusBar>
#include <QApplication>
#include <functional>
#include <vector>

static QString roleName(Role r) {
    if (r == Role::LogisticsTeam) return "ADMIN";
    if (r == Role::DeliveryMan)   return "DRIVER";
    return "CUSTOMER";
}

MainWindow::MainWindow(LogisticsSystem *sys, QWidget *parent)
    : QMainWindow(parent), system(sys)
{
    setWindowTitle("LogiCore");
    resize(1280, 800);

    User* u = system->getAuthManager()->getCurrentUser();
    Role role = u ? u->getRole() : Role::LogisticsTeam;
    QString uname = u ? QString::fromStdString(u->getUsername()) : "operator";
    QString displayName = uname;
    if (role == Role::DeliveryMan) {
        if (Driver* d = system->getDriverByUserId(u->getId()))
            displayName = QString::fromStdString(d->getName());
    } else if (role == Role::Customer) {
        if (Customer* c = system->getCustomer(u->getLinkedId()))
            displayName = QString::fromStdString(c->getName());
    }

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QHBoxLayout *main = new QHBoxLayout(central);
    main->setSpacing(0);
    main->setContentsMargins(0, 0, 0, 0);

    QWidget *side = new QWidget(this);
    side->setFixedWidth(240);
    side->setStyleSheet("background:#0f172a;");
    QVBoxLayout *menu = new QVBoxLayout(side);
    menu->setContentsMargins(16, 24, 16, 16);
    menu->setSpacing(4);

    QLabel *logo = new QLabel("LogiCore", side);
    QFont lf = logo->font(); lf.setPointSize(20); lf.setBold(true);
    logo->setFont(lf);
    logo->setStyleSheet("color:white;");
    QLabel *sub = new QLabel("LOGISTICS", side);
    sub->setStyleSheet("color:#64748b;font-size:10px;font-weight:700;letter-spacing:2px;");
    menu->addWidget(logo);
    menu->addWidget(sub);
    menu->addSpacing(20);

    struct Nav { QString t; QString section; std::function<void()> go; };
    std::vector<Nav> items;
    if (role == Role::LogisticsTeam) {
        items = {
            {"Dashboard",     "Overview",   [this]{ openDashboard();  }},
            {"Orders",        "",           [this]{ openDeliveries(); }},
            {"Tracking",      "",           [this]{ openTracking();   }},
            {"Assignments",   "Operate",    [this]{ openAssign();     }},
            {"Drivers",       "",           [this]{ openDrivers();    }},
            {"Customers",     "",           [this]{ openCustomers();  }},
            {"Service Areas", "",           [this]{ openLocations();  }},
            {"Search",        "Insight",    [this]{ openSearch();     }},
            {"Queues & Sorting", "",        [this]{ openDsa();        }},
            {"Reports",       "",           [this]{ openReports();    }},
            {"Settings",      "",           [this]{ openSettings();   }},
        };
    } else if (role == Role::DeliveryMan) {
        items = {
            {"Dashboard",     "Overview",   [this]{ openDashboard();  }},
            {"My Deliveries", "",           [this]{ openDeliveries(); }},
            {"Tracking",      "",           [this]{ openTracking();   }},
            {"Search",        "Insight",    [this]{ openSearch();     }},
            {"Settings",      "",           [this]{ openSettings();   }},
        };
    } else {
        items = {
            {"Dashboard",     "Overview",   [this]{ openDashboard();  }},
            {"My Orders",     "",           [this]{ openDeliveries(); }},
            {"Tracking",      "",           [this]{ openTracking();   }},
            {"Search",        "Insight",    [this]{ openSearch();     }},
            {"Settings",      "",           [this]{ openSettings();   }},
        };
    }

    stack = new QStackedWidget(this);
    stack->setStyleSheet("background:#f1f5f9;");
    dashboard  = new DashboardPage(system, this);
    deliveries = new DeliveriesPage(system, this);
    tracking   = new TrackingPage(system, this);
    assign     = new AssignmentsPage(system, this);
    drivers    = new DriversPage(system, this);
    customers  = new CustomersPage(system, this);
    locations  = new LocationsPage(system, this);
    search     = new SearchPage(system, this);
    dsa        = new DsaViewerPage(system, this);
    reports    = new ReportsPage(system, this);
    settings   = new SettingsPage(system, this);

    stack->addWidget(dashboard);
    stack->addWidget(deliveries);
    stack->addWidget(tracking);
    stack->addWidget(assign);
    stack->addWidget(drivers);
    stack->addWidget(customers);
    stack->addWidget(locations);
    stack->addWidget(search);
    stack->addWidget(dsa);
    stack->addWidget(reports);
    stack->addWidget(settings);

    QString lastSection;
    for (size_t i = 0; i < items.size(); i++) {
        if (!items[i].section.isEmpty() && items[i].section != lastSection) {
            lastSection = items[i].section;
            QLabel* s = new QLabel(items[i].section.toUpper(), side);
            s->setStyleSheet("color:#475569;font-size:10px;font-weight:700;letter-spacing:1.5px;padding:12px 14px 4px 14px;");
            menu->addWidget(s);
        }
        QPushButton* b = new QPushButton(side);
        b->setText(QString(items[i].t).replace("&", "&&"));
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(Theme::navStyle(false));
        menu->addWidget(b);
        navBtns << b;
        navTitles << items[i].t;
        auto go  = items[i].go;
        int  idx = (int)i;
        connect(b, &QPushButton::clicked, this, [this, go, idx]{
            go();
            selectNav(idx);
        });
    }
    menu->addStretch();

    QWidget* userCard = new QWidget(side);
    userCard->setStyleSheet("background:#1e293b;border-radius:12px;");
    QHBoxLayout* uc = new QHBoxLayout(userCard);
    uc->setContentsMargins(12, 10, 12, 10);
    uc->addWidget(Theme::avatar(userCard, displayName, 36));
    QVBoxLayout* ui = new QVBoxLayout();
    ui->setSpacing(0);
    QLabel* un = new QLabel(displayName, userCard);
    un->setStyleSheet("color:white;font-weight:700;font-size:13px;");
    QLabel* rl = new QLabel(roleName(role), userCard);
    rl->setStyleSheet("color:#94a3b8;font-size:10px;font-weight:700;letter-spacing:1px;");
    ui->addWidget(un);
    ui->addWidget(rl);
    uc->addLayout(ui, 1);
    QPushButton* outBtn = new QPushButton("Sign out", userCard);
    outBtn->setCursor(Qt::PointingHandCursor);
    outBtn->setStyleSheet(
        "QPushButton{background:transparent;color:#94a3b8;font-weight:700;font-size:11px;"
        "padding:6px 10px;border:1px solid #334155;border-radius:6px;}"
        "QPushButton:hover{color:white;border-color:#64748b;}");
    uc->addWidget(outBtn);
    menu->addWidget(userCard);
    connect(outBtn, &QPushButton::clicked, this, &MainWindow::doLogout);

    QWidget* right = new QWidget(this);
    QVBoxLayout* rv = new QVBoxLayout(right);
    rv->setContentsMargins(0, 0, 0, 0);
    rv->setSpacing(0);

    QWidget* top = new QWidget(right);
    top->setStyleSheet("background:white;border-bottom:1px solid #e2e8f0;");
    QHBoxLayout* tv = new QHBoxLayout(top);
    tv->setContentsMargins(24, 12, 24, 12);
    tv->setSpacing(12);

    globalSearch = new QLineEdit(top);
    globalSearch->setPlaceholderText("Search orders by ID, location or status");
    globalSearch->setMinimumWidth(320);
    globalSearch->setMaximumWidth(420);
    Theme::styleInput(globalSearch);

    crumb = new QLabel("LogiCore  ›  Dashboard", top);
    crumb->setStyleSheet("color:#64748b;font-size:13px;font-weight:600;margin-left:8px;");

    QPushButton* refBtn = new QPushButton("Refresh", top);
    Theme::styleGhostBtn(refBtn);

    QFrame* profilePill = new QFrame(top);
    profilePill->setStyleSheet("QFrame{background:#f8fafc;border:1px solid #e2e8f0;border-radius:18px;}");
    QHBoxLayout* ph = new QHBoxLayout(profilePill);
    ph->setContentsMargins(6, 4, 10, 4);
    ph->setSpacing(8);
    ph->addWidget(Theme::avatar(profilePill, displayName, 28));
    QLabel* uNameLabel = new QLabel(displayName, profilePill);
    uNameLabel->setStyleSheet("color:#0f172a;font-weight:700;font-size:12px;");
    QLabel* uRoleBadge = new QLabel(roleName(role), profilePill);
    uRoleBadge->setStyleSheet("color:#1e40af;background:#dbeafe;font-size:10px;font-weight:800;padding:2px 6px;border-radius:6px;letter-spacing:0.5px;");
    ph->addWidget(uNameLabel);
    ph->addWidget(uRoleBadge);

    tv->addWidget(globalSearch);
    tv->addWidget(crumb);
    tv->addStretch();
    tv->addWidget(refBtn);
    tv->addWidget(profilePill);
    rv->addWidget(top);
    rv->addWidget(stack, 1);

    main->addWidget(side);
    main->addWidget(right, 1);

    statusInfo = new QLabel(this);
    statusInfo->setStyleSheet("color:#475569;font-size:12px;padding-left:12px;");
    statusBar()->addWidget(statusInfo, 1);

    connect(refBtn, &QPushButton::clicked, this, &MainWindow::refreshAll);
    connect(globalSearch, &QLineEdit::returnPressed, this, [this]{
        openSearch();
        search->setQuery(globalSearch->text().trimmed());
        selectNav(navTitles.indexOf("Search"));
    });

    refreshAll();
    stack->setCurrentWidget(dashboard);
    selectNav(0);
}

void MainWindow::selectNav(int i) {
    for (int k = 0; k < navBtns.size(); k++)
        navBtns[k]->setStyleSheet(Theme::navStyle(k == i));
    if (crumb && i >= 0 && i < navTitles.size()) {
        crumb->setText("LogiCore  ›  " + navTitles[i]);
    }
}

void MainWindow::refreshAll() {
    if (dashboard)  dashboard->refresh();
    if (deliveries) deliveries->refresh();
    if (tracking)   tracking->refresh();
    if (drivers)    drivers->refresh();
    if (customers)  customers->refresh();
    if (assign)     assign->refresh();
    if (reports)    reports->refresh();
    if (dsa)        dsa->refresh();
    if (settings)   settings->refresh();
    if (locations)  locations->refresh();
    if (statusInfo) {
        User* u = system->getAuthManager()->getCurrentUser();
        Role role = u ? u->getRole() : Role::LogisticsTeam;
        if (role == Role::LogisticsTeam) {
            statusInfo->setText(QString("Pending: %1    Urgent: %2    Batch groups: %3    Available drivers: %4")
                .arg(system->getPendingCount()).arg(system->getUrgentCount())
                .arg(system->getBatchCount())
                .arg(system->getAvailableDriverCount()));
        } else if (role == Role::DeliveryMan && u) {
            Driver* dd = system->getDriverByUserId(u->getId());
            int mine = 0, active = 0;
            if (dd) for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it)
                if ((*it)->getDriverId() == dd->getId()) {
                    mine++;
                    if ((*it)->getStatus() == Status::Assigned || (*it)->getStatus() == Status::InTransit) active++;
                }
            statusInfo->setText(QString("My orders: %1    Active: %2    Pending in city: %3")
                .arg(mine).arg(active).arg(system->getPendingCount()));
        } else if (u) {
            int mine = 0;
            for (auto it = system->getAllDeliveries().begin(); it != system->getAllDeliveries().end(); ++it)
                if ((*it)->getCustomerId() == u->getLinkedId()) mine++;
            statusInfo->setText(QString("My orders: %1").arg(mine));
        }
    }
}

void MainWindow::openDashboard()  { stack->setCurrentWidget(dashboard); refreshAll(); }
void MainWindow::openDeliveries() { deliveries->refresh(); stack->setCurrentWidget(deliveries); }
void MainWindow::openTracking()   { if (tracking) tracking->refresh(); stack->setCurrentWidget(tracking); }
void MainWindow::openAssign()     { assign->refresh();     stack->setCurrentWidget(assign);     }
void MainWindow::openDrivers()    { drivers->refresh();    stack->setCurrentWidget(drivers);    }
void MainWindow::openCustomers()  { customers->refresh();  stack->setCurrentWidget(customers);  }
void MainWindow::openLocations()  { locations->refresh();  stack->setCurrentWidget(locations);  }
void MainWindow::openSearch()     {                        stack->setCurrentWidget(search);     }
void MainWindow::openDsa()        { dsa->refresh();        stack->setCurrentWidget(dsa);        }
void MainWindow::openReports()    { reports->refresh();    stack->setCurrentWidget(reports);    }
void MainWindow::openSettings()   { settings->refresh();   stack->setCurrentWidget(settings);   }
void MainWindow::doLogout()       { QApplication::exit(7); }

void MainWindow::switchToPage(int idx) {
    switch (idx) {
        case 0: openDashboard(); break;
        case 1: openDeliveries(); break;
        case 2: openTracking(); break;
        case 3: openAssign(); break;
        case 4: openDrivers(); break;
        case 5: openCustomers(); break;
        case 6: openLocations(); break;
        case 7: openSearch(); break;
        case 8: openDsa(); break;
        case 9: openReports(); break;
        case 10: openSettings(); break;
    }
    selectNav(idx);
}
