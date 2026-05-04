# Architecture — lob-qt
# Qt 6 desktop visualization and control layer

---

## Design philosophy

1. **The engine does not know Qt exists.** `lob-engine` is a pure C++ static library. No Qt headers, no Qt types, no Qt dependencies anywhere in `src/`. The UI links against the engine — not the other way around.

2. **The UI thread never touches engine state.** All engine operations happen on the worker thread. All widget updates happen on the UI thread. The bridge is the only point of contact between the two worlds.

3. **Qt signals/slots are the communication protocol.** Cross-thread signals with `Qt::QueuedConnection` are thread-safe by design. No manual mutex locking required for UI updates.

---

## Top-level data flow

```
+--------------------------------------------------+
|              Qt Main Thread (UI)                 |
|                                                  |
|  +-----------+  +-----------+  +-------------+  |
|  | OrderBook |  |  Trade    |  |   Metrics   |  |
|  |   Panel   |  |   Tape   |  |    Panel    |  |
|  +-----------+  +-----------+  +-------------+  |
|  +-----------+  +-----------+  +-------------+  |
|  |   Order   |  |  Replay   |  |   Spread    |  |
|  |   Entry   |  | Controls  |  |    Chart    |  |
|  +-----------+  +-----------+  +-------------+  |
|                                                  |
|              MainWindow                          |
+--------------------+-----------------------------+
                     |
                     |  Qt signals (QueuedConnection)
                     |  commands via invokeMethod
                     v
+--------------------------------------------------+
|              EngineWorker (QObject)              |
|         runs on dedicated QThread               |
|                                                  |
|  Owns:                                           |
|  - MatchingEngine                                |
|  - SyntheticGenerator                           |
|  - CsvReplayReader                               |
|  - QTimer (replay tick, metrics tick)           |
|                                                  |
|  Emits signals:                                  |
|  - bookUpdated(BookSnapshot)                     |
|  - tradeExecuted(TradeRecord)                    |
|  - metricsUpdated(MetricsSnapshot)               |
|  - orderRejected(RejectionRecord)                |
|  - replayFinished()                              |
+--------------------------------------------------+
                     |
                     |  direct method calls
                     v
+--------------------------------------------------+
|         lob-engine (liblob_core.a)               |
|  MatchingEngine, OrderBook, SyntheticGenerator   |
|  CsvReplayReader, Metrics, Snapshot              |
+--------------------------------------------------+
```

---

## Directory structure

```
lob-qt/
├── src/
│   ├── main.cpp                    # QApplication entry point
│   ├── engine/
│   │   ├── engine_worker.hpp       # QObject wrapper for matching engine
│   │   └── engine_worker.cpp
│   ├── bridge/
│   │   ├── bridge_types.hpp        # Qt-friendly snapshot structs (no lob:: types in UI)
│   │   └── command.hpp             # Command objects sent from UI to engine
│   ├── ui/
│   │   ├── main_window.hpp         # Top-level QMainWindow
│   │   ├── main_window.cpp
│   │   ├── order_book_panel.hpp    # Bid/ask table widget
│   │   ├── order_book_panel.cpp
│   │   ├── trade_tape.hpp          # Scrolling trade list
│   │   ├── trade_tape.cpp
│   │   ├── order_entry.hpp         # Order submission form
│   │   ├── order_entry.cpp
│   │   ├── replay_controls.hpp     # Play/pause/step/reset
│   │   ├── replay_controls.cpp
│   │   ├── metrics_panel.hpp       # Latency, throughput, rates
│   │   ├── metrics_panel.cpp
│   │   ├── spread_chart.hpp        # Rolling spread line chart
│   │   └── spread_chart.cpp
│   └── models/
│       ├── order_book_model.hpp    # QAbstractTableModel for bid/ask tables
│       ├── order_book_model.cpp
│       ├── trade_tape_model.hpp    # QAbstractTableModel for trade tape
│       └── trade_tape_model.cpp
├── CMakeLists.txt
├── .clang-format                   # Same as lob-engine
├── .gitignore
└── README.md
```

---

## Module responsibilities

### `src/engine/engine_worker.hpp/.cpp` — the bridge

This is the most important class in the project. It is a `QObject` that lives on a `QThread`. It owns the matching engine and all feed sources. It receives commands from the UI and emits signals back to the UI.

```cpp
class EngineWorker : public QObject {
    Q_OBJECT
public:
    explicit EngineWorker(QObject* parent = nullptr);

public slots:
    void submitOrder(lob::Order order);
    void cancelOrder(lob::OrderId id);
    void loadCsv(QString path);
    void startReplay(double speed_multiplier);
    void pauseReplay();
    void stepReplay();
    void resetEngine();
    void startScenario(QString scenario_name);
    void stopScenario();

signals:
    void bookUpdated(BookSnapshot snapshot);
    void tradeExecuted(TradeRecord record);
    void metricsUpdated(MetricsSnapshot snapshot);
    void orderRejected(RejectionRecord record);
    void replayFinished();
    void replayProgress(int current, int total);

private:
    lob::MatchingEngine engine_;
    lob::CsvReplayReader* replay_reader_ {nullptr};
    lob::SyntheticGenerator* generator_ {nullptr};
    QTimer* replay_timer_ {nullptr};
    QTimer* metrics_timer_ {nullptr};
    QTimer* generator_timer_ {nullptr};

    void emit_book_snapshot();
    void emit_metrics_snapshot();
    void process_result(const lob::MatchResult& result);
};
```

**Key rule:** `EngineWorker` is constructed on the main thread but immediately moved to the worker thread via `moveToThread()`. After `moveToThread()`, all slot invocations happen on the worker thread automatically.

```cpp
// In MainWindow constructor:
worker_ = new EngineWorker();
worker_thread_ = new QThread(this);
worker_->moveToThread(worker_thread_);
worker_thread_->start();
```

---

### `src/bridge/bridge_types.hpp` — Qt-friendly data types

The UI never uses `lob::` types directly. Bridge types are plain Qt structs that can cross thread boundaries safely via signals.

```cpp
struct LevelEntry {
    qint64  price    {0};
    quint64 volume   {0};
    int     count    {0};
};

struct BookSnapshot {
    QVector<LevelEntry> bids;  // top 10, descending
    QVector<LevelEntry> asks;  // top 10, ascending
    qint64  best_bid      {0};
    qint64  best_ask      {0};
    qint64  mid_price     {0};
    qint64  spread        {0};
    qint64  last_trade    {0};
    quint64 order_count   {0};
};

struct TradeRecord {
    quint64 aggressor_id  {0};
    quint64 passive_id    {0};
    qint64  price         {0};
    quint64 quantity      {0};
    bool    buy_aggressor {true};
    QString timestamp;
};

struct MetricsSnapshot {
    double   throughput_eps  {0.0};  // events per second
    quint64  p50_ns          {0};
    quint64  p95_ns          {0};
    quint64  p99_ns          {0};
    quint64  max_ns          {0};
    double   fill_rate       {0.0};
    double   cancel_rate     {0.0};
    double   rejection_rate  {0.0};
};

struct RejectionRecord {
    quint64 order_id {0};
    QString reason;
};
```

All bridge types must be registered with `qRegisterMetaType<T>()` before use in signals/slots across threads.

---

### `src/models/order_book_model.hpp/.cpp` — QAbstractTableModel

The bid and ask tables use a proper Qt model rather than direct widget manipulation. This separates data from presentation and makes the view update efficiently.

```cpp
class OrderBookModel : public QAbstractTableModel {
    Q_OBJECT
public:
    enum Column { Price = 0, Volume, Count, ColumnCount };

    explicit OrderBookModel(bool is_bid, QObject* parent = nullptr);

    int rowCount(const QModelIndex&) const override;
    int columnCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation, int role) const override;

public slots:
    void update(const QVector<LevelEntry>& levels);

private:
    QVector<LevelEntry> levels_;
    bool is_bid_;
};
```

`update()` is called when `bookUpdated` signal is received. It calls `beginResetModel()` / `endResetModel()` which triggers the view to repaint.

---

### `src/models/trade_tape_model.hpp/.cpp` — QAbstractTableModel

```cpp
class TradeTapeModel : public QAbstractTableModel {
    Q_OBJECT
public:
    static constexpr int kMaxRows = 500;
    enum Column { Time = 0, Side, Price, Quantity, ColumnCount };

    int rowCount(const QModelIndex&) const override;
    int columnCount(const QModelIndex&) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation, int role) const override;

public slots:
    void addTrade(const TradeRecord& record);

private:
    QVector<TradeRecord> trades_;
};
```

`addTrade()` prepends the new trade at index 0. When size exceeds `kMaxRows`, removes the last entry. Uses `beginInsertRows` / `endInsertRows` for efficient view update.

---

### `src/ui/main_window.hpp/.cpp` — top-level window

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void on_book_updated(const BookSnapshot& snapshot);
    void on_trade_executed(const TradeRecord& record);
    void on_metrics_updated(const MetricsSnapshot& snapshot);
    void on_order_rejected(const RejectionRecord& record);
    void on_replay_finished();
    void on_submit_order();
    void on_cancel_order();
    void on_load_csv();
    void on_play_replay();
    void on_pause_replay();
    void on_step_replay();
    void on_reset_engine();
    void on_run_scenario();
    void on_stop_scenario();

private:
    void setup_ui();
    void setup_connections();
    void setup_worker();

    // Worker
    EngineWorker* worker_       {nullptr};
    QThread*      worker_thread_{nullptr};

    // UI panels
    OrderBookPanel*  book_panel_    {nullptr};
    TradeTape*       trade_tape_    {nullptr};
    OrderEntry*      order_entry_   {nullptr};
    ReplayControls*  replay_ctrl_   {nullptr};
    MetricsPanel*    metrics_panel_ {nullptr};
    SpreadChart*     spread_chart_  {nullptr};
};
```

Layout: `QSplitter` splits left/right. Left side: order book + trade tape stacked vertically. Right side: order entry + replay controls + metrics + spread chart stacked vertically.

---

### `src/ui/spread_chart.hpp/.cpp` — QtCharts line chart

```cpp
class SpreadChart : public QWidget {
    Q_OBJECT
public:
    explicit SpreadChart(QWidget* parent = nullptr);

public slots:
    void add_point(qint64 spread_ticks);

private:
    static constexpr int kMaxPoints = 120;  // 60 seconds at 0.5s intervals
    QChart*      chart_  {nullptr};
    QLineSeries* series_ {nullptr};
    QChartView*  view_   {nullptr};
    qint64       x_      {0};
};
```

---

## CMake structure

```cmake
cmake_minimum_required(VERSION 3.20)
project(lob_qt CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)

# Import lob_core from sibling project
add_subdirectory(../lob-engine ${CMAKE_BINARY_DIR}/lob-engine EXCLUDE_FROM_ALL)

add_executable(lob_qt
    src/main.cpp
    src/engine/engine_worker.cpp
    src/ui/main_window.cpp
    src/ui/order_book_panel.cpp
    src/ui/trade_tape.cpp
    src/ui/order_entry.cpp
    src/ui/replay_controls.cpp
    src/ui/metrics_panel.cpp
    src/ui/spread_chart.cpp
    src/models/order_book_model.cpp
    src/models/trade_tape_model.cpp
)

target_include_directories(lob_qt PRIVATE src/ ../lob-engine/src/)
target_link_libraries(lob_qt PRIVATE lob_core Qt6::Core Qt6::Widgets Qt6::Charts)
```

---

## Signal/slot connection map

All connections use `Qt::QueuedConnection` for cross-thread signals.

| Signal (worker thread) | Slot (UI thread) | Connection type |
|---|---|---|
| `worker::bookUpdated` | `book_panel_::update` | QueuedConnection |
| `worker::bookUpdated` | `spread_chart_::add_point` | QueuedConnection |
| `worker::tradeExecuted` | `trade_tape_::addTrade` | QueuedConnection |
| `worker::metricsUpdated` | `metrics_panel_::update` | QueuedConnection |
| `worker::orderRejected` | `order_entry_::show_rejection` | QueuedConnection |
| `worker::replayFinished` | `replay_ctrl_::on_finished` | QueuedConnection |
| `worker::replayProgress` | `replay_ctrl_::update_counter` | QueuedConnection |

UI → Worker (AutoConnection resolves to QueuedConnection since worker is on different thread):

| UI action | Worker slot called |
|---|---|
| Submit order button | `worker::submitOrder(Order)` |
| Cancel button | `worker::cancelOrder(OrderId)` |
| Load CSV button | `worker::loadCsv(path)` |
| Play button | `worker::startReplay(speed)` |
| Pause button | `worker::pauseReplay()` |
| Step button | `worker::stepReplay()` |
| Reset button | `worker::resetEngine()` |
| Run scenario button | `worker::startScenario(name)` |
| Stop scenario button | `worker::stopScenario()` |

---

## Replay timing mechanism

The replay engine uses a `QTimer` on the worker thread to pace event delivery:

```
replay_timer_ = new QTimer(this);  // 'this' is EngineWorker on worker thread
connect(replay_timer_, &QTimer::timeout, this, &EngineWorker::on_replay_tick);
replay_timer_->setInterval(interval_ms);  // interval = (1000 / events_per_sec) * speed_factor
replay_timer_->start();
```

On each tick, one event is read from the CSV reader, submitted to the engine, and the result is emitted as signals. This gives the Step button a natural implementation: call `on_replay_tick()` once manually.

---

## Key design decisions and rationale

| Decision | Rationale |
|---|---|
| `moveToThread()` on EngineWorker | Correct Qt pattern for worker objects. All slot calls execute on the worker thread automatically. No manual thread management. |
| `Qt::QueuedConnection` for all cross-thread signals | Thread-safe by Qt's event loop. No mutex needed for UI updates. |
| Bridge types separate from `lob::` types | UI code never depends on engine headers. Decouples UI from engine changes. |
| `QAbstractTableModel` for tables | Efficient partial updates. Separates data from presentation. Standard Qt pattern. |
| `QTimer` for replay pacing | Integrates naturally with Qt event loop on worker thread. Pause is `timer->stop()`, resume is `timer->start()`. |
| Top 10 levels only in book snapshot | Sufficient for visualization. Sending full book on every event would flood the signal queue. |
| Metrics emitted every 1 second | Metrics panel does not need per-event updates. 1-second polling via `QTimer` is appropriate. |
| Max 500 trades in tape | Prevents unbounded memory growth during long runs. |
