#pragma once
#include "bridge/bridge_types.hpp"
#include <QAbstractTableModel>

namespace lob_qt {

class TradeTapeModel : public QAbstractTableModel {
    Q_OBJECT

public:
    static constexpr int kMaxRows = 500;
    enum Column { Time = 0, Side, Price, Quantity, ColumnCount };

    explicit TradeTapeModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = {}) const override;
    int      columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

public slots:
    void add_trade(const lob_qt::TradeRecord& record);

private:
    QVector<TradeRecord> trades_;
};

} // namespace lob_qt
