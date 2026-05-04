#pragma once
#include "bridge/bridge_types.hpp"
#include <QWidget>

class QLabel;

namespace lob_qt {

class MetricsPanel : public QWidget {
    Q_OBJECT

public:
    explicit MetricsPanel(QWidget* parent = nullptr);

public slots:
    void update(const lob_qt::MetricsSnapshot& snap);
    void update_book_info(const lob_qt::BookSnapshot& snap);

private:
    QLabel* throughput_label_  {nullptr};
    QLabel* p50_label_         {nullptr};
    QLabel* p95_label_         {nullptr};
    QLabel* p99_label_         {nullptr};
    QLabel* max_label_         {nullptr};
    QLabel* fill_rate_label_   {nullptr};
    QLabel* cancel_rate_label_ {nullptr};
    QLabel* best_bid_label_    {nullptr};
    QLabel* best_ask_label_    {nullptr};
    QLabel* spread_label_      {nullptr};
    QLabel* last_trade_label_  {nullptr};
    QLabel* order_count_label_ {nullptr};
};

} // namespace lob_qt
