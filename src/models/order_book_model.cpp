#include "models/order_book_model.hpp"

namespace lob_qt {

OrderBookModel::OrderBookModel(bool is_bid, QObject* parent)
    : QAbstractTableModel(parent)
    , is_bid_(is_bid)
    , row_color_(is_bid ? QColor(40, 80, 40) : QColor(80, 40, 40)) {}

int OrderBookModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(levels_.size());
}

int OrderBookModel::columnCount(const QModelIndex&) const {
    return ColumnCount;
}

QVariant OrderBookModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= levels_.size()) return {};

    const auto& entry = levels_[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Price:      return QString::number(entry.price);
        case Volume:     return QString::number(entry.volume);
        case OrderCount: return QString::number(entry.count);
        default:         return {};
        }
    }

    if (role == Qt::BackgroundRole) {
        // Highlight best level (row 0) with brighter colour
        if (index.row() == 0) {
            return is_bid_ ? QColor(30, 160, 60) : QColor(180, 40, 40);
        }
        return row_color_;
    }

    if (role == Qt::ForegroundRole) {
        return QColor(0xcd, 0xd6, 0xf4);  // Catppuccin text
    }

    if (role == Qt::TextAlignmentRole) {
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    }

    return {};
}

QVariant OrderBookModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case Price:      return "Price";
    case Volume:     return "Volume";
    case OrderCount: return "Orders";
    default:         return {};
    }
}

void OrderBookModel::update(const QVector<lob_qt::LevelEntry>& levels) {
    beginResetModel();
    levels_ = levels;
    endResetModel();
}

} // namespace lob_qt
