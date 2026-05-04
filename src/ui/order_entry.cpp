#include "ui/order_entry.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
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

    auto* header = new QLabel("<b>Order Entry</b>", this);
    header->setStyleSheet("color: #89b4fa; font-size: 13px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(header);
    layout->addLayout(form);
    layout->addWidget(submit_btn_);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Cancel by ID:", this));
    layout->addLayout(cancel_layout);
    layout->addWidget(feedback_label_);
    layout->addStretch();
    setLayout(layout);

    connect(submit_btn_, &QPushButton::clicked,
            this, &OrderEntry::on_submit_clicked);
    connect(cancel_btn_, &QPushButton::clicked,
            this, &OrderEntry::on_cancel_clicked);
    connect(type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderEntry::on_type_changed);
}

void OrderEntry::on_type_changed(int index) {
    // Iceberg = 4, Stop = 2, StopLimit = 3, Market = 1
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
    if (type_idx != 1 && price_edit_->text().isEmpty()) {  // not market
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

} // namespace lob_qt
