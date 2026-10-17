#pragma once
#include <QWidget>
#include <QFrame>
#include <QTableWidget>
#include <QLabel>
#include <QString>

namespace Theme {

inline const char* BG        = "#f8fafc";
inline const char* CARD      = "#ffffff";
inline const char* BORDER    = "#e2e8f0";
inline const char* DIVIDER   = "#f1f5f9";
inline const char* INK       = "#0f172a";
inline const char* BODY      = "#334155";
inline const char* MUTED     = "#64748b";
inline const char* FAINT     = "#94a3b8";
inline const char* ACCENT    = "#2563eb";
inline const char* ACCENT_DK = "#1d4ed8";
inline const char* SUCCESS   = "#16a34a";
inline const char* WARN      = "#d97706";
inline const char* DANGER    = "#dc2626";
inline const char* VIOLET    = "#7c3aed";
inline const char* SKY       = "#0284c7";
inline const char* SIDEBAR   = "#0f172a";

inline constexpr int PAGE_MARGIN = 24;
inline constexpr int SECTION_GAP = 16;
inline constexpr int CARD_PAD    = 18;
inline constexpr int BTN_H       = 38;
inline constexpr int TOAST_MS    = 2500;
inline constexpr int LIVE_MS     = 4000;

QWidget* pageHeader(QWidget* parent, const QString& title, const QString& subtitle = QString());
QFrame*  sectionCard(QWidget* parent, const QString& title = QString(), const QString& subtitle = QString());
QFrame*  statCard(QWidget* parent, const QString& label, const QString& value,
                  const QString& accent, const QString& caption = QString());
void     styleCardTable(QTableWidget* t);
void     styleInput(QWidget* w);
void     stylePrimaryBtn(QWidget* b, const QString& bg = ACCENT);
void     styleDarkBtn(QWidget* b);
void     styleGhostBtn(QWidget* b);
void     styleDangerBtn(QWidget* b);
void     setMsg(QLabel* m, bool ok, const QString& text);
QString  navStyle(bool active);
void     setPill(QTableWidgetItem* item, const QString& kind);
QString  pillBackground(const QString& kind);
QString  pillForeground(const QString& kind);
QLabel*  avatar(QWidget* parent, const QString& name, int diameter = 36);
void     toast(QWidget* parent, const QString& msg, bool ok = true);

}
