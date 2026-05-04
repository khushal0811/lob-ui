#pragma once
#include "bridge/bridge_types.hpp"
#include <QWidget>

class QTableView;
class QLabel;

namespace lob_qt {

class OrderBookModel;

class OrderBookPanel : public QWidget {
    Q_OBJECT

public:
    explicit OrderBookPanel(QWidget* parent = nullptr);

public slots:
    void update(const lob_qt::BookSnapshot& snapshot);

private:
    void setup_table(QTableView* view, bool is_bid);

    OrderBookModel* bid_model_    {nullptr};
    OrderBookModel* ask_model_    {nullptr};
    QTableView*     bid_view_     {nullptr};
    QTableView*     ask_view_     {nullptr};
    QLabel*         spread_label_ {nullptr};
    QLabel*         mid_label_    {nullptr};
};

} // namespace lob_qt
