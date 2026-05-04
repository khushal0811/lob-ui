#pragma once
#include "bridge/bridge_types.hpp"
#include "bridge/command.hpp"
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

// Forward declare engine types — keeps Qt out of engine headers
namespace lob {
class MatchingEngine;
class SyntheticGenerator;
class CsvReplayReader;
} // namespace lob

namespace lob_qt {

class EngineWorker : public QObject {
    Q_OBJECT

public:
    explicit EngineWorker(QObject* parent = nullptr);
    ~EngineWorker() override;

public slots:
    // Called from UI thread — executes on worker thread via QueuedConnection
    void submitOrder(lob_qt::SubmitOrderCommand cmd);
    void cancelOrder(quint64 order_id);
    void loadCsv(QString path);
    void startReplay(double speed_multiplier);
    void pauseReplay();
    void stepReplay();
    void resetEngine();
    void startScenario(QString scenario_name);
    void stopScenario();

    // Internal slots — called by QTimers on worker thread
    void on_generator_tick();
    void on_replay_tick();
    void on_metrics_tick();

signals:
    void bookUpdated(lob_qt::BookSnapshot snapshot);
    void tradeExecuted(lob_qt::TradeRecord record);
    void metricsUpdated(lob_qt::MetricsSnapshot snapshot);
    void orderRejected(lob_qt::RejectionRecord record);
    void replayFinished();
    void replayProgress(int current, int total);

private:
    std::unique_ptr<lob::MatchingEngine>    engine_;
    std::unique_ptr<lob::SyntheticGenerator> generator_;
    std::unique_ptr<lob::CsvReplayReader>   replay_reader_;

    QTimer* generator_timer_ {nullptr};
    QTimer* replay_timer_    {nullptr};
    QTimer* metrics_timer_   {nullptr};

    quint64 next_order_id_    {1};
    quint64 event_count_      {0};
    quint64 last_event_count_ {0};  // for throughput calculation
    int     replay_total_     {0};
    int     replay_current_   {0};

    void init_engine();
    void init_timers();
    void emit_book_snapshot();

    // Latency tracking for on_metrics_tick()
    quint64 trade_count_      {0};
    quint64 cancel_count_     {0};
    quint64 last_trade_count_ {0};
    quint64 last_cancel_count_{0};
    quint64 lat_min_ns_       {UINT64_MAX};
    quint64 lat_max_ns_       {0};
    quint64 lat_sum_ns_       {0};
    quint64 lat_count_        {0};
};

} // namespace lob_qt
