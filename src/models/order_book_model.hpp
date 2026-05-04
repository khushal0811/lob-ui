#pragma once
#include "bridge/bridge_types.hpp"
#include <QAbstractTableModel>
#include <QColor>

namespace lob_qt {

class OrderBookModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Price = 0, Volume, OrderCount, ColumnCount };

    explicit OrderBookModel(bool is_bid, QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = {}) const override;
    int      columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

public slots:
    void update(const QVector<lob_qt::LevelEntry>& levels);

private:
    QVector<LevelEntry> levels_;
    bool                is_bid_;
    QColor              row_color_;
};

} // namespace lob_qt
