#include "ui/order_book_panel.hpp"
#include "models/order_book_model.hpp"
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

    setup_table(bid_view_, true);
    setup_table(ask_view_, false);

    spread_label_ = new QLabel("Spread: —", this);
    mid_label_    = new QLabel("Mid: —",    this);

    auto* info_layout = new QHBoxLayout;
    info_layout->addWidget(mid_label_);
    info_layout->addStretch();
    info_layout->addWidget(spread_label_);

    auto* header = new QLabel("<b>Order Book</b>", this);
    header->setStyleSheet("color: #89b4fa; font-size: 13px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(header);
    layout->addWidget(ask_view_);       // asks on top (ascending)
    layout->addLayout(info_layout);
    layout->addWidget(bid_view_);       // bids on bottom (descending)
    setLayout(layout);
}

void OrderBookPanel::setup_table(QTableView* view, bool /*is_bid*/) {
    view->setSelectionMode(QAbstractItemView::NoSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setAlternatingRowColors(false);
    view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    view->verticalHeader()->hide();
    view->setShowGrid(false);
    view->setFixedHeight(200);
    view->verticalHeader()->setDefaultSectionSize(20);
}

void OrderBookPanel::update(const lob_qt::BookSnapshot& snapshot) {
    bid_model_->update(snapshot.bids);
    ask_model_->update(snapshot.asks);

    spread_label_->setText(QString("Spread: %1").arg(snapshot.spread));
    mid_label_->setText(QString("Mid: %1").arg(snapshot.mid_price));
}

} // namespace lob_qt
