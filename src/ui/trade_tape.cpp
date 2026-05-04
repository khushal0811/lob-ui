#include "ui/trade_tape.hpp"
#include "models/trade_tape_model.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

namespace lob_qt {

TradeTape::TradeTape(QWidget* parent) : QWidget(parent) {
    model_ = new TradeTapeModel(this);
    view_  = new QTableView(this);
    view_->setModel(model_);
    view_->setSelectionMode(QAbstractItemView::NoSelection);
    view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    view_->verticalHeader()->hide();
    view_->setShowGrid(false);
    view_->verticalHeader()->setDefaultSectionSize(20);
    // Step 8: alternating rows for readability
    view_->setAlternatingRowColors(true);

    auto* header = new QLabel("<b>Trade Tape</b>", this);
    // Step 8: match order book header styling exactly
    header->setStyleSheet("font-size: 14px; font-weight: bold; color: #89b4fa;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(header);
    layout->addWidget(view_);
    setLayout(layout);
}

void TradeTape::add_trade(const lob_qt::TradeRecord& record) {
    model_->add_trade(record);
    view_->scrollToTop();   // newest trades always visible at top
}

} // namespace lob_qt
