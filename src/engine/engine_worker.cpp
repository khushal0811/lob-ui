#include "engine/engine_worker.hpp"
#include "engine/matching_engine.hpp"
#include "feed/synthetic_gen.hpp"
#include "feed/csv_replay.hpp"
#include "core/config.hpp"
#include "core/enums.hpp"
#include "core/order.hpp"
#include "core/event.hpp"
#include <QDateTime>
#include <algorithm>
#include <fstream>
#include <iterator>

namespace lob_qt {

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

static QString reject_reason_to_string(lob::RejectReason r) {
    switch (r) {
    case lob::RejectReason::InvalidPrice:          return "Invalid price";
    case lob::RejectReason::InvalidQuantity:       return "Invalid quantity";
    case lob::RejectReason::DuplicateOrderId:      return "Duplicate order ID";
    case lob::RejectReason::OrderNotFound:         return "Order not found";
    case lob::RejectReason::OrderNotCancellable:   return "Order not cancellable";
    case lob::RejectReason::InvalidModify:         return "Invalid modify";
    case lob::RejectReason::StopPriceInconsistent: return "Stop price inconsistent";
    case lob::RejectReason::PriceBandViolation:    return "Price band violation";
    case lob::RejectReason::MaxSizeViolation:      return "Max size violation";
    case lob::RejectReason::MarketExhausted:       return "Market exhausted";
    default:                                        return "Unknown";
    }
}

static void emit_trades(EngineWorker* self, const std::vector<lob::Trade>& trades) {
    for (const auto& trade : trades) {
        TradeRecord rec;
        rec.aggressor_id  = trade.aggressor_id;
        rec.passive_id    = trade.passive_id;
        rec.price         = trade.price;
        rec.quantity      = trade.quantity;
        rec.buy_aggressor = (trade.aggressor_side == lob::AggressorSide::Buy);
        rec.timestamp     = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        emit self->tradeExecuted(rec);
    }
}

// -------------------------------------------------------------------------
// Construction / destruction
// -------------------------------------------------------------------------

EngineWorker::EngineWorker(QObject* parent)
    : QObject(parent) {
    qRegisterMetaType<lob_qt::BookSnapshot>();
    qRegisterMetaType<lob_qt::TradeRecord>();
    qRegisterMetaType<lob_qt::MetricsSnapshot>();
    qRegisterMetaType<lob_qt::RejectionRecord>();

    init_engine();
    init_timers();
}

EngineWorker::~EngineWorker() = default;

void EngineWorker::init_engine() {
    lob::EngineConfig config;
    engine_ = std::make_unique<lob::MatchingEngine>(config);
}

void EngineWorker::init_timers() {
    generator_timer_ = new QTimer(this);
    replay_timer_    = new QTimer(this);
    metrics_timer_   = new QTimer(this);

    connect(generator_timer_, &QTimer::timeout, this, &EngineWorker::on_generator_tick);
    connect(replay_timer_,    &QTimer::timeout, this, &EngineWorker::on_replay_tick);
    connect(metrics_timer_,   &QTimer::timeout, this, &EngineWorker::on_metrics_tick);

    metrics_timer_->start(1000);
}

// -------------------------------------------------------------------------
// Part 3.2 — submitOrder and cancelOrder
// -------------------------------------------------------------------------

void EngineWorker::submitOrder(lob_qt::SubmitOrderCommand cmd) {
    lob::Order order;
    order.id         = cmd.id;
    order.side       = (cmd.side == SideUi::Buy) ? lob::Side::Buy : lob::Side::Sell;
    order.price      = cmd.price;
    order.stop_price = cmd.stop_price;
    order.quantity   = cmd.quantity;
    order.orig_qty   = cmd.quantity;
    order.peak_qty   = cmd.peak_qty;
    order.status     = lob::OrderStatus::New;

    switch (cmd.type) {
    case OrderTypeUi::Limit:     order.type = lob::OrderType::Limit;     break;
    case OrderTypeUi::Market:    order.type = lob::OrderType::Market;    break;
    case OrderTypeUi::Stop:      order.type = lob::OrderType::Stop;      break;
    case OrderTypeUi::StopLimit: order.type = lob::OrderType::StopLimit; break;
    case OrderTypeUi::Iceberg:   order.type = lob::OrderType::Iceberg;   break;
    }

    auto result = engine_->submit(lob::NewOrderEvent{order});
    ++event_count_;

    emit_trades(this, result.trades);

    for (const auto& rej : result.rejections) {
        RejectionRecord r;
        r.order_id = rej.order_id;
        r.reason   = reject_reason_to_string(rej.reason);
        emit orderRejected(r);
    }

    emit_book_snapshot();
}

void EngineWorker::cancelOrder(quint64 order_id) {
    lob::CancelOrderEvent ev;
    ev.order_id = order_id;
    engine_->submit(ev);
    ++event_count_;
    emit_book_snapshot();
}

// -------------------------------------------------------------------------
// Part 3.4 — loadCsv and startReplay (with pre-counted total)
// -------------------------------------------------------------------------

void EngineWorker::loadCsv(QString path) {
    replay_timer_->stop();
    replay_reader_.reset();

    // Pre-count lines so we can show "Event N / total"
    {
        std::ifstream counter(path.toStdString());
        replay_total_ = static_cast<int>(
            std::count(std::istreambuf_iterator<char>(counter),
                       std::istreambuf_iterator<char>(), '\n'));
    }

    replay_reader_  = std::make_unique<lob::CsvReplayReader>(path.toStdString());
    replay_current_ = 0;
    emit replayProgress(0, replay_total_);
}

void EngineWorker::startReplay(double speed_multiplier) {
    if (!replay_reader_) return;
    // Base: 100 events/sec = 10ms interval; speed scales it
    int interval_ms = std::max(1, static_cast<int>(10.0 / speed_multiplier));
    replay_timer_->start(interval_ms);
}

void EngineWorker::on_replay_tick() {
    if (!replay_reader_) return;

    lob::OrderEvent ev;
    if (!replay_reader_->next(ev)) {
        replay_timer_->stop();
        emit replayFinished();
        return;
    }

    auto result = engine_->submit(ev);
    ++event_count_;
    ++replay_current_;

    emit_trades(this, result.trades);
    emit replayProgress(replay_current_, replay_total_);
    emit_book_snapshot();
}

// -------------------------------------------------------------------------
// Phase 2.5 — startScenario / stopScenario / on_generator_tick
// -------------------------------------------------------------------------

void EngineWorker::startScenario(QString scenario_name) {
    generator_timer_->stop();
    generator_.reset();

    lob::EngineConfig config;
    config.mid_price = 10000;

    if (scenario_name == "small") {
        config.arrival_rate       = 100.0;
        config.cancel_probability = 0.1;
        config.price_std_dev      = 5.0;
    } else if (scenario_name == "large") {
        config.arrival_rate       = 5000.0;
        config.cancel_probability = 0.3;
        config.price_std_dev      = 20.0;
    } else if (scenario_name == "high_cancel") {
        config.arrival_rate       = 1000.0;
        config.cancel_probability = 0.7;
        config.price_std_dev      = 20.0;
    } else if (scenario_name == "market_heavy") {
        config.arrival_rate       = 1000.0;
        config.cancel_probability = 0.2;
        config.price_std_dev      = 30.0;
        config.market_maker_mode  = true;
    } else if (scenario_name == "iceberg_stop") {
        config.arrival_rate       = 500.0;
        config.cancel_probability = 0.2;
        config.price_std_dev      = 15.0;
    } else {
        // Default: "medium"
        config.arrival_rate       = 1000.0;
        config.cancel_probability = 0.3;
        config.price_std_dev      = 20.0;
    }

    generator_ = std::make_unique<lob::SyntheticGenerator>(config, 42);
    generator_timer_->start(1);
}

void EngineWorker::stopScenario() {
    generator_timer_->stop();
    generator_.reset();
}

void EngineWorker::on_generator_tick() {
    if (!generator_) return;

    for (int i = 0; i < 10; ++i) {
        lob::OrderEvent ev;
        if (!generator_->next(ev)) {
            generator_timer_->stop();
            break;
        }
        auto result = engine_->submit(ev);
        ++event_count_;
        emit_trades(this, result.trades);
    }

    emit_book_snapshot();
}

void EngineWorker::pauseReplay()  { replay_timer_->stop(); }
void EngineWorker::stepReplay()   { on_replay_tick(); }

void EngineWorker::resetEngine() {
    generator_timer_->stop();
    replay_timer_->stop();
    generator_.reset();
    replay_reader_.reset();
    replay_current_ = 0;
    replay_total_   = 0;
    event_count_    = 0;
    last_event_count_ = 0;
    init_engine();
    emit_book_snapshot();
}

// -------------------------------------------------------------------------
// Metrics
// -------------------------------------------------------------------------

void EngineWorker::on_metrics_tick() {
    MetricsSnapshot snap;
    quint64 delta       = event_count_ - last_event_count_;
    snap.throughput_eps = static_cast<double>(delta);
    last_event_count_   = event_count_;
    emit metricsUpdated(snap);
}

// -------------------------------------------------------------------------
// Book snapshot
// -------------------------------------------------------------------------

void EngineWorker::emit_book_snapshot() {
    BookSnapshot snap;
    const auto& book = engine_->book();

    int count = 0;
    for (const auto& [price, level] : book.bids()) {
        if (count++ >= 10) break;
        LevelEntry e;
        e.price  = price;
        e.volume = level.total_volume();
        e.count  = static_cast<int>(level.size());
        snap.bids.push_back(e);
    }
    count = 0;
    for (const auto& [price, level] : book.asks()) {
        if (count++ >= 10) break;
        LevelEntry e;
        e.price  = price;
        e.volume = level.total_volume();
        e.count  = static_cast<int>(level.size());
        snap.asks.push_back(e);
    }

    if (auto bb = book.best_bid())  snap.best_bid   = *bb;
    if (auto ba = book.best_ask())  snap.best_ask   = *ba;
    if (auto mp = book.mid_price()) snap.mid_price  = *mp;
    if (auto sp = book.spread())    snap.spread     = *sp;
    snap.last_trade  = book.last_trade_price;
    snap.order_count = static_cast<quint64>(book.order_count());

    emit bookUpdated(snap);
}

void EngineWorker::emit_metrics_snapshot() {
    // Phase 4
}

} // namespace lob_qt
