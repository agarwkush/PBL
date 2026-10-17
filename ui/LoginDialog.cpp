#include "LoginDialog.h"
#include "../core/LogisticsSystem.h"
#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFont>
#include <QFrame>
#include <QFormLayout>
#include <string>

namespace {

const char* kTypeStyle =
    "QPushButton{background:#f1f5f9;color:#334155;font-weight:700;font-size:13px;"
    "padding:10px 0;border:1px solid #e2e8f0;}"
    "QPushButton:hover{background:#e2e8f0;}"
    "QPushButton:checked{background:#2563eb;color:#ffffff;border:1px solid #2563eb;}";

}

LoginDialog::LoginDialog(LogisticsSystem *sys, QWidget *parent)
    : QDialog(parent), system(sys)
{
    setWindowTitle("LogiCore — Sign in");
    setFixedSize(860, 540);
    setStyleSheet("QDialog{background:#0f172a;}");

    QHBoxLayout* root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    QWidget* left = new QWidget(this);
    left->setObjectName("brandPanel");
    left->setFixedWidth(320);
    left->setStyleSheet(
        "QWidget#brandPanel{background:#1e3a8a;}"
        "QWidget#brandPanel QLabel{background:transparent;}");
    QVBoxLayout* L = new QVBoxLayout(left);
    L->setContentsMargins(32, 40, 32, 32);
    L->setSpacing(14);

    QLabel* logo = new QLabel("LogiCore", left);
    QFont lf = logo->font(); lf.setPointSize(26); lf.setBold(true);
    logo->setFont(lf);
    logo->setStyleSheet("color:white;");
    QLabel* tag = new QLabel("Logistics and delivery management", left);
    tag->setWordWrap(true);
    tag->setStyleSheet("color:#bfdbfe;font-size:13px;");
    L->addWidget(logo);
    L->addWidget(tag);
    L->addSpacing(18);

    struct Who { const char* title; const char* text; };
    const Who roles[] = {
        {"Admin", "Manage orders, drivers, customers, reports and settings."},
        {"Driver", "See the deliveries assigned to you and update their status."},
        {"Customer", "Place orders and track their progress."},
    };
    for (const Who& w : roles) {
        QLabel* head = new QLabel(w.title, left);
        head->setStyleSheet("color:#ffffff;font-weight:700;font-size:13px;");
        QLabel* body = new QLabel(w.text, left);
        body->setWordWrap(true);
        body->setStyleSheet("color:#bfdbfe;font-size:12px;");
        L->addWidget(head);
        L->addWidget(body);
        L->addSpacing(4);
    }
    L->addStretch();

    QWidget* right = new QWidget(this);
    right->setObjectName("formPanel");
    right->setStyleSheet(
        "QWidget#formPanel{background:#ffffff;}"
        "QWidget#formPanel QLabel{background:transparent;}");
    QVBoxLayout* R = new QVBoxLayout(right);
    R->setContentsMargins(40, 36, 40, 32);
    R->setSpacing(12);

    QLabel* t = new QLabel("Sign in", right);
    QFont tf = t->font(); tf.setPointSize(20); tf.setBold(true);
    t->setFont(tf);
    t->setStyleSheet("color:#0f172a;");
    QLabel* s = new QLabel("Choose how you are signing in.", right);
    s->setStyleSheet("color:#64748b;font-size:13px;");

    QHBoxLayout* typeRow = new QHBoxLayout();
    typeRow->setSpacing(0);
    roleGroup = new QButtonGroup(this);
    roleGroup->setExclusive(true);
    adminBtn    = new QPushButton("Admin", right);
    driverBtn   = new QPushButton("Driver", right);
    customerBtn = new QPushButton("Customer", right);
    for (QPushButton* b : {adminBtn, driverBtn, customerBtn}) {
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(kTypeStyle);
        roleGroup->addButton(b);
        typeRow->addWidget(b);
    }
    adminBtn->setChecked(true);
    adminBtn->setStyleSheet(kTypeStyle);

    roleHint = new QLabel(right);
    roleHint->setWordWrap(true);
    roleHint->setStyleSheet("color:#475569;font-size:12px;");

    userEdit = new QLineEdit(right);
    userEdit->setPlaceholderText("Username");
    userEdit->setClearButtonEnabled(true);
    passEdit = new QLineEdit(right);
    passEdit->setPlaceholderText("Password");
    passEdit->setEchoMode(QLineEdit::Password);
    Theme::styleInput(userEdit);
    Theme::styleInput(passEdit);

    msg = new QLabel("", right);
    msg->setWordWrap(true);
    msg->setVisible(false);

    QPushButton* btn = new QPushButton("Sign in", right);
    btn->setMinimumHeight(42);
    Theme::stylePrimaryBtn(btn);

    QPushButton* regBtn = new QPushButton("New customer? Create an account", right);
    regBtn->setCursor(Qt::PointingHandCursor);
    regBtn->setFlat(true);
    regBtn->setStyleSheet("QPushButton{color:#2563eb;font-weight:700;font-size:12px;text-align:left;padding:4px 0;border:none;}");

    R->addWidget(t);
    R->addWidget(s);
    R->addSpacing(4);
    R->addLayout(typeRow);
    R->addWidget(roleHint);
    R->addSpacing(6);
    R->addWidget(userEdit);
    R->addWidget(passEdit);
    R->addWidget(btn);
    R->addWidget(msg);
    R->addWidget(regBtn);
    R->addStretch();

    root->addWidget(left);
    root->addWidget(right, 1);

    connect(roleGroup, &QButtonGroup::buttonClicked, this, &LoginDialog::roleChanged);
    connect(btn,      &QPushButton::clicked,     this, &LoginDialog::tryLogin);
    connect(userEdit, &QLineEdit::returnPressed,  this, &LoginDialog::tryLogin);
    connect(passEdit, &QLineEdit::returnPressed,  this, &LoginDialog::tryLogin);
    connect(regBtn,   &QPushButton::clicked,      this, &LoginDialog::registerCustomer);
    roleChanged();
}

Role LoginDialog::selectedRole() const {
    if (driverBtn->isChecked()) return Role::DeliveryMan;
    if (customerBtn->isChecked()) return Role::Customer;
    return Role::LogisticsTeam;
}

void LoginDialog::roleChanged() {
    Role r = selectedRole();
    if (r == Role::LogisticsTeam) roleHint->setText("Admin accounts are created on first start. Username: admin.");
    else if (r == Role::DeliveryMan) roleHint->setText("Driver logins are created by the admin on the Drivers page.");
    else roleHint->setText("Use your customer account, or create one below.");
    msg->setVisible(false);
}

void LoginDialog::tryLogin()
{
    std::string u = userEdit->text().trimmed().toStdString();
    std::string p = passEdit->text().toStdString();
    auto fail = [&](const QString& m){
        msg->setText(m);
        msg->setStyleSheet("color:#b91c1c;font-weight:600;background:#fef2f2;"
                           "border:1px solid #fecaca;border-radius:8px;padding:8px 12px;");
        msg->setVisible(true);
    };
    if (u.empty() || p.empty()) { fail("Enter your username and password."); return; }
    LogisticsSystem::LoginOutcome outcome = system->loginAs(u, p, selectedRole());
    if (outcome.ok) accept();
    else fail(QString::fromStdString(outcome.error));
}

void LoginDialog::registerCustomer() {
    QDialog d(this);
    d.setWindowTitle("Create Customer Account");
    d.setFixedSize(440, 420);
    d.setStyleSheet(
        "QDialog { background-color: #ffffff; }"
        "QLabel { color: #0f172a; font-weight: 600; font-size: 13px; }"
    );
    QVBoxLayout* dl = new QVBoxLayout(&d);
    dl->setContentsMargins(24, 20, 24, 20);
    dl->setSpacing(12);

    QLabel* title = new QLabel("Create Customer Account", &d);
    QFont tf = title->font(); tf.setPointSize(15); tf.setBold(true);
    title->setFont(tf);
    title->setStyleSheet("color:#0f172a;");
    dl->addWidget(title);

    QFormLayout* f = new QFormLayout();
    f->setSpacing(10);
    QLineEdit* n  = new QLineEdit(&d); n->setPlaceholderText("Full name");
    QLineEdit* ph = new QLineEdit(&d); ph->setPlaceholderText("10-digit phone number");
    QLineEdit* ad = new QLineEdit(&d); ad->setPlaceholderText("Area, e.g. Baner");
    QLineEdit* un = new QLineEdit(&d); un->setPlaceholderText("3 to 20 letters, digits or _");
    QLineEdit* pw = new QLineEdit(&d); pw->setPlaceholderText("At least 4 characters");
    pw->setEchoMode(QLineEdit::Password);
    for (auto w : {n, ph, ad, un, pw}) Theme::styleInput(w);
    f->addRow("Full name",   n);
    f->addRow("Phone",       ph);
    f->addRow("Address",     ad);
    f->addRow("Username",    un);
    f->addRow("Password",    pw);
    dl->addLayout(f);

    QLabel* m = new QLabel("", &d); m->setWordWrap(true); m->setVisible(false);
    dl->addWidget(m);

    QPushButton* ok = new QPushButton("Create Account", &d);
    ok->setMinimumHeight(40);
    Theme::stylePrimaryBtn(ok, Theme::SUCCESS);
    dl->addWidget(ok);

    connect(ok, &QPushButton::clicked, &d, [&]{
        bool done = system->registerCustomerUser(
            n->text().toStdString(), ph->text().toStdString(), ad->text().toStdString(),
            un->text().trimmed().toStdString(), pw->text().toStdString());
        if (done) d.accept();
        else {
            m->setText(QString::fromStdString(system->lastError()));
            m->setStyleSheet("color:#dc2626;font-size:12px;font-weight:600;background:#fef2f2;border:1px solid #fecaca;border-radius:6px;padding:6px;");
            m->setVisible(true);
        }
    });
    if (d.exec() == QDialog::Accepted) {
        customerBtn->setChecked(true);
        roleChanged();
        userEdit->setText(un->text().trimmed());
        passEdit->clear();
        passEdit->setFocus();
        msg->setText("Account created. Enter your password to sign in.");
        msg->setStyleSheet("color:#15803d;font-weight:600;background:#f0fdf4;"
                          "border:1px solid #bbf7d0;border-radius:8px;padding:8px 12px;");
        msg->setVisible(true);
    }
}
