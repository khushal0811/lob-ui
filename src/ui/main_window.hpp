#pragma once
#include <QMainWindow>

class QThread;

namespace lob_qt {

class EngineWorker;
class OrderBookPanel;
class TradeTape;
class OrderEntry;
class ReplayControls;
class MetricsPanel;
class SpreadChart;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setup_worker();
    void setup_ui();
    void setup_connections();

    // Worker
    EngineWorker* worker_        {nullptr};
    QThread*      worker_thread_ {nullptr};

    // Panels
    OrderBookPanel* book_panel_    {nullptr};
    TradeTape*      trade_tape_    {nullptr};
    OrderEntry*     order_entry_   {nullptr};
    ReplayControls* replay_ctrl_   {nullptr};
    MetricsPanel*   metrics_panel_ {nullptr};
    SpreadChart*    spread_chart_  {nullptr};
};

} // namespace lob_qt
