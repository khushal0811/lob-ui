#include "ui/welcome_overlay.hpp"
#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QVBoxLayout>

namespace lob_qt {

WelcomeOverlay::WelcomeOverlay(QWidget* parent) : QWidget(parent) {
    // Transparent to mouse events on background, opaque on card
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAutoFillBackground(false);

    // ---- Card frame ----
    auto* card = new QFrame(this);
    card->setFrameShape(QFrame::NoFrame);
    card->setStyleSheet(
        "QFrame { background: #1e1e2e; border: 1px solid #45475a; border-radius: 10px; }"
        "QLabel { background: transparent; border: none; }"
        "QPushButton { background: #89b4fa; color: #1e1e2e; border-radius: 4px; "
        "              padding: 10px 24px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background: #b4d0fb; }");
    card->setFixedWidth(620);

    auto* title = new QLabel("<b style='font-size:18px;color:#cdd6f4;'>"
                             "lob-engine Order Book Visualizer</b>", card);
    title->setAlignment(Qt::AlignCenter);

    auto* intro = new QLabel(
        "<span style='color:#a6adc8;font-size:12px;'>"
        "This application visualizes a real-time C++ limit order book and matching<br>"
        "engine running on a dedicated background thread.</span>", card);
    intro->setAlignment(Qt::AlignCenter);
    intro->setWordWrap(true);

    auto make_section = [card](const QString& title, const QString& body) {
        auto* frame = new QFrame(card);
        frame->setStyleSheet(
            "QFrame { background: #181825; border: 1px solid #313244; border-radius: 6px; }");
        auto* lbl_title = new QLabel(
            QString("<b style='color:#89b4fa;'>%1</b>").arg(title), frame);
        auto* lbl_body  = new QLabel(
            QString("<span style='color:#a6adc8;font-size:11px;'>%1</span>").arg(body), frame);
        lbl_body->setWordWrap(true);
        auto* fl = new QVBoxLayout(frame);
        fl->setContentsMargins(10, 6, 10, 6);
        fl->setSpacing(2);
        fl->addWidget(lbl_title);
        fl->addWidget(lbl_body);
        return frame;
    };

    auto* paused_note = new QLabel(
        "<span style='color:#f9e2af;font-size:12px;'>"
        "The engine is currently <b>PAUSED</b>. Press Start to begin the medium market scenario."
        "</span>", card);
    paused_note->setAlignment(Qt::AlignCenter);
    paused_note->setWordWrap(true);

    auto* start_btn = new QPushButton("▶  Start — Run Medium Scenario", card);

    auto* vl = new QVBoxLayout(card);
    vl->setContentsMargins(32, 24, 32, 28);
    vl->setSpacing(12);
    vl->addWidget(title);
    vl->addWidget(intro);
    vl->addSpacing(4);
    vl->addWidget(make_section("ORDER BOOK  (top-left)",
        "Red rows = Ask side (sellers). Best ask closest to mid.<br>"
        "Green rows = Bid side (buyers). Best bid at top.<br>"
        "Mid price and spread shown between the two tables."));
    vl->addWidget(make_section("TRADE TAPE  (bottom-left)",
        "Every executed trade appears here in real time.<br>"
        "Blue = buy-initiated trade.  Orange = sell-initiated trade."));
    vl->addWidget(make_section("ORDER ENTRY  (top-right)",
        "Submit limit, market, stop, stop-limit, or iceberg orders.<br>"
        "Orders appear on the book instantly via the matching engine."));
    vl->addWidget(make_section("REPLAY CONTROLS  (middle-right)",
        "Load a CSV event log and replay it step-by-step or at variable speed.<br>"
        "Select and run built-in market scenarios from the dropdown."));
    vl->addWidget(make_section("METRICS + SPREAD CHART  (bottom-right)",
        "Live throughput (ev/s), latency percentiles, fill/cancel rates,<br>"
        "and a rolling spread line chart."));
    vl->addSpacing(4);
    vl->addWidget(paused_note);
    vl->addSpacing(4);
    vl->addWidget(start_btn, 0, Qt::AlignCenter);

    // Centre card inside overlay
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addStretch();
    outer->addWidget(card, 0, Qt::AlignCenter);
    outer->addStretch();
    setLayout(outer);

    connect(start_btn, &QPushButton::clicked, this, [this] {
        hide();
        emit start_requested();
    });
}

void WelcomeOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 180));
}

void WelcomeOverlay::resizeEvent(QResizeEvent* ev) {
    QWidget::resizeEvent(ev);
    if (parentWidget())
        setGeometry(parentWidget()->rect());
}

} // namespace lob_qt
