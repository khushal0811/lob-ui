#include "ui/metrics_panel.hpp"
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace lob_qt {

static QLabel* make_value_label(QWidget* parent) {
    auto* lbl = new QLabel("—", parent);
    lbl->setStyleSheet("color: #cdd6f4; font-family: 'Menlo', monospace;");
    return lbl;
}

MetricsPanel::MetricsPanel(QWidget* parent) : QWidget(parent) {
    throughput_label_  = make_value_label(this);
    p50_label_         = make_value_label(this);
    p95_label_         = make_value_label(this);
    p99_label_         = make_value_label(this);
    max_label_         = make_value_label(this);
    fill_rate_label_   = make_value_label(this);
    cancel_rate_label_ = make_value_label(this);
    best_bid_label_    = make_value_label(this);
    best_ask_label_    = make_value_label(this);
    spread_label_      = make_value_label(this);
    last_trade_label_  = make_value_label(this);
    order_count_label_ = make_value_label(this);

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(4);
    form->addRow("Throughput:",  throughput_label_);
    form->addRow("p50 latency:", p50_label_);
    form->addRow("p95 latency:", p95_label_);
    form->addRow("p99 latency:", p99_label_);
    form->addRow("Max latency:", max_label_);
    form->addRow("Fill rate:",   fill_rate_label_);
    form->addRow("Cancel rate:", cancel_rate_label_);
    form->addRow("Best bid:",    best_bid_label_);
    form->addRow("Best ask:",    best_ask_label_);
    form->addRow("Spread:",      spread_label_);
    form->addRow("Last trade:",  last_trade_label_);
    form->addRow("Orders:",      order_count_label_);

    auto* header = new QLabel("<b>Metrics</b>", this);
    header->setStyleSheet("color: #89b4fa; font-size: 13px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(header);
    layout->addLayout(form);
    layout->addStretch();
    setLayout(layout);
}

void MetricsPanel::update(const lob_qt::MetricsSnapshot& snap) {
    throughput_label_->setText(
        QString("%1 ev/s").arg(snap.throughput_eps, 0, 'f', 0));

    auto ns_str = [](quint64 ns) -> QString {
        if (ns == 0)   return "—";
        if (ns < 1000) return QString("%1 ns").arg(ns);
        return QString("%1 µs").arg(ns / 1000.0, 0, 'f', 1);
    };

    p50_label_->setText(ns_str(snap.p50_ns));
    p95_label_->setText(ns_str(snap.p95_ns));
    p99_label_->setText(ns_str(snap.p99_ns));
    max_label_->setText(ns_str(snap.max_ns));

    fill_rate_label_->setText(
        snap.fill_rate > 0
            ? QString("%1%").arg(snap.fill_rate * 100.0, 0, 'f', 1)
            : "—");
    cancel_rate_label_->setText(
        snap.cancel_rate > 0
            ? QString("%1%").arg(snap.cancel_rate * 100.0, 0, 'f', 1)
            : "—");
}

void MetricsPanel::update_book_info(const lob_qt::BookSnapshot& snap) {
    best_bid_label_->setText(snap.best_bid   > 0 ? QString::number(snap.best_bid)   : "—");
    best_ask_label_->setText(snap.best_ask   > 0 ? QString::number(snap.best_ask)   : "—");
    spread_label_->setText(  snap.spread     > 0 ? QString::number(snap.spread)     : "—");
    last_trade_label_->setText(snap.last_trade > 0 ? QString::number(snap.last_trade) : "—");
    order_count_label_->setText(QString::number(snap.order_count));
}

} // namespace lob_qt
