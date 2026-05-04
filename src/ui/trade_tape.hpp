#pragma once
#include "bridge/bridge_types.hpp"
#include <QWidget>

class QTableView;

namespace lob_qt {

class TradeTapeModel;

class TradeTape : public QWidget {
    Q_OBJECT

public:
    explicit TradeTape(QWidget* parent = nullptr);

public slots:
    void add_trade(const lob_qt::TradeRecord& record);

private:
    TradeTapeModel* model_ {nullptr};
    QTableView*     view_  {nullptr};
};

} // namespace lob_qt
