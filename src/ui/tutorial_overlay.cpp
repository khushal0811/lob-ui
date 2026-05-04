#include "ui/tutorial_overlay.hpp"
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QVBoxLayout>

namespace lob_qt {

// ─────────────────────────────────────────────────────────────────────────────
// Per-scenario tutorial content
// ─────────────────────────────────────────────────────────────────────────────

TutorialContent TutorialOverlay::content_for(const QString& scenario) {

    if (scenario == "medium" || scenario == "default") {
        return {
            "🎯  Try It: Cross the Spread",

            "The engine is live. The Order Book shows real bid/ask prices updating every "
            "millisecond. Here is your first exercise:",

            {
                "Look at the <b>Order Book → top green row</b> (Best Bid). "
                    "Note the price — call it <b>B</b>.",
                "In <b>Order Entry</b> set:<br>"
                    "&nbsp;&nbsp;• <b>Type</b> → <span style='color:#a6e3a1;'>Limit</span><br>"
                    "&nbsp;&nbsp;• <b>Side</b> → <span style='color:#f38ba8;'>Sell</span><br>"
                    "&nbsp;&nbsp;• <b>Price</b> → <span style='color:#f9e2af;'>B</span> "
                    "(same as best bid)<br>"
                    "&nbsp;&nbsp;• <b>Quantity</b> → 50",
                "Click <b>Submit Order</b>.",
                "Watch the <b>Trade Tape</b> — your sell <i>crossed</i> the bid and "
                    "filled immediately. The price appears in orange (sell-initiated).",
                "<b>Now try the opposite:</b> set Price to <b>B − 5</b> "
                    "(below the bid). Your order will <i>rest</i> on the Ask side "
                    "in the red table and wait for a buyer."
            },

            "💡 <b>Bonus:</b> Try Type → <b>Market</b> (no price needed). "
                "A market sell sweeps all bids it can find instantly — "
                "watch multiple rows vanish from the book at once."
        };
    }

    if (scenario == "small") {
        return {
            "🎯  Try It: Queue a Large Passive Order",

            "The Small scenario has low activity — perfect for watching your order "
            "sit on the book and slowly get matched.",

            {
                "Look at the <b>Order Book → best ask</b> (red, top). Note price <b>A</b>.",
                "In <b>Order Entry</b> set:<br>"
                    "&nbsp;&nbsp;• <b>Type</b> → Limit<br>"
                    "&nbsp;&nbsp;• <b>Side</b> → <span style='color:#89b4fa;'>Buy</span><br>"
                    "&nbsp;&nbsp;• <b>Price</b> → <b>A − 3</b> (3 ticks below best ask)<br>"
                    "&nbsp;&nbsp;• <b>Quantity</b> → 500",
                "Click <b>Submit Order</b>. Your order appears in the <b>green bid table</b> "
                    "as a new price level with Volume = 500.",
                "Watch it over time — as synthetic sellers hit that price, your quantity "
                    "decreases. When it hits 0 you see a fill in the Trade Tape.",
                "To cancel early: copy the <b>Order ID</b> from the feedback label, "
                    "paste into <b>Cancel by ID</b>, click <b>Cancel Order</b>. "
                    "Your level disappears from the book."
            },

            "💡 <b>Bonus:</b> Submit a second buy at <b>A − 1</b> (one tick below ask). "
                "You'll see two separate green levels. The closer one fills first."
        };
    }

    if (scenario == "large") {
        return {
            "🎯  Try It: Sweep Multiple Levels with a Market Order",

            "The Large scenario has a deep, fast-moving book. Market orders here "
            "are dramatic — they eat through multiple price levels in one shot.",

            {
                "Count the green (bid) rows — there are up to 10 levels visible.",
                "In <b>Order Entry</b> set:<br>"
                    "&nbsp;&nbsp;• <b>Type</b> → <span style='color:#f9e2af;'>Market</span> "
                    "(price field disappears — no price needed)<br>"
                    "&nbsp;&nbsp;• <b>Side</b> → <span style='color:#f38ba8;'>Sell</span><br>"
                    "&nbsp;&nbsp;• <b>Quantity</b> → 10000",
                "Click <b>Submit Order</b>.",
                "Watch the <b>bid table</b> — top levels get consumed. Multiple orange "
                    "fills hit the Trade Tape in rapid succession.",
                "Check <b>Metrics → Fill rate</b>. With a large qty market order, "
                    "your fill rate stat spikes for that second."
            },

            "💡 <b>Bonus:</b> Try an <b>Iceberg</b> order — set Type → Iceberg, "
                "Quantity → 5000, Peak qty → 200. Only 200 shows on the book at a time; "
                "as it fills, the next 200 automatically appear."
        };
    }

    if (scenario == "high_cancel") {
        return {
            "🎯  Try It: Place and Cancel Your Own Order",

            "High-Cancel mode means most synthetic orders are withdrawn quickly. "
            "Practice the full order lifecycle: add → cancel.",

            {
                "Submit a resting buy order:<br>"
                    "&nbsp;&nbsp;• <b>Type</b> → Limit &nbsp; <b>Side</b> → Buy<br>"
                    "&nbsp;&nbsp;• <b>Price</b> → <span style='color:#f9e2af;'>Best Bid − 10</span>"
                    " &nbsp; <b>Qty</b> → 200<br>"
                    "Click <b>Submit Order</b>.",
                "<b>Note the Order ID</b> shown in the green feedback message "
                    "(e.g. \"Order 1002 sent\").",
                "Watch the green bid table — your level appears at that price.",
                "In <b>Cancel by ID</b>: type <b>1002</b> (your ID), "
                    "click <b>Cancel Order</b>.",
                "Your price level disappears from the book instantly — "
                    "the engine confirmed the cancel."
            },

            "💡 <b>Bonus:</b> Try cancelling an ID that doesn't exist (e.g. 9999999). "
                "The engine returns a rejection — check the <b>red feedback label</b> "
                "to see the rejection reason."
        };
    }

    if (scenario == "market_heavy") {
        return {
            "🎯  Try It: Defend Against Market Order Flow",

            "Market-Heavy mode blasts aggressive market orders through the book. "
            "Your limit orders act as liquidity that market orders consume.",

            {
                "Post a <b>large passive sell</b>:<br>"
                    "&nbsp;&nbsp;• Type → Limit &nbsp; Side → Sell<br>"
                    "&nbsp;&nbsp;• Price → <b>Best Ask + 2</b> &nbsp; Qty → 1000",
                "Click <b>Submit Order</b> — your ask appears in the red table.",
                "Watch the book. Incoming market buys will eat through cheaper asks first, "
                    "then reach yours.",
                "When your order fills, a <b>blue BUY entry</b> appears in the tape "
                    "(the buyer was the aggressor, you were passive).",
                "Watch <b>Metrics → Fill rate</b> — in market-heavy mode it's unusually "
                    "high because most events are aggressive."
            },

            "💡 <b>Bonus:</b> Post a <b>Stop order</b>:<br>"
                "Type → Stop, Side → Buy, Stop px → <span style='color:#f9e2af;'>"
                "Best Ask + 5</span>, Qty → 300.<br>"
                "When the market moves up and touches that price, your stop triggers "
                "and becomes a market buy."
        };
    }

    if (scenario == "iceberg_stop") {
        return {
            "🎯  Try It: Iceberg and Stop-Limit Orders",

            "This scenario is designed for testing advanced order types. "
            "Follow these steps to see both in action.",

            {
                "<b>Iceberg order</b> — hides most of its quantity:<br>"
                    "&nbsp;&nbsp;• Type → <b>Iceberg</b> &nbsp; Side → Buy<br>"
                    "&nbsp;&nbsp;• Price → Best Bid &nbsp; Qty → 2000 &nbsp; "
                    "<b>Peak qty → 100</b><br>"
                    "Submit. The book shows Volume = 100, not 2000. "
                    "As fills happen, it auto-replenishes.",
                "<b>Stop-Limit order</b> — only activates when price hits stop px:<br>"
                    "&nbsp;&nbsp;• Type → <b>Stop-Limit</b> &nbsp; Side → Buy<br>"
                    "&nbsp;&nbsp;• Price → Best Ask + 1 &nbsp; "
                    "<b>Stop px → Best Ask + 3</b> &nbsp; Qty → 500<br>"
                    "Submit. The order is <i>dormant</i> until the market rises to Stop px, "
                    "then it activates as a limit at Price.",
                "Watch the <b>Spread Chart</b> — iceberg replenishment keeps the spread "
                    "tight as passive orders stay topped up."
            },

            "💡 <b>Want to add your own scenario?</b> Create a CSV with columns:<br>"
                "<span style='font-family:Menlo,monospace;font-size:11px;'>"
                "event_type, order_id, side, price, quantity, order_type</span><br>"
                "Load it via <b>Replay Controls → Load CSV</b> and step through "
                "event by event."
        };
    }

    // Fallback
    return {
        "🎯  Explore the Order Book",
        "The engine is running. Try submitting an order:",
        {
            "Set Type → Limit, Side → Buy, Price → Best Ask, Qty → 100.",
            "Click Submit Order and watch it fill immediately in the Trade Tape.",
            "Try cancelling it first with Cancel by ID."
        },
        "💡 Select a scenario from Replay Controls to see different market conditions."
    };
}

// ─────────────────────────────────────────────────────────────────────────────
// Widget implementation
// ─────────────────────────────────────────────────────────────────────────────

TutorialOverlay::TutorialOverlay(QWidget* parent) : QWidget(parent) {
    setAutoFillBackground(false);
    build_card();
    hide();
}

void TutorialOverlay::build_card() {
    // Card frame
    card_ = new QFrame(this);
    card_->setObjectName("tutorialCard");
    card_->setStyleSheet(
        "QFrame#tutorialCard {"
        "  background: #1e1e2e;"
        "  border: 2px solid #89b4fa;"
        "  border-radius: 12px;"
        "}"
        "QLabel { background: transparent; border: none; color: #cdd6f4; }"
        "QPushButton#dismissBtn {"
        "  background: #89b4fa; color: #1e1e2e;"
        "  border-radius: 5px; padding: 8px 20px;"
        "  font-weight: bold; font-size: 13px;"
        "}"
        "QPushButton#dismissBtn:hover { background: #b4d0fb; }"
        "QPushButton#skipBtn {"
        "  background: transparent; color: #6c7086;"
        "  border: 1px solid #45475a; border-radius: 5px; padding: 8px 20px;"
        "}"
        "QPushButton#skipBtn:hover { color: #cdd6f4; border-color: #cdd6f4; }");
    card_->setFixedWidth(580);

    title_label_ = new QLabel(card_);
    title_label_->setStyleSheet("font-size: 15px; font-weight: bold; color: #89b4fa;");
    title_label_->setWordWrap(true);

    intro_label_ = new QLabel(card_);
    intro_label_->setStyleSheet("color: #a6adc8; font-size: 12px;");
    intro_label_->setWordWrap(true);

    steps_label_ = new QLabel(card_);
    steps_label_->setStyleSheet("color: #cdd6f4; font-size: 12px;");
    steps_label_->setWordWrap(true);
    steps_label_->setTextFormat(Qt::RichText);

    bonus_label_ = new QLabel(card_);
    bonus_label_->setStyleSheet(
        "color: #f9e2af; font-size: 11px; background: #181825;"
        "border: 1px solid #45475a; border-radius: 5px; padding: 6px 8px;");
    bonus_label_->setWordWrap(true);
    bonus_label_->setTextFormat(Qt::RichText);

    dismiss_btn_ = new QPushButton("Got it — I'll try it!", card_);
    dismiss_btn_->setObjectName("dismissBtn");
    skip_btn_    = new QPushButton("Skip", card_);
    skip_btn_->setObjectName("skipBtn");

    auto* btn_row = new QHBoxLayout;
    btn_row->addStretch();
    btn_row->addWidget(dismiss_btn_);
    btn_row->addWidget(skip_btn_);

    // Scenarios reference section
    auto* scenarios_title = new QLabel(
        "<b style='color:#cba6f7;'>📋  Built-in Scenarios</b>", card_);
    scenarios_title->setStyleSheet("background:transparent;border:none;");

    auto* scenarios_body = new QLabel(
        "<table style='color:#a6adc8;font-size:11px;' cellspacing='4'>"
        "<tr><td><b style='color:#cdd6f4;'>medium</b></td>"
            "<td>Balanced flow — great starting point</td></tr>"
        "<tr><td><b style='color:#cdd6f4;'>small</b></td>"
            "<td>Low volume — watch individual orders fill slowly</td></tr>"
        "<tr><td><b style='color:#cdd6f4;'>large</b></td>"
            "<td>High volume — book updates every millisecond</td></tr>"
        "<tr><td><b style='color:#cdd6f4;'>high_cancel</b></td>"
            "<td>70% of orders cancelled — practice the cancel flow</td></tr>"
        "<tr><td><b style='color:#cdd6f4;'>market_heavy</b></td>"
            "<td>Aggressive market orders dominate</td></tr>"
        "<tr><td><b style='color:#cdd6f4;'>iceberg_stop</b></td>"
            "<td>Advanced order types — iceberg + stop-limit</td></tr>"
        "</table>", card_);
    scenarios_body->setTextFormat(Qt::RichText);
    scenarios_body->setStyleSheet("background:transparent;border:none;");

    auto* csv_label = new QLabel(
        "<span style='color:#a6adc8;font-size:11px;'>"
        "<b style='color:#cba6f7;'>📂  Add your own:</b> "
        "Replay Controls → <b>Load CSV…</b> — format: "
        "<span style='font-family:Menlo,monospace;'>"
        "event_type, order_id, side, price, quantity, order_type</span>"
        "</span>", card_);
    csv_label->setTextFormat(Qt::RichText);
    csv_label->setWordWrap(true);
    csv_label->setStyleSheet("background:transparent;border:none;");

    // Separator line
    auto* sep = new QFrame(card_);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #313244; background: #313244; max-height: 1px;");

    auto* card_layout = new QVBoxLayout(card_);
    card_layout->setContentsMargins(24, 20, 24, 20);
    card_layout->setSpacing(10);
    card_layout->addWidget(title_label_);
    card_layout->addWidget(intro_label_);
    card_layout->addWidget(steps_label_);
    card_layout->addWidget(bonus_label_);
    card_layout->addSpacing(4);
    card_layout->addWidget(sep);
    card_layout->addWidget(scenarios_title);
    card_layout->addWidget(scenarios_body);
    card_layout->addWidget(csv_label);
    card_layout->addSpacing(4);
    card_layout->addLayout(btn_row);

    // Centre the card in the overlay
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addStretch();
    outer->addWidget(card_, 0, Qt::AlignCenter);
    outer->addStretch();
    setLayout(outer);

    connect(dismiss_btn_, &QPushButton::clicked, this, &TutorialOverlay::on_dismiss);
    connect(skip_btn_,    &QPushButton::clicked, this, &TutorialOverlay::on_dismiss);
}

void TutorialOverlay::populate(const TutorialContent& c) {
    title_label_->setText(c.title);
    intro_label_->setText(c.intro);

    // Build numbered steps as rich text
    QString steps_html;
    for (int i = 0; i < c.steps.size(); ++i) {
        steps_html += QString(
            "<p style='margin:4px 0;'>"
            "<span style='color:#89b4fa;font-weight:bold;'>%1.</span>&nbsp;"
            "%2</p>").arg(i + 1).arg(c.steps[i]);
    }
    steps_label_->setText(steps_html);
    bonus_label_->setText(c.bonus_tip);
    bonus_label_->setVisible(!c.bonus_tip.isEmpty());
}

void TutorialOverlay::show_for_scenario(const QString& scenario_name) {
    populate(content_for(scenario_name));

    if (parentWidget()) {
        setGeometry(parentWidget()->rect());
    }
    show();
    raise();
}

void TutorialOverlay::on_dismiss() {
    hide();
    emit dismissed();
}

void TutorialOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), QColor(0, 0, 0, 160));
}

} // namespace lob_qt
