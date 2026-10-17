#include "Charts.h"
#include <QPainter>
#include <QPainterPath>
#include <QFontMetrics>
#include <QtMath>
#include <algorithm>

namespace {
void drawEmpty(QPainter& p, const QRect& area, const QString& text) {
    p.setPen(QColor("#94a3b8"));
    QFont f = p.font(); f.setPointSize(10); p.setFont(f);
    p.drawText(area, Qt::AlignCenter, text);
}
}

DonutChart::DonutChart(QWidget* parent) : QWidget(parent) { setMinimumSize(300, 230); }

void DonutChart::setData(const std::vector<Slice>& s) { m_slices = s; update(); }

void DonutChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    int total = 0;
    for (auto& s : m_slices) total += s.value;
    int size = qMin(width() / 2 - 20, height() - 50);
    size = qBound(90, size, 140);
    QRectF r(16, (height() - size) / 2, size, size);

    if (total <= 0) {
        p.setPen(QColor("#e2e8f0")); p.setBrush(QColor("#f8fafc"));
        p.drawEllipse(r);
        drawEmpty(p, r.toRect(), "No orders");
    } else {
        double start = -90.0 * 16.0;
        for (auto& s : m_slices) {
            if (s.value <= 0) continue;
            double span = 360.0 * 16.0 * s.value / total;
            p.setBrush(s.color); p.setPen(QPen(Qt::white, 2));
            p.drawPie(r, (int)start, (int)span);
            start += span;
        }
        p.setBrush(Qt::white); p.setPen(Qt::NoPen);
        int hole = (int)(size * 0.58);
        QRectF inner = r.adjusted((size - hole)/2, (size - hole)/2, -(size - hole)/2, -(size - hole)/2);
        p.drawEllipse(inner);
        p.setPen(QColor("#0f172a"));
        QFont bf = p.font(); bf.setBold(true); bf.setPointSize(15);
        p.setFont(bf);
        p.drawText(inner.adjusted(0, -6, 0, 0), Qt::AlignCenter, QString::number(total));
        p.setPen(QColor("#94a3b8"));
        QFont sf = p.font(); sf.setBold(false); sf.setPointSize(8);
        p.setFont(sf);
        p.drawText(inner.adjusted(0, 16, 0, 0), Qt::AlignCenter, "TOTAL");
    }

    int lx = (int)r.right() + 18;
    int ly = qMax(20, (int)(height() - m_slices.size() * 26) / 2);
    QFont lf = font(); lf.setPointSize(9);
    p.setFont(lf);
    for (auto& s : m_slices) {
        p.setBrush(s.color); p.setPen(Qt::NoPen);
        p.drawRoundedRect(lx, ly + 3, 10, 10, 3, 3);
        p.setPen(QColor("#334155"));
        p.drawText(lx + 16, ly + 12, s.label);
        int pct = total > 0 ? (int)(100.0 * s.value / total) : 0;
        QFont cf = p.font(); cf.setBold(true);
        p.setFont(cf);
        p.setPen(QColor("#0f172a"));
        p.drawText(QRect(width() - 65, ly, 55, 20), Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1%").arg(pct));
        p.setFont(lf);
        ly += 24;
    }
}

BarChart::BarChart(QWidget* parent) : QWidget(parent) { setMinimumSize(300, 230); }

void BarChart::setData(const std::vector<Bar>& b) { m_bars = b; update(); }

void BarChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (m_bars.empty()) {
        drawEmpty(p, rect(), "No data");
        return;
    }

    int mx = 1;
    for (auto& b : m_bars) mx = qMax(mx, b.value);
    int n         = (int)m_bars.size();
    int chartTop  = 24;
    int chartH    = height() - 60;
    int chartLeft = 16;
    int w         = width() - 32;
    int bw        = qBound(22, (w - 14 * (n - 1)) / qMax(1, n), 44);
    int totalBarsW = n * bw + (n - 1) * 14;
    int x         = chartLeft + qMax(0, (w - totalBarsW) / 2);

    p.setPen(QColor("#f1f5f9"));
    for (int g = 1; g <= 3; g++) {
        int gy = chartTop + chartH - (chartH * g / 4);
        p.drawLine(chartLeft, gy, chartLeft + w, gy);
    }
    p.setPen(QColor("#e2e8f0"));
    p.drawLine(chartLeft, chartTop + chartH, chartLeft + w, chartTop + chartH);

    QFont lf = font(); lf.setPointSize(8);
    p.setFont(lf);
    for (auto& b : m_bars) {
        int bh = b.value > 0 ? qMax(6, (int)((double)b.value / mx * chartH)) : 0;
        QRect bar(x, chartTop + chartH - bh, bw, bh);
        if (bh > 0) {
            p.setBrush(b.color); p.setPen(Qt::NoPen);
            p.drawRoundedRect(bar, 5, 5);
        }

        p.setPen(QColor("#0f172a"));
        QFont vf = p.font(); vf.setBold(true); vf.setPointSize(9);
        p.setFont(vf);
        p.drawText(QRect(x - 8, chartTop + chartH - bh - 20, bw + 16, 18), Qt::AlignHCenter | Qt::AlignBottom, QString::number(b.value));

        p.setPen(QColor("#64748b"));
        p.setFont(lf);
        p.drawText(QRect(x - 8, chartTop + chartH + 6, bw + 16, 24), Qt::AlignHCenter | Qt::AlignTop, b.label);
        x += bw + 14;
    }
}

HBarChart::HBarChart(QWidget* parent) : QWidget(parent) { setMinimumSize(280, 210); }

void HBarChart::setData(const std::vector<Item>& items) { m_items = items; update(); }

void HBarChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (m_items.empty()) {
        drawEmpty(p, rect(), "No data");
        return;
    }

    int mx = 1;
    for (auto& it : m_items) mx = qMax(mx, it.count);

    int y = 16;
    int leftW = 110;
    int rightW = 44;
    int barW = qMax(40, width() - leftW - rightW - 36);

    QFont lf = font(); lf.setPointSize(9);
    p.setFont(lf);

    for (auto& it : m_items) {
        p.setPen(QColor("#334155"));
        p.drawText(QRect(16, y, leftW, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   QFontMetrics(lf).elidedText(it.name, Qt::ElideRight, leftW - 8));

        int bw = qMax(8, (int)((double)it.count / mx * barW));
        QRect bgBar(16 + leftW, y + 4, barW, 10);
        QRect fgBar(16 + leftW, y + 4, bw, 10);

        p.setBrush(QColor("#f1f5f9")); p.setPen(Qt::NoPen);
        p.drawRoundedRect(bgBar, 5, 5);

        p.setBrush(QColor("#2563eb"));
        p.drawRoundedRect(fgBar, 5, 5);

        p.setPen(QColor("#0f172a"));
        QFont cf = p.font(); cf.setBold(true);
        p.setFont(cf);
        p.drawText(QRect(16 + leftW + barW + 8, y, rightW, 20), Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.count));
        p.setFont(lf);
        y += 28;
    }
}

RouteMapWidget::RouteMapWidget(QWidget* parent) : QWidget(parent) { setMinimumSize(320, 160); }

void RouteMapWidget::setRoute(const QString& origin, const QString& destination,
                              const QString& status, double progress) {
    m_origin = origin;
    m_destination = destination;
    m_status = status;
    m_progress = qBound(0.0, progress, 1.0);
    update();
}

void RouteMapWidget::clearRoute() {
    m_origin.clear();
    m_destination.clear();
    m_status.clear();
    m_progress = 0.0;
    update();
}

void RouteMapWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (m_origin.isEmpty()) {
        drawEmpty(p, rect(), "Select an order to see its route");
        return;
    }

    int margin = 70;
    int y = height() / 2 + 8;
    double x1 = margin;
    double x2 = width() - margin;
    double xp = x1 + (x2 - x1) * m_progress;

    p.setPen(QPen(QColor("#e2e8f0"), 6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(x1, y), QPointF(x2, y));
    p.setPen(QPen(QColor("#2563eb"), 6, Qt::SolidLine, Qt::RoundCap));
    p.drawLine(QPointF(x1, y), QPointF(xp, y));

    auto dot = [&](double x, const QColor& col) {
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(col);
        p.drawEllipse(QPointF(x, y), 7, 7);
    };
    dot(x1, QColor("#16a34a"));
    dot(x2, QColor("#dc2626"));
    dot(xp, QColor("#2563eb"));

    QFont f = p.font(); f.setPointSize(9); f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);
    auto label = [&](double x, const QString& text, int top, Qt::AlignmentFlag align) {
        QString shown = fm.elidedText(text, Qt::ElideRight, 150);
        int w = fm.horizontalAdvance(shown) + 4;
        int left = qBound(4, (int)x - w / 2, width() - w - 4);
        if (align == Qt::AlignLeft) left = qMax(4, (int)x - 8);
        if (align == Qt::AlignRight) left = qMin(width() - w - 4, (int)x - w + 8);
        p.setPen(QColor("#0f172a"));
        p.drawText(QRect(left, top, w, 20), Qt::AlignCenter, shown);
    };
    label(x1, m_origin.split(',')[0].trimmed(), y - 34, Qt::AlignLeft);
    label(x2, m_destination.split(',')[0].trimmed(), y - 34, Qt::AlignRight);

    QFont sf = p.font(); sf.setPointSize(9); sf.setBold(false);
    p.setFont(sf);
    p.setPen(QColor("#64748b"));
    p.drawText(QRect(0, y + 16, width(), 20), Qt::AlignCenter, m_status);
}
