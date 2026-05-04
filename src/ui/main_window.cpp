#include "ui/main_window.hpp"
#include "engine/engine_worker.hpp"
#include "ui/order_book_panel.hpp"
#include "ui/trade_tape.hpp"
#include "ui/order_entry.hpp"
#include "ui/replay_controls.hpp"
#include "ui/metrics_panel.hpp"
#include "ui/spread_chart.hpp"
#include "ui/welcome_overlay.hpp"
#include <QShowEvent>
#include <QSplitter>
#include <QThread>
#include <QVBoxLayout>
#include <QWidget>

namespace lob_qt {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle("lob-engine — Order Book Visualizer");
    setMinimumSize(1280, 800);

    setup_worker();
    setup_ui();
    setup_connections();
}

MainWindow::~MainWindow() {
    worker_thread_->quit();
    worker_thread_->wait();
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    // Show overlay on first display, covering the full window
    if (overlay_) {
        overlay_->setGeometry(rect());
        overlay_->show();
        overlay_->raise();
    }
}

void MainWindow::setup_worker() {
    worker_        = new EngineWorker();
    worker_thread_ = new QThread(this);

    worker_->moveToThread(worker_thread_);

    connect(worker_thread_, &QThread::finished,
            worker_,        &QObject::deleteLater);

    worker_thread_->start();
}

void MainWindow::setup_ui() {
    book_panel_    = new OrderBookPanel(this);
    trade_tape_    = new TradeTape(this);
    order_entry_   = new OrderEntry(this);
    replay_ctrl_   = new ReplayControls(this);
    metrics_panel_ = new MetricsPanel(this);
    spread_chart_  = new SpreadChart(this);

    // Left column: order book (top) + trade tape (bottom)
    auto* left_widget = new QWidget(this);
    auto* left_layout = new QVBoxLayout(left_widget);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->setSpacing(4);
    left_layout->addWidget(book_panel_, 2);
    left_layout->addWidget(trade_tape_, 3);

    // Right column: order entry + replay controls + metrics + spread chart
    auto* right_widget = new QWidget(this);
    right_widget->setMinimumWidth(440);
    auto* right_layout = new QVBoxLayout(right_widget);
    right_layout->setContentsMargins(4, 4, 4, 4);
    right_layout->setSpacing(4);
    right_layout->addWidget(order_entry_);
    right_layout->addWidget(replay_ctrl_);
    right_layout->addWidget(metrics_panel_);
    right_layout->addWidget(spread_chart_, 1);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(left_widget);
    splitter->addWidget(right_widget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({820, 460});

    setCentralWidget(splitter);

    // Overlay: created last so it sits on top; no parent layout
    overlay_ = new WelcomeOverlay(this);
}

void MainWindow::setup_connections() {
    // ---- Engine → UI (QueuedConnection — cross-thread) ----

    connect(worker_, &EngineWorker::bookUpdated,
            book_panel_, &OrderBookPanel::update,
            Qt::QueuedConnection);

    connect(worker_, &EngineWorker::tradeExecuted,
            trade_tape_, &TradeTape::add_trade,
            Qt::QueuedConnection);

    connect(worker_, &EngineWorker::orderRejected,
            order_entry_, &OrderEntry::show_rejection,
            Qt::QueuedConnection);

    connect(worker_, &EngineWorker::replayFinished,
            replay_ctrl_, &ReplayControls::on_replay_finished,
            Qt::QueuedConnection);

    connect(worker_, &EngineWorker::replayProgress,
            replay_ctrl_, &ReplayControls::update_progress,
            Qt::QueuedConnection);

    connect(worker_, &EngineWorker::bookUpdated,
            metrics_panel_, &MetricsPanel::update_book_info,
            Qt::QueuedConnection);

    connect(worker_, &EngineWorker::bookUpdated,
            this, [this](const lob_qt::BookSnapshot& snap) {
                spread_chart_->add_point(snap.spread);
            }, Qt::QueuedConnection);

    connect(worker_, &EngineWorker::metricsUpdated,
            metrics_panel_, &MetricsPanel::update,
            Qt::QueuedConnection);

    // ---- UI → Engine (AutoConnection resolves to QueuedConnection) ----

    connect(order_entry_, &OrderEntry::submit_order,
            worker_, &EngineWorker::submitOrder);

    connect(order_entry_, &OrderEntry::cancel_order,
            worker_, &EngineWorker::cancelOrder);

    connect(replay_ctrl_, &ReplayControls::load_csv,
            worker_, &EngineWorker::loadCsv);

    connect(replay_ctrl_, &ReplayControls::start_replay,
            worker_, &EngineWorker::startReplay);

    connect(replay_ctrl_, &ReplayControls::pause_replay,
            worker_, &EngineWorker::pauseReplay);

    connect(replay_ctrl_, &ReplayControls::step_replay,
            worker_, &EngineWorker::stepReplay);

    connect(replay_ctrl_, &ReplayControls::reset_engine,
            worker_, &EngineWorker::resetEngine);

    connect(replay_ctrl_, &ReplayControls::start_scenario,
            worker_, &EngineWorker::startScenario);

    connect(replay_ctrl_, &ReplayControls::stop_scenario,
            worker_, &EngineWorker::stopScenario);

    // Overlay start button → engine starts medium scenario (Step 4: no auto-start)
    connect(overlay_, &WelcomeOverlay::start_requested,
            worker_, [this] {
                QMetaObject::invokeMethod(worker_, "startScenario",
                                          Qt::QueuedConnection,
                                          Q_ARG(QString, "medium"));
            });
}

} // namespace lob_qt
