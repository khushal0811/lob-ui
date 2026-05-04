#include "models/trade_tape_model.hpp"
#include <QColor>

namespace lob_qt {

TradeTapeModel::TradeTapeModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int TradeTapeModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(trades_.size());
}

int TradeTapeModel::columnCount(const QModelIndex&) const {
    return ColumnCount;
}

QVariant TradeTapeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= trades_.size()) return {};
    const auto& t = trades_[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Time:     return t.timestamp;
        case Side:     return t.buy_aggressor ? "BUY" : "SELL";
        case Price:    return QString::number(t.price);
        case Quantity: return QString::number(t.quantity);
        default:       return {};
        }
    }

    if (role == Qt::ForegroundRole) {
        // Blue for buy-initiated, orange for sell-initiated
        return t.buy_aggressor ? QColor(0x89, 0xb4, 0xfa)
                               : QColor(0xfe, 0x8c, 0x02);
    }

    if (role == Qt::TextAlignmentRole) {
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    }

    return {};
}

QVariant TradeTapeModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case Time:     return "Time";
    case Side:     return "Side";
    case Price:    return "Price";
    case Quantity: return "Qty";
    default:       return {};
    }
}

void TradeTapeModel::add_trade(const lob_qt::TradeRecord& record) {
    beginInsertRows({}, 0, 0);
    trades_.prepend(record);
    endInsertRows();

    if (trades_.size() > kMaxRows) {
        beginRemoveRows({}, kMaxRows, trades_.size() - 1);
        trades_.resize(kMaxRows);
        endRemoveRows();
    }
}

} // namespace lob_qt
