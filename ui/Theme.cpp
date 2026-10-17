#include "Theme.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QFrame>
#include <QTableWidget>
#include <QHeaderView>
#include <QTimer>
#include <QPushButton>
#include <QBrush>
#include <QColor>
#include <QStringList>

namespace Theme {

QWidget* pageHeader(QWidget* parent, const QString& title, const QString& subtitle) {
    QWidget* w = new QWidget(parent);
    QVBoxLayout* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 6);
    l->setSpacing(4);
    QLabel* t = new QLabel(title, w);
    QFont tf = t->font(); tf.setPointSize(20); tf.setBold(true);
    t->setFont(tf);
    t->setStyleSheet(QString("color:%1;").arg(INK));
    l->addWidget(t);
    if (!subtitle.isEmpty()) {
        QLabel* s = new QLabel(subtitle, w);
        s->setStyleSheet(QString("color:%1;font-size:13px;").arg(MUTED));
        s->setWordWrap(true);
        l->addWidget(s);
    }
    return w;
}

QFrame* sectionCard(QWidget* parent, const QString& title, const QString& subtitle) {
    QFrame* c = new QFrame(parent);
    c->setObjectName("lcSection");
    c->setStyleSheet(
        "QFrame#lcSection{background:#ffffff;border:1px solid #e2e8f0;border-radius:12px;}"
        "QFrame#lcSection QLabel{background:transparent;}");
    QVBoxLayout* l = new QVBoxLayout(c);
    l->setContentsMargins(CARD_PAD, CARD_PAD, CARD_PAD, CARD_PAD);
    l->setSpacing(10);
    if (!title.isEmpty()) {
        QHBoxLayout* h = new QHBoxLayout();
        h->setSpacing(8);
        QLabel* t = new QLabel(title, c);
        QFont tf = t->font(); tf.setBold(true); tf.setPointSize(13);
        t->setFont(tf);
        t->setStyleSheet(QString("color:%1;").arg(INK));
        h->addWidget(t);
        if (!subtitle.isEmpty()) {
            QLabel* s = new QLabel(subtitle, c);
            s->setStyleSheet(QString("color:%1;font-size:12px;").arg(MUTED));
            s->setWordWrap(true);
            h->addWidget(s, 1);
        }
        l->addLayout(h);
    }
    return c;
}

QFrame* statCard(QWidget* parent, const QString& label, const QString& value,
                 const QString& accent, const QString& caption) {
    QFrame* card = new QFrame(parent);
    card->setObjectName("kpiCard");
    card->setMinimumHeight(105);
    card->setStyleSheet(
        "QFrame#kpiCard{background:#ffffff;border:1px solid #e2e8f0;border-radius:12px;}"
        "QFrame#kpiCard:hover{border:1px solid #cbd5e1;}"
        "QFrame#kpiCard QLabel{background:transparent;border:none;}");
    QHBoxLayout* h = new QHBoxLayout(card);
    h->setContentsMargins(18, 16, 18, 16);
    h->setSpacing(14);

    QFrame* accentBar = new QFrame(card);
    accentBar->setFixedWidth(4);
    accentBar->setStyleSheet(QString("background:%1;border-radius:2px;").arg(accent));
    h->addWidget(accentBar);

    QVBoxLayout* textCol = new QVBoxLayout();
    textCol->setSpacing(2);
    textCol->setContentsMargins(0, 0, 0, 0);
    QLabel* t = new QLabel(label, card);
    t->setStyleSheet("color:#64748b;font-size:12px;font-weight:600;");
    textCol->addWidget(t);

    QLabel* v = new QLabel(value, card);
    v->setObjectName("kpiValue");
    v->setStyleSheet("color:#0f172a;font-size:26px;font-weight:800;padding:0;margin:0;");
    textCol->addWidget(v);

    if (!caption.isEmpty()) {
        QLabel* s = new QLabel(caption, card);
        s->setObjectName("kpiCaption");
        s->setStyleSheet("color:#94a3b8;font-size:11px;");
        textCol->addWidget(s);
    }
    h->addLayout(textCol, 1);
    return card;
}

static void baseTable(QTableWidget* t, bool bordered) {
    t->setAlternatingRowColors(true);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setSelectionMode(QAbstractItemView::SingleSelection);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->verticalHeader()->setVisible(false);
    t->verticalHeader()->setDefaultSectionSize(42);
    t->horizontalHeader()->setStretchLastSection(true);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    t->horizontalHeader()->setFixedHeight(40);
    t->setShowGrid(false);
    t->setFocusPolicy(Qt::NoFocus);
    if (bordered) {
        t->setStyleSheet(
            "QTableWidget{background:#ffffff;border:1px solid #e2e8f0;border-radius:10px;"
            "alternate-background-color:#f8fafc;}"
            "QTableWidget::item:selected{background:#eff6ff;color:#1d4ed8;}"
            "QHeaderView::section{background:#f8fafc;color:#64748b;padding:10px 12px;border:none;"
            "border-bottom:1px solid #e2e8f0;font-weight:700;font-size:11px;letter-spacing:0.5px;}");
    } else {
        t->setStyleSheet(
            "QTableWidget{background:#ffffff;border:none;border-radius:0px;"
            "alternate-background-color:#f8fafc;}"
            "QTableWidget::item:selected{background:#eff6ff;color:#1d4ed8;}"
            "QHeaderView::section{background:#f8fafc;color:#64748b;padding:10px 12px;border:none;"
            "border-bottom:1px solid #e2e8f0;font-weight:700;font-size:11px;letter-spacing:0.5px;}");
    }
}

void styleCardTable(QTableWidget* t) { baseTable(t, false); }

void styleInput(QWidget* w) {
    w->setMinimumHeight(BTN_H);
    w->setStyleSheet(
        "QLineEdit,QComboBox,QSpinBox,QDoubleSpinBox{padding:8px 12px;border:1px solid #cbd5e1;"
        "border-radius:8px;background:#ffffff;selection-background-color:#dbeafe;color:#0f172a;font-size:13px;}"
        "QLineEdit:focus,QComboBox:focus{border:1.5px solid #2563eb;}");
}

static QString btnBase(const QString& bg, const QString& fg, const QString& border, const QString& hover) {
    return QString(
        "QPushButton{background:%1;color:%2;font-weight:600;min-height:22px;"
        "padding:8px 16px;border:1px solid %3;border-radius:8px;font-size:13px;}"
        "QPushButton:hover{background:%4;}"
        "QPushButton:disabled{background:#f1f5f9;color:#94a3b8;border:1px solid #e2e8f0;}")
        .arg(bg, fg, border, hover);
}

void stylePrimaryBtn(QWidget* b, const QString& bg) {
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(QString(
        "QPushButton{background:%1;color:#ffffff;font-weight:700;min-height:22px;"
        "padding:8px 18px;border:none;border-radius:8px;font-size:13px;}"
        "QPushButton:hover{background:%2;}"
        "QPushButton:disabled{background:#cbd5e1;color:#64748b;}")
        .arg(bg, ACCENT_DK));
}

void styleDarkBtn(QWidget* b) {
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(btnBase("#0f172a", "white", "#0f172a", "#1e293b"));
}

void styleGhostBtn(QWidget* b) {
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(btnBase("#ffffff", "#334155", "#cbd5e1", "#f8fafc"));
}

void styleDangerBtn(QWidget* b) {
    b->setCursor(Qt::PointingHandCursor);
    b->setStyleSheet(btnBase("#fef2f2", "#b91c1c", "#fecaca", "#fee2e2"));
}

void setMsg(QLabel* m, bool ok, const QString& text) {
    if (!m) return;
    m->setText(text);
    m->setStyleSheet(QString(
        "color:%1;font-weight:600;background:%2;"
        "border:1px solid %3;border-radius:8px;padding:8px 12px;")
        .arg(ok ? SUCCESS : DANGER,
             ok ? "#f0fdf4"   : "#fef2f2",
             ok ? "#bbf7d0"   : "#fecaca"));
}

QString navStyle(bool active) {
    if (active)
        return "QPushButton{background:#2563eb;color:#ffffff;text-align:left;"
               "padding:10px 16px;border:none;border-radius:8px;font-weight:700;font-size:13px;}";
    return "QPushButton{background:transparent;color:#94a3b8;text-align:left;"
           "padding:10px 16px;border:none;border-radius:8px;font-weight:600;font-size:13px;}"
           "QPushButton:hover{background:#1e293b;color:#f8fafc;}";
}

namespace {
struct PillStyle {
    const char* bg;
    const char* fg;
};
}

static PillStyle pillStyle(const QString& kind) {
    QString k = kind.toLower();
    if (k.contains("unavail"))  return {"#e2e8f0", "#334155"};
    if (k.contains("pend"))     return {"#fef3c7", "#92400e"};
    if (k.contains("picked"))   return {"#e0e7ff", "#3730a3"};
    if (k.contains("transit"))  return {"#ede9fe", "#5b21b6"};
    if (k.contains("assign"))   return {"#dbeafe", "#1e40af"};
    if (k.contains("deliver"))  return {"#dcfce7", "#166534"};
    if (k.contains("cancel"))   return {"#fee2e2", "#991b1b"};
    if (k.contains("urgent"))   return {"#fee2e2", "#991b1b"};
    if (k.contains("high"))     return {"#ffedd5", "#9a3412"};
    if (k.contains("normal"))   return {"#f1f5f9", "#334155"};
    if (k.contains("avail") || k.contains("active") || k.contains("free")) return {"#dcfce7", "#166534"};
    if (k.contains("busy"))     return {"#fef3c7", "#92400e"};
    return {"#f1f5f9", "#334155"};
}

QString pillBackground(const QString& kind) {
    return QString::fromLatin1(pillStyle(kind).bg);
}

QString pillForeground(const QString& kind) {
    return QString::fromLatin1(pillStyle(kind).fg);
}

void setPill(QTableWidgetItem* item, const QString& kind) {
    if (!item) return;
    PillStyle style = pillStyle(kind);
    QFont f = item->font(); f.setBold(true); f.setPointSize(10);
    item->setFont(f);
    item->setBackground(QColor(style.bg));
    item->setForeground(QBrush(QColor(style.fg)));
    item->setTextAlignment(Qt::AlignCenter);
}

QLabel* avatar(QWidget* parent, const QString& name, int diameter) {
    QString initials;
    for (const QString& w : name.split(' ', Qt::SkipEmptyParts)) {
        if (initials.size() < 2 && !w.isEmpty()) initials += w[0].toUpper();
    }
    if (initials.isEmpty()) initials = "LC";
    QLabel* a = new QLabel(initials, parent);
    a->setFixedSize(diameter, diameter);
    a->setAlignment(Qt::AlignCenter);
    QFont f = a->font(); f.setBold(true); f.setPointSize(12);
    a->setFont(f);
    a->setStyleSheet(QString(
        "background:#2563eb;color:#ffffff;border-radius:%1px;").arg(diameter / 2));
    return a;
}

void toast(QWidget* parent, const QString& msg, bool ok) {
    for (QObject* ch : parent->children()) {
        if (ch->objectName() == "lcToast") {
            if (QWidget* w = qobject_cast<QWidget*>(ch)) w->deleteLater();
        }
    }
    QLabel* t = new QLabel(msg, parent);
    t->setObjectName("lcToast");
    t->setWordWrap(true);
    t->setMaximumWidth(parent->width() - 60);
    t->setStyleSheet(QString(
        "background:%1;color:white;font-weight:700;padding:10px 18px;"
        "border-radius:10px;border:1px solid %2;font-size:13px;")
        .arg(ok ? "#0f172a" : DANGER, ok ? "#1e293b" : "#fca5a5"));
    t->adjustSize();
    t->move(qMax(12, (parent->width() - t->width()) / 2), 16);
    t->show();
    t->raise();
    QTimer::singleShot(TOAST_MS, t, &QObject::deleteLater);
}

}
