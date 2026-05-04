#pragma once
#include "bridge/bridge_types.hpp"
#include "bridge/command.hpp"
#include <QWidget>
#include <QMap>

class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;
class QTableWidget;

namespace lob_qt {

// Tracks a single order the user has submitted
struct UserOrderStatus {
    quint64 id          {0};
    QString side;          // "BUY" / "SELL"
    QString type;          // "Limit" / "Market" etc.
    qint64  price       {0};   // 0 = market
    quint64 ordered_qty {0};
    quint64 filled_qty  {0};
};

class OrderEntry : public QWidget {
    Q_OBJECT

public:
    explicit OrderEntry(QWidget* parent = nullptr);

public slots:
    void show_rejection(const lob_qt::RejectionRecord& record);
    void show_accepted(quint64 order_id);
    void on_trade_update(const lob_qt::TradeRecord& record);  // connected in MainWindow

signals:
    void submit_order(lob_qt::SubmitOrderCommand cmd);
    void cancel_order(quint64 order_id);

private slots:
    void on_submit_clicked();
    void on_cancel_clicked();
    void on_type_changed(int index);

private:
    void setup_ui();
    bool validate_inputs();
    void refresh_status_table();

    QComboBox*    type_combo_    {nullptr};
    QComboBox*    side_combo_    {nullptr};
    QLineEdit*    price_edit_    {nullptr};
    QLineEdit*    qty_edit_      {nullptr};
    QLineEdit*    peak_edit_     {nullptr};   // iceberg only
    QLineEdit*    stop_edit_     {nullptr};   // stop/stop-limit only
    QPushButton*  submit_btn_    {nullptr};
    QLineEdit*    cancel_id_edit_{nullptr};
    QPushButton*  cancel_btn_    {nullptr};
    QLabel*       feedback_label_{nullptr};
    QTableWidget* status_table_  {nullptr};

    // Ordered insertion: newest first
    QList<quint64>                order_keys_;
    QMap<quint64, UserOrderStatus> user_orders_;

    static constexpr int kMaxTracked = 8;
    static quint64 next_id_;
};

} // namespace lob_qt
