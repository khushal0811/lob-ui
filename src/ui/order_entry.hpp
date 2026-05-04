#pragma once
#include "bridge/bridge_types.hpp"
#include "bridge/command.hpp"
#include <QWidget>

class QComboBox;
class QLineEdit;
class QPushButton;
class QLabel;

namespace lob_qt {

class OrderEntry : public QWidget {
    Q_OBJECT

public:
    explicit OrderEntry(QWidget* parent = nullptr);

public slots:
    void show_rejection(const lob_qt::RejectionRecord& record);
    void show_accepted(quint64 order_id);

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

    QComboBox*   type_combo_    {nullptr};
    QComboBox*   side_combo_    {nullptr};
    QLineEdit*   price_edit_    {nullptr};
    QLineEdit*   qty_edit_      {nullptr};
    QLineEdit*   peak_edit_     {nullptr};   // iceberg only
    QLineEdit*   stop_edit_     {nullptr};   // stop/stop-limit only
    QPushButton* submit_btn_    {nullptr};
    QLineEdit*   cancel_id_edit_{nullptr};
    QPushButton* cancel_btn_    {nullptr};
    QLabel*      feedback_label_{nullptr};

    static quint64 next_id_;
};

} // namespace lob_qt
