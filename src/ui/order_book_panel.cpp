#include "ui/order_book_panel.hpp"
#include "models/order_book_model.hpp"
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

namespace lob_qt {

OrderBookPanel::OrderBookPanel(QWidget* parent) : QWidget(parent) {
    bid_model_ = new OrderBookModel(true,  this);
    ask_model_ = new OrderBookModel(false, this);
    bid_view_  = new QTableView(this);
    ask_view_  = new QTableView(this);

    bid_view_->setModel(bid_model_);
    ask_view_->setModel(ask_model_);

    setup_table(bid_view_);
    setup_table(ask_view_);

    spread_label_ = new QLabel("Spread: —", this);
    mid_label_    = new QLabel("Mid: —",    this);

    spread_label_->setStyleSheet("font-weight: bold; font-size: 13px; color: #a6e3a1;");
    mid_label_->setStyleSheet("font-weight: bold; font-size: 13px; color: #cdd6f4;");

    auto* info_layout = new QHBoxLayout;
    info_layout->addWidget(mid_label_);
    info_layout->addStretch();
    info_layout->addWidget(spread_label_);

    // Horizontal separator between ask and bid tables
    auto* sep = new QFrame(this);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("color: #45475a;");
    sep->setFixedHeight(2);

    auto* header = new QLabel("<b>Order Book</b>", this);
    header->setStyleSheet("font-size: 14px; font-weight: bold; color: #89b4fa;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(4);
    layout->addWidget(header);
    layout->addWidget(ask_view_);       // asks on top
    layout->addWidget(sep);             // separator
    layout->addLayout(info_layout);     // mid / spread
    layout->addWidget(bid_view_);       // bids below
    setLayout(layout);
}

void OrderBookPanel::setup_table(QTableView* view) {
    view->setSelectionMode(QAbstractItemView::NoSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setAlternatingRowColors(false);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    view->verticalHeader()->hide();
    view->setShowGrid(false);
    view->setFixedHeight(200);
    view->verticalHeader()->setDefaultSectionSize(20);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void OrderBookPanel::update(const lob_qt::BookSnapshot& snapshot) {
    bid_model_->update(snapshot.bids);
    ask_model_->update(snapshot.asks);

    // Scroll ask table to bottom so best ask (lowest price) sits closest to mid
    ask_view_->scrollToBottom();

    spread_label_->setText(QString("Spread: %1").arg(snapshot.spread));
    mid_label_->setText(QString("Mid: %1").arg(snapshot.mid_price));

    // Color spread: green <= 5, yellow if wider
    if (snapshot.spread > 5) {
        spread_label_->setStyleSheet("font-weight: bold; font-size: 13px; color: #f9e2af;");
    } else {
        spread_label_->setStyleSheet("font-weight: bold; font-size: 13px; color: #a6e3a1;");
    }
}

} // namespace lob_qt
