#include "ui/order_entry.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace lob_qt {

quint64 OrderEntry::next_id_ = 1000;

OrderEntry::OrderEntry(QWidget* parent) : QWidget(parent) {
    setup_ui();
}

void OrderEntry::setup_ui() {
    type_combo_     = new QComboBox(this);
    side_combo_     = new QComboBox(this);
    price_edit_     = new QLineEdit(this);
    qty_edit_       = new QLineEdit(this);
    peak_edit_      = new QLineEdit(this);
    stop_edit_      = new QLineEdit(this);
    submit_btn_     = new QPushButton("Submit Order", this);
    cancel_id_edit_ = new QLineEdit(this);
    cancel_btn_     = new QPushButton("Cancel Order", this);
    feedback_label_ = new QLabel("", this);

    // Explicit style — macOS native theme would otherwise override dark palette
    const QString combo_style =
        "QComboBox { background: #181825; color: #cdd6f4; "
        "border: 1px solid #45475a; border-radius: 3px; padding: 3px 6px; } "
        "QComboBox::drop-down { border: none; width: 20px; } "
        "QComboBox QAbstractItemView { background: #181825; color: #cdd6f4; "
        "selection-background-color: #45475a; }";
    type_combo_->setStyleSheet(combo_style);
    side_combo_->setStyleSheet(combo_style);

    type_combo_->addItems({"Limit", "Market", "Stop", "Stop-Limit", "Iceberg"});
    side_combo_->addItems({"Buy", "Sell"});

    price_edit_->setPlaceholderText("Price (ticks)");
    qty_edit_->setPlaceholderText("Quantity");
    peak_edit_->setPlaceholderText("Peak qty (iceberg)");
    stop_edit_->setPlaceholderText("Stop price");
    cancel_id_edit_->setPlaceholderText("Order ID to cancel");

    peak_edit_->hide();
    stop_edit_->hide();

    auto* form = new QFormLayout;
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(6);
    form->setContentsMargins(0, 0, 0, 0);
    form->addRow("Type:",     type_combo_);
    form->addRow("Side:",     side_combo_);
    form->addRow("Price:",    price_edit_);
    form->addRow("Quantity:", qty_edit_);
    form->addRow("Peak qty:", peak_edit_);
    form->addRow("Stop px:",  stop_edit_);

    feedback_label_->setWordWrap(true);

    auto* cancel_layout = new QHBoxLayout;
    cancel_layout->addWidget(cancel_id_edit_);
    cancel_layout->addWidget(cancel_btn_);

    // ── Order Status Table ───────────────────────────────────────────────────
    status_table_ = new QTableWidget(0, 6, this);
    status_table_->setHorizontalHeaderLabels({"ID", "Side", "Price", "Ordered", "Filled", "St."});
    status_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    status_table_->setSelectionMode(QAbstractItemView::NoSelection);
    status_table_->setShowGrid(false);
    status_table_->setAlternatingRowColors(true);
    status_table_->verticalHeader()->hide();
    status_table_->verticalHeader()->setDefaultSectionSize(19);
    status_table_->setFixedHeight(152);  // exactly 8 rows
    status_table_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    status_table_->setStyleSheet(
        "QTableWidget { border: 1px solid #313244; border-radius: 4px; }"
        "QHeaderView::section { font-size: 11px; padding: 2px 4px; }");

    // Column widths: ID=48, Side=38, Price=54, Ordered=54, Filled=48, Status=30
    auto* hh = status_table_->horizontalHeader();
    hh->setSectionResizeMode(QHeaderView::Fixed);
    hh->resizeSection(0, 48);
    hh->resizeSection(1, 38);
    hh->resizeSection(2, 56);
    hh->resizeSection(3, 56);
    hh->resizeSection(4, 48);
    hh->resizeSection(5, 28);
    // ────────────────────────────────────────────────────────────────────────

    auto* header = new QLabel("<b>Order Entry</b>", this);
    header->setStyleSheet("color: #89b4fa; font-size: 14px; font-weight: bold;");

    auto* status_header = new QLabel("<b>My Orders — Ordered vs Filled</b>", this);
    status_header->setStyleSheet("color: #89b4fa; font-size: 12px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);
    layout->addWidget(header);
    layout->addLayout(form);
    layout->addWidget(submit_btn_);
    layout->addSpacing(4);
    layout->addWidget(new QLabel("Cancel by ID:", this));
    layout->addLayout(cancel_layout);
    layout->addWidget(feedback_label_);
    layout->addSpacing(4);
    layout->addWidget(status_header);
    layout->addWidget(status_table_);
    setLayout(layout);

    connect(submit_btn_, &QPushButton::clicked,
            this, &OrderEntry::on_submit_clicked);
    connect(cancel_btn_, &QPushButton::clicked,
            this, &OrderEntry::on_cancel_clicked);
    connect(type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderEntry::on_type_changed);
}

void OrderEntry::on_type_changed(int index) {
    peak_edit_->setVisible(index == 4);
    stop_edit_->setVisible(index == 2 || index == 3);
    price_edit_->setEnabled(index != 1);  // market orders have no price
}

void OrderEntry::on_submit_clicked() {
    if (!validate_inputs()) return;

    SubmitOrderCommand cmd;
    cmd.id         = next_id_++;
    cmd.side       = (side_combo_->currentIndex() == 0) ? SideUi::Buy : SideUi::Sell;
    cmd.type       = static_cast<OrderTypeUi>(type_combo_->currentIndex());
    cmd.price      = price_edit_->text().toLongLong();
    cmd.quantity   = qty_edit_->text().toULongLong();
    cmd.peak_qty   = peak_edit_->text().toULongLong();
    cmd.stop_price = stop_edit_->text().toLongLong();

    // Track in status table
    UserOrderStatus s;
    s.id          = cmd.id;
    s.side        = (cmd.side == SideUi::Buy) ? "BUY" : "SELL";
    s.type        = type_combo_->currentText();
    s.price       = cmd.price;
    s.ordered_qty = cmd.quantity;
    s.filled_qty  = 0;

    // Maintain newest-first, cap at kMaxTracked
    order_keys_.prepend(cmd.id);
    user_orders_.insert(cmd.id, s);
    if (order_keys_.size() > kMaxTracked) {
        user_orders_.remove(order_keys_.takeLast());
    }
    refresh_status_table();

    emit submit_order(cmd);
    show_accepted(cmd.id);
}

void OrderEntry::on_cancel_clicked() {
    bool ok    = false;
    quint64 id = cancel_id_edit_->text().toULongLong(&ok);
    if (!ok || id == 0) {
        feedback_label_->setStyleSheet("color: #f38ba8;");
        feedback_label_->setText("Invalid order ID");
        return;
    }
    emit cancel_order(id);
    feedback_label_->setStyleSheet("color: #89b4fa;");
    feedback_label_->setText(QString("Cancel sent for ID %1").arg(id));
}

bool OrderEntry::validate_inputs() {
    if (qty_edit_->text().isEmpty() || qty_edit_->text().toULongLong() == 0) {
        feedback_label_->setStyleSheet("color: #f38ba8;");
        feedback_label_->setText("Quantity must be > 0");
        return false;
    }
    int type_idx = type_combo_->currentIndex();
    if (type_idx != 1 && price_edit_->text().isEmpty()) {
        feedback_label_->setStyleSheet("color: #f38ba8;");
        feedback_label_->setText("Price required for this order type");
        return false;
    }
    return true;
}

void OrderEntry::show_rejection(const lob_qt::RejectionRecord& record) {
    feedback_label_->setStyleSheet("color: #f38ba8;");
    feedback_label_->setText(
        QString("Rejected (ID %1): %2").arg(record.order_id).arg(record.reason));
}

void OrderEntry::show_accepted(quint64 order_id) {
    feedback_label_->setStyleSheet("color: #a6e3a1;");
    feedback_label_->setText(QString("Order %1 sent").arg(order_id));
    QTimer::singleShot(3000, this, [this] { feedback_label_->clear(); });
}

void OrderEntry::on_trade_update(const lob_qt::TradeRecord& record) {
    // Check both sides — user order could be aggressor or passive
    quint64 matched_id = 0;
    if (user_orders_.contains(record.aggressor_id)) {
        matched_id = record.aggressor_id;
    } else if (user_orders_.contains(record.passive_id)) {
        matched_id = record.passive_id;
    }
    if (matched_id == 0) return;

    auto& s = user_orders_[matched_id];
    s.filled_qty = qMin(s.filled_qty + record.quantity, s.ordered_qty);
    refresh_status_table();
}

void OrderEntry::refresh_status_table() {
    status_table_->setRowCount(static_cast<int>(order_keys_.size()));

    for (int row = 0; row < order_keys_.size(); ++row) {
        const auto& s = user_orders_[order_keys_[row]];

        bool filled   = s.filled_qty >= s.ordered_qty;
        bool partial  = s.filled_qty > 0 && !filled;
        QString status_icon = filled ? "✅" : (partial ? "⏳" : "📋");

        QColor side_color = (s.side == "BUY") ? QColor(0x89,0xb4,0xfa)
                                               : QColor(0xf3,0x8b,0xa8);

        auto make_item = [&](const QString& text, Qt::Alignment align = Qt::AlignRight | Qt::AlignVCenter) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            item->setFlags(Qt::ItemIsEnabled);
            return item;
        };

        status_table_->setItem(row, 0, make_item(QString::number(s.id)));
        auto* side_item = make_item(s.side, Qt::AlignCenter | Qt::AlignVCenter);
        side_item->setForeground(side_color);
        status_table_->setItem(row, 1, side_item);
        status_table_->setItem(row, 2, make_item(s.price > 0 ? QString::number(s.price) : "MKT"));
        status_table_->setItem(row, 3, make_item(QString::number(s.ordered_qty)));

        auto* filled_item = make_item(QString::number(s.filled_qty));
        if (filled)       filled_item->setForeground(QColor(0xa6,0xe3,0xa1));  // green
        else if (partial) filled_item->setForeground(QColor(0xf9,0xe2,0xaf));  // yellow
        status_table_->setItem(row, 4, filled_item);

        status_table_->setItem(row, 5, make_item(status_icon, Qt::AlignCenter | Qt::AlignVCenter));
    }
}

} // namespace lob_qt
