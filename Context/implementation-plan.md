# Master implementation plan
# lob-qt — Qt 6 desktop visualization layer for the C++ limit order book engine

---

## How to use this document

This is the authoritative implementation guide for every coding session. Each phase is broken into numbered parts. Each part has a precise goal, exact file list, full code, and acceptance criteria. Work through parts in order. Do not start a new phase until the checklist at the end of the previous phase is fully cleared.

**Critical rules:**
- Never modify anything in `../lob-engine/src/`
- Never run engine code on the Qt UI thread
- Never touch Qt widgets from the worker thread
- Every commit must build and run without crash

---

## Repository setup (do this before phase 1)

```bash
mkdir lob-qt && cd lob-qt
git init
git remote add origin git@github.com:<you>/lob-qt.git
```

**.gitignore:**
```
build/
.cache/
*.o
*.a
*.so
compile_commands.json
*.app/
moc_*
ui_*
qrc_*
```

**.clang-format** — copy exactly from `../lob-engine/.clang-format`

First commit: `chore: initial repo scaffold, clang-format, gitignore`

---

---

# Phase 1 — Project scaffold, engine worker, and blank window

**Goal:** A Qt application that launches, proves the threading model works, and closes cleanly. Nothing visible yet except a blank window.

---

## Part 1.1 — CMakeLists.txt and Qt setup

### Goal
Working CMake build that finds Qt6 and links lob_core. App compiles and launches.

### File: `CMakeLists.txt`

```cmake
cmake_minimum_required(VERSION 3.20)
project(lob_qt CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AUTOMOC ON)

add_compile_options(-Wall -Wextra -Wpedantic -Werror)

# Qt 6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Charts)
qt_standard_project_setup()

# Import lob_core from sibling project
# Only build the core library, not tests or benchmarks
set(LOB_ENGINE_DIR "${CMAKE_SOURCE_DIR}/../lob-engine")
add_library(lob_core STATIC IMPORTED)
set_target_properties(lob_core PROPERTIES
    IMPORTED_LOCATION "${LOB_ENGINE_DIR}/build_release/src/liblob_core.a"
    INTERFACE_INCLUDE_DIRECTORIES "${LOB_ENGINE_DIR}/src"
)

# Collect sources
file(GLOB_RECURSE LOB_QT_SOURCES src/*.cpp)

qt_add_executable(lob_qt ${LOB_QT_SOURCES})

target_include_directories(lob_qt PRIVATE src/)
target_link_libraries(lob_qt PRIVATE
    lob_core
    Qt6::Core
    Qt6::Widgets
    Qt6::Charts
)
```

### Note on lob_core import
This uses the pre-built `liblob_core.a` from the release build of lob-engine. This is simpler than `add_subdirectory` and avoids rebuilding the engine. If lob-engine hasn't been built with `build_release` yet, run `cmake -B build_release -DCMAKE_BUILD_TYPE=Release && cmake --build build_release` in the lob-engine directory first.

### Verify Qt is installed
```bash
brew install qt6           # macOS
qt6-cmake --version        # should print version
```

### Commit
`build: CMakeLists.txt — Qt6 + imported lob_core static library`

---

## Part 1.2 — Bridge types

### Goal
Define all Qt-friendly structs that cross the thread boundary. No `lob::` types in UI code.

### File: `src/bridge/bridge_types.hpp`

```cpp
#pragma once
#include <QMetaType>
#include <QString>
#include <QVector>
#include <cstdint>

namespace lob_qt {

struct LevelEntry {
    qint64  price  {0};
    quint64 volume {0};
    int     count  {0};
};

struct BookSnapshot {
    QVector<LevelEntry> bids;        // top 10, price descending
    QVector<LevelEntry> asks;        // top 10, price ascending
    qint64  best_bid    {0};
    qint64  best_ask    {0};
    qint64  mid_price   {0};
    qint64  spread      {0};
    qint64  last_trade  {0};
    quint64 order_count {0};
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
    double   throughput_eps {0.0};
    quint64  p50_ns         {0};
    quint64  p95_ns         {0};
    quint64  p99_ns         {0};
    quint64  max_ns         {0};
    double   fill_rate      {0.0};
    double   cancel_rate    {0.0};
    double   rejection_rate {0.0};
};

struct RejectionRecord {
    quint64 order_id {0};
    QString reason;
};

} // namespace lob_qt

// Register for cross-thread signal/slot use
Q_DECLARE_METATYPE(lob_qt::BookSnapshot)
Q_DECLARE_METATYPE(lob_qt::TradeRecord)
Q_DECLARE_METATYPE(lob_qt::MetricsSnapshot)
Q_DECLARE_METATYPE(lob_qt::RejectionRecord)
```

### File: `src/bridge/command.hpp`

```cpp
#pragma once
#include <cstdint>
#include <QString>

namespace lob_qt {

// Mirrors lob::OrderType without including engine headers in UI code
enum class OrderTypeUi : uint8_t {
    Limit,
    Market,
    Stop,
    StopLimit,
    Iceberg
};

enum class SideUi : uint8_t {
    Buy,
    Sell
};

struct SubmitOrderCommand {
    uint64_t    id          {0};
    SideUi      side        {SideUi::Buy};
    OrderTypeUi type        {OrderTypeUi::Limit};
    int64_t     price       {0};
    int64_t     stop_price  {0};
    uint64_t    quantity    {0};
    uint64_t    peak_qty    {0};
};

} // namespace lob_qt
```

### Commit
`bridge: Qt-friendly snapshot types and command structs, Q_DECLARE_METATYPE registration`

---

## Part 1.3 — EngineWorker skeleton

### Goal
Full `EngineWorker` class with all slots and signals declared. Slots are stubbed — they will be filled in over the next phases. The threading setup must work correctly right now.

### File: `src/engine/engine_worker.hpp`

```cpp
#pragma once
#include "bridge/bridge_types.hpp"
#include "bridge/command.hpp"
#include <QObject>
#include <QString>
#include <QTimer>
#include <memory>

// Forward declare engine types — don't include Qt in engine headers
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

    quint64 next_order_id_   {1};
    quint64 event_count_     {0};
    quint64 last_event_count_{0};  // for throughput calculation
    int     replay_total_    {0};
    int     replay_current_  {0};

    void init_engine();
    void init_timers();
    void emit_book_snapshot();
    void emit_metrics_snapshot();
    void process_match_result(const void* result_ptr);  // takes lob::MatchResult*
};

} // namespace lob_qt
```

### File: `src/engine/engine_worker.cpp`

```cpp
#include "engine/engine_worker.hpp"
#include "engine/matching_engine.hpp"   // from ../lob-engine/src/
#include "feed/synthetic_gen.hpp"
#include "feed/csv_replay.hpp"
#include "core/config.hpp"
#include <QDateTime>

namespace lob_qt {

EngineWorker::EngineWorker(QObject* parent)
    : QObject(parent) {
    // Register metatypes for cross-thread signals
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

    metrics_timer_->start(1000);  // emit metrics every second
}

// --- Stub implementations (filled in per phase) ---

void EngineWorker::submitOrder(lob_qt::SubmitOrderCommand) {}
void EngineWorker::cancelOrder(quint64) {}
void EngineWorker::loadCsv(QString) {}
void EngineWorker::startReplay(double) {}
void EngineWorker::pauseReplay() { replay_timer_->stop(); }
void EngineWorker::stepReplay() { on_replay_tick(); }
void EngineWorker::resetEngine() { init_engine(); emit_book_snapshot(); }
void EngineWorker::startScenario(QString) {}
void EngineWorker::stopScenario() { generator_timer_->stop(); generator_.reset(); }

void EngineWorker::on_generator_tick() {}
void EngineWorker::on_replay_tick() {}

void EngineWorker::on_metrics_tick() {
    MetricsSnapshot snap;
    quint64 delta = event_count_ - last_event_count_;
    snap.throughput_eps = static_cast<double>(delta);
    last_event_count_ = event_count_;
    emit metricsUpdated(snap);
}

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

    if (auto bb = book.best_bid()) snap.best_bid  = *bb;
    if (auto ba = book.best_ask()) snap.best_ask  = *ba;
    if (auto mp = book.mid_price()) snap.mid_price = *mp;
    if (auto sp = book.spread())    snap.spread    = *sp;
    snap.last_trade  = book.last_trade_price;
    snap.order_count = book.order_count();

    emit bookUpdated(snap);
}

void EngineWorker::emit_metrics_snapshot() {}

void EngineWorker::process_match_result(const void*) {}

} // namespace lob_qt
```

### Commit
`engine: EngineWorker skeleton — QObject worker, timers, stub slots, metatype registration`

---

## Part 1.4 — MainWindow and main.cpp

### Goal
A blank QMainWindow that creates the worker, moves it to a thread, starts the thread, and connects the `destroyed` signal for clean shutdown.

### File: `src/ui/main_window.hpp`

```cpp
#pragma once
#include <QMainWindow>

class QThread;

namespace lob_qt {
class EngineWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void setup_worker();
    void setup_ui();
    void setup_connections();

    EngineWorker* worker_        {nullptr};
    QThread*      worker_thread_ {nullptr};
};

} // namespace lob_qt
```

### File: `src/ui/main_window.cpp`

```cpp
#include "ui/main_window.hpp"
#include "engine/engine_worker.hpp"
#include <QThread>
#include <QLabel>

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

void MainWindow::setup_worker() {
    worker_        = new EngineWorker();
    worker_thread_ = new QThread(this);

    worker_->moveToThread(worker_thread_);

    // Clean up worker when thread finishes
    connect(worker_thread_, &QThread::finished,
            worker_, &QObject::deleteLater);

    worker_thread_->start();
}

void MainWindow::setup_ui() {
    // Placeholder — replaced in phase 2
    auto* label = new QLabel("lob-engine running on worker thread", this);
    label->setAlignment(Qt::AlignCenter);
    setCentralWidget(label);
}

void MainWindow::setup_connections() {
    // Placeholder — filled in per phase
}

} // namespace lob_qt
```

### File: `src/main.cpp`

```cpp
#include "ui/main_window.hpp"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("lob-qt");
    app.setApplicationVersion("0.1.0");

    lob_qt::MainWindow window;
    window.show();

    return app.exec();
}
```

### Build and verify:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/lob_qt
```

App should launch showing a blank window with the title "lob-engine — Order Book Visualizer". No crash on close.

### Commit
`ui: MainWindow and main.cpp — blank window, worker thread setup, clean shutdown`

---

## Phase 1 checklist

### Build
- [ ] `cmake -B build && cmake --build build` succeeds zero warnings
- [ ] App launches without crash
- [ ] App closes cleanly — no hung thread, no "QThread destroyed while still running" warning

### Threading
- [ ] EngineWorker is constructed and `moveToThread()` called before `worker_thread_->start()`
- [ ] `worker_thread_->quit()` and `wait()` called in MainWindow destructor
- [ ] `connect(worker_thread_, &QThread::finished, worker_, &QObject::deleteLater)` is present

### Code quality
- [ ] No Qt headers included in `engine_worker.hpp` beyond QObject, QTimer, QString
- [ ] No `lob::` types in `bridge_types.hpp` or `command.hpp`
- [ ] `Q_DECLARE_METATYPE` present for all four bridge types
- [ ] `qRegisterMetaType` called in EngineWorker constructor

### Git
- [ ] Parts 1.1 through 1.4 committed in order with correct messages

---

---

# Phase 2 — Live order book panel and trade tape

**Goal:** Real matching output from the engine rendered live in the UI. This is the most important visual milestone.

---

## Part 2.1 — OrderBookModel

### Goal
QAbstractTableModel that holds a list of `LevelEntry` and updates the view efficiently.

### File: `src/models/order_book_model.hpp`

```cpp
#pragma once
#include "bridge/bridge_types.hpp"
#include <QAbstractTableModel>
#include <QColor>

namespace lob_qt {

class OrderBookModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum Column { Price = 0, Volume, OrderCount, ColumnCount };

    explicit OrderBookModel(bool is_bid, QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = {}) const override;
    int      columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

public slots:
    void update(const QVector<LevelEntry>& levels);

private:
    QVector<LevelEntry> levels_;
    bool                is_bid_;
    QColor              row_color_;
};

} // namespace lob_qt
```

### File: `src/models/order_book_model.cpp`

```cpp
#include "models/order_book_model.hpp"

namespace lob_qt {

OrderBookModel::OrderBookModel(bool is_bid, QObject* parent)
    : QAbstractTableModel(parent)
    , is_bid_(is_bid)
    , row_color_(is_bid ? QColor(200, 255, 200) : QColor(255, 200, 200)) {}

int OrderBookModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(levels_.size());
}

int OrderBookModel::columnCount(const QModelIndex&) const {
    return ColumnCount;
}

QVariant OrderBookModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= levels_.size()) return {};

    const auto& entry = levels_[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Price:      return QString::number(entry.price);
        case Volume:     return QString::number(entry.volume);
        case OrderCount: return QString::number(entry.count);
        default:         return {};
        }
    }

    if (role == Qt::BackgroundRole) {
        // Highlight best level
        if (index.row() == 0) {
            return is_bid_ ? QColor(100, 220, 100) : QColor(220, 100, 100);
        }
        return row_color_;
    }

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignRight + Qt::AlignVCenter;
    }

    return {};
}

QVariant OrderBookModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case Price:      return "Price";
    case Volume:     return "Volume";
    case OrderCount: return "Orders";
    default:         return {};
    }
}

void OrderBookModel::update(const QVector<LevelEntry>& levels) {
    beginResetModel();
    levels_ = levels;
    endResetModel();
}

} // namespace lob_qt
```

### Commit
`models: OrderBookModel — QAbstractTableModel for bid/ask price levels with color coding`

---

## Part 2.2 — TradeTapeModel

### File: `src/models/trade_tape_model.hpp`

```cpp
#pragma once
#include "bridge/bridge_types.hpp"
#include <QAbstractTableModel>

namespace lob_qt {

class TradeTapeModel : public QAbstractTableModel {
    Q_OBJECT

public:
    static constexpr int kMaxRows = 500;
    enum Column { Time = 0, Side, Price, Quantity, ColumnCount };

    explicit TradeTapeModel(QObject* parent = nullptr);

    int      rowCount(const QModelIndex& parent = {}) const override;
    int      columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

public slots:
    void add_trade(const lob_qt::TradeRecord& record);

private:
    QVector<TradeRecord> trades_;
};

} // namespace lob_qt
```

### File: `src/models/trade_tape_model.cpp`

```cpp
#include "models/trade_tape_model.hpp"

namespace lob_qt {

TradeTapeModel::TradeTapeModel(QObject* parent)
    : QAbstractTableModel(parent) {}

int TradeTapeModel::rowCount(const QModelIndex&) const {
    return static_cast<int>(trades_.size());
}

int TradeTapeModel::columnCount(const QModelIndex&) const {
    return ColumnCount;
}

QVariant TradeTapeModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= trades_.size()) return {};
    const auto& t = trades_[index.row()];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Time:     return t.timestamp;
        case Side:     return t.buy_aggressor ? "BUY" : "SELL";
        case Price:    return QString::number(t.price);
        case Quantity: return QString::number(t.quantity);
        default:       return {};
        }
    }

    if (role == Qt::ForegroundRole) {
        return t.buy_aggressor ? QColor(0, 120, 200) : QColor(200, 80, 0);
    }

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignRight + Qt::AlignVCenter;
    }

    return {};
}

QVariant TradeTapeModel::headerData(int section, Qt::Orientation orientation,
                                     int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) return {};
    switch (section) {
    case Time:     return "Time";
    case Side:     return "Side";
    case Price:    return "Price";
    case Quantity: return "Qty";
    default:       return {};
    }
}

void TradeTapeModel::add_trade(const lob_qt::TradeRecord& record) {
    beginInsertRows({}, 0, 0);
    trades_.prepend(record);
    endInsertRows();

    if (trades_.size() > kMaxRows) {
        beginRemoveRows({}, kMaxRows, trades_.size() - 1);
        trades_.resize(kMaxRows);
        endRemoveRows();
    }
}

} // namespace lob_qt
```

### Commit
`models: TradeTapeModel — prepend-insert with 500 row cap and color-coded side`

---

## Part 2.3 — OrderBookPanel widget

### File: `src/ui/order_book_panel.hpp`

```cpp
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

    OrderBookModel* bid_model_ {nullptr};
    OrderBookModel* ask_model_ {nullptr};
    QTableView*     bid_view_  {nullptr};
    QTableView*     ask_view_  {nullptr};
    QLabel*         spread_label_ {nullptr};
    QLabel*         mid_label_    {nullptr};
};

} // namespace lob_qt
```

### File: `src/ui/order_book_panel.cpp`

```cpp
#include "ui/order_book_panel.hpp"
#include "models/order_book_model.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

namespace lob_qt {

OrderBookPanel::OrderBookPanel(QWidget* parent) : QWidget(parent) {
    bid_model_ = new OrderBookModel(true,  this);
    ask_model_ = new OrderBookModel(false, this);
    bid_view_  = new QTableView(this);
    ask_view_  = new QTableView(this);

    bid_view_->setModel(bid_model_);
    ask_view_->setModel(ask_model_);

    setup_table(bid_view_, true);
    setup_table(ask_view_, false);

    spread_label_ = new QLabel("Spread: —", this);
    mid_label_    = new QLabel("Mid: —",    this);

    auto* info_layout = new QHBoxLayout;
    info_layout->addWidget(mid_label_);
    info_layout->addStretch();
    info_layout->addWidget(spread_label_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>Order Book</b>", this));
    layout->addWidget(ask_view_);
    layout->addLayout(info_layout);
    layout->addWidget(bid_view_);
    setLayout(layout);
}

void OrderBookPanel::setup_table(QTableView* view, bool) {
    view->setSelectionMode(QAbstractItemView::NoSelection);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setAlternatingRowColors(false);
    view->horizontalHeader()->setStretchLastSection(true);
    view->verticalHeader()->hide();
    view->setShowGrid(false);
    view->setFixedHeight(220);
}

void OrderBookPanel::update(const lob_qt::BookSnapshot& snapshot) {
    bid_model_->update(snapshot.bids);
    ask_model_->update(snapshot.asks);

    spread_label_->setText(QString("Spread: %1").arg(snapshot.spread));
    mid_label_->setText(QString("Mid: %1").arg(snapshot.mid_price));
}

} // namespace lob_qt
```

### Commit
`ui: OrderBookPanel — two QTableViews for bids/asks with spread and mid labels`

---

## Part 2.4 — TradeTape widget

### File: `src/ui/trade_tape.hpp`

```cpp
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
```

### File: `src/ui/trade_tape.cpp`

```cpp
#include "ui/trade_tape.hpp"
#include "models/trade_tape_model.hpp"
#include <QHeaderView>
#include <QLabel>
#include <QTableView>
#include <QVBoxLayout>

namespace lob_qt {

TradeTape::TradeTape(QWidget* parent) : QWidget(parent) {
    model_ = new TradeTapeModel(this);
    view_  = new QTableView(this);
    view_->setModel(model_);
    view_->setSelectionMode(QAbstractItemView::NoSelection);
    view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view_->horizontalHeader()->setStretchLastSection(true);
    view_->verticalHeader()->hide();
    view_->setShowGrid(false);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>Trade Tape</b>", this));
    layout->addWidget(view_);
    setLayout(layout);
}

void TradeTape::add_trade(const lob_qt::TradeRecord& record) {
    model_->add_trade(record);
    view_->scrollToTop();
}

} // namespace lob_qt
```

### Commit
`ui: TradeTape — prepend-scrolling trade list with 500 row cap`

---

## Part 2.5 — Wire engine worker signals to live data

### Goal
`EngineWorker` now runs the synthetic generator and emits real `bookUpdated` and `tradeExecuted` signals. MainWindow connects them to the panels.

### Update `engine_worker.cpp` — implement `startScenario` and `on_generator_tick`

```cpp
void EngineWorker::startScenario(QString scenario_name) {
    generator_timer_->stop();
    generator_.reset();

    lob::EngineConfig config;
    config.mid_price   = 10000;
    config.arrival_rate = 1000.0;

    if (scenario_name == "small") {
        config.arrival_rate    = 100.0;
        config.cancel_probability = 0.1;
        config.price_std_dev   = 5.0;
    } else if (scenario_name == "large") {
        config.arrival_rate    = 5000.0;
        config.cancel_probability = 0.3;
    } else if (scenario_name == "high_cancel") {
        config.cancel_probability = 0.7;
    }
    // default = medium

    generator_ = std::make_unique<lob::SyntheticGenerator>(config, 42);
    generator_timer_->start(1);  // fire every 1ms
}

void EngineWorker::on_generator_tick() {
    if (!generator_) return;

    // Process up to 10 events per tick to keep UI responsive
    for (int i = 0; i < 10; ++i) {
        lob::OrderEvent ev;
        if (!generator_->next(ev)) {
            generator_timer_->stop();
            break;
        }
        auto result = engine_->submit(ev);
        ++event_count_;

        for (const auto& trade : result.trades) {
            TradeRecord rec;
            rec.aggressor_id  = trade.aggressor_id;
            rec.passive_id    = trade.passive_id;
            rec.price         = trade.price;
            rec.quantity      = trade.quantity;
            rec.buy_aggressor = (trade.aggressor_side == lob::AggressorSide::Buy);
            rec.timestamp     = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
            emit tradeExecuted(rec);
        }
    }

    emit_book_snapshot();
}
```

### Update `main_window.cpp` — add panels and connections

```cpp
void MainWindow::setup_ui() {
    book_panel_  = new OrderBookPanel(this);
    trade_tape_  = new TradeTape(this);

    auto* left   = new QWidget(this);
    auto* layout = new QVBoxLayout(left);
    layout->addWidget(book_panel_);
    layout->addWidget(trade_tape_);
    left->setLayout(layout);

    setCentralWidget(left);
}

void MainWindow::setup_connections() {
    connect(worker_, &EngineWorker::bookUpdated,
            book_panel_, &OrderBookPanel::update,
            Qt::QueuedConnection);

    connect(worker_, &EngineWorker::tradeExecuted,
            trade_tape_, &TradeTape::add_trade,
            Qt::QueuedConnection);

    // Auto-start medium scenario for demo
    QMetaObject::invokeMethod(worker_, "startScenario",
                              Qt::QueuedConnection,
                              Q_ARG(QString, "medium"));
}
```

### Commit
`engine: synthetic generator wired to live UI — bookUpdated and tradeExecuted signals flowing`

---

## Phase 2 checklist

### Build
- [ ] Clean build zero warnings
- [ ] App launches and shows order book panel and trade tape

### Visual verification
- [ ] Bid side shows green rows, ask side shows red rows
- [ ] Best level (row 0) is darker green / darker red
- [ ] Trade tape shows trades scrolling in real time
- [ ] Buy-initiated trades are blue, sell-initiated are orange
- [ ] Spread and mid labels update continuously

### Threading
- [ ] No "QObject: Cannot create children for a parent in a different thread" warning in console
- [ ] No crashes after running for 60 seconds

### Data correctness
- [ ] Best bid price is always lower than best ask price (no crossed book visible)
- [ ] Trade price is always within the bid/ask range

### Git
- [ ] Parts 2.1 through 2.5 committed in order
# Implementation plan — phases 3, 4, and 5
# lob-qt — Qt 6 desktop visualization layer

---

# Phase 3 — Order entry panel and replay controls

**Goal:** User interaction. Submit orders manually, cancel by ID, load and replay CSV event logs step by step or at speed.

---

## Part 3.1 — Order entry panel

### File: `src/ui/order_entry.hpp`

```cpp
#pragma once
#include "bridge/bridge_types.hpp"
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

    QComboBox*   type_combo_   {nullptr};
    QComboBox*   side_combo_   {nullptr};
    QLineEdit*   price_edit_   {nullptr};
    QLineEdit*   qty_edit_     {nullptr};
    QLineEdit*   peak_edit_    {nullptr};  // iceberg only
    QLineEdit*   stop_edit_    {nullptr};  // stop only
    QPushButton* submit_btn_   {nullptr};
    QLineEdit*   cancel_id_edit_{nullptr};
    QPushButton* cancel_btn_   {nullptr};
    QLabel*      feedback_label_{nullptr};

    static quint64 next_id_;
};

} // namespace lob_qt
```

### File: `src/ui/order_entry.cpp`

```cpp
#include "ui/order_entry.hpp"
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTimer>

namespace lob_qt {

quint64 OrderEntry::next_id_ = 1000;

OrderEntry::OrderEntry(QWidget* parent) : QWidget(parent) {
    setup_ui();
}

void OrderEntry::setup_ui() {
    type_combo_    = new QComboBox(this);
    side_combo_    = new QComboBox(this);
    price_edit_    = new QLineEdit(this);
    qty_edit_      = new QLineEdit(this);
    peak_edit_     = new QLineEdit(this);
    stop_edit_     = new QLineEdit(this);
    submit_btn_    = new QPushButton("Submit Order", this);
    cancel_id_edit_= new QLineEdit(this);
    cancel_btn_    = new QPushButton("Cancel Order", this);
    feedback_label_= new QLabel("", this);

    type_combo_->addItems({"Limit", "Market", "Stop", "Stop-Limit", "Iceberg"});
    side_combo_->addItems({"Buy", "Sell"});

    price_edit_->setPlaceholderText("Price (ticks)");
    qty_edit_->setPlaceholderText("Quantity");
    peak_edit_->setPlaceholderText("Peak qty (iceberg)");
    stop_edit_->setPlaceholderText("Stop price");
    cancel_id_edit_->setPlaceholderText("Order ID to cancel");

    peak_edit_->hide();
    stop_edit_->hide();

    auto* form = new QFormLayout;
    form->addRow("Type:",     type_combo_);
    form->addRow("Side:",     side_combo_);
    form->addRow("Price:",    price_edit_);
    form->addRow("Quantity:", qty_edit_);
    form->addRow("Peak qty:", peak_edit_);
    form->addRow("Stop px:",  stop_edit_);

    feedback_label_->setWordWrap(true);
    feedback_label_->setStyleSheet("color: green;");

    auto* cancel_layout = new QHBoxLayout;
    cancel_layout->addWidget(cancel_id_edit_);
    cancel_layout->addWidget(cancel_btn_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>Order Entry</b>", this));
    layout->addLayout(form);
    layout->addWidget(submit_btn_);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("Cancel by ID:", this));
    layout->addLayout(cancel_layout);
    layout->addWidget(feedback_label_);
    layout->addStretch();
    setLayout(layout);

    connect(submit_btn_, &QPushButton::clicked, this, &OrderEntry::on_submit_clicked);
    connect(cancel_btn_, &QPushButton::clicked, this, &OrderEntry::on_cancel_clicked);
    connect(type_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OrderEntry::on_type_changed);
}

void OrderEntry::on_type_changed(int index) {
    // Iceberg = 4, Stop = 2, StopLimit = 3
    peak_edit_->setVisible(index == 4);
    stop_edit_->setVisible(index == 2 || index == 3);
    price_edit_->setEnabled(index != 1);  // market has no price
}

void OrderEntry::on_submit_clicked() {
    if (!validate_inputs()) return;

    SubmitOrderCommand cmd;
    cmd.id       = next_id_++;
    cmd.side     = (side_combo_->currentIndex() == 0) ? SideUi::Buy : SideUi::Sell;
    cmd.type     = static_cast<OrderTypeUi>(type_combo_->currentIndex());
    cmd.price    = price_edit_->text().toLongLong();
    cmd.quantity = qty_edit_->text().toULongLong();
    cmd.peak_qty = peak_edit_->text().toULongLong();
    cmd.stop_price = stop_edit_->text().toLongLong();

    emit submit_order(cmd);
    show_accepted(cmd.id);
}

void OrderEntry::on_cancel_clicked() {
    bool ok = false;
    quint64 id = cancel_id_edit_->text().toULongLong(&ok);
    if (!ok || id == 0) {
        feedback_label_->setStyleSheet("color: red;");
        feedback_label_->setText("Invalid order ID");
        return;
    }
    emit cancel_order(id);
    feedback_label_->setStyleSheet("color: blue;");
    feedback_label_->setText(QString("Cancel sent for ID %1").arg(id));
}

bool OrderEntry::validate_inputs() {
    if (qty_edit_->text().isEmpty() || qty_edit_->text().toULongLong() == 0) {
        feedback_label_->setStyleSheet("color: red;");
        feedback_label_->setText("Quantity must be > 0");
        return false;
    }
    int type_idx = type_combo_->currentIndex();
    if (type_idx != 1 && price_edit_->text().isEmpty()) {  // not market
        feedback_label_->setStyleSheet("color: red;");
        feedback_label_->setText("Price required for this order type");
        return false;
    }
    return true;
}

void OrderEntry::show_rejection(const lob_qt::RejectionRecord& record) {
    feedback_label_->setStyleSheet("color: red;");
    feedback_label_->setText(QString("Rejected (ID %1): %2")
                             .arg(record.order_id).arg(record.reason));
}

void OrderEntry::show_accepted(quint64 order_id) {
    feedback_label_->setStyleSheet("color: green;");
    feedback_label_->setText(QString("Order %1 accepted").arg(order_id));
    // Clear after 3 seconds
    QTimer::singleShot(3000, feedback_label_, [this]{ feedback_label_->clear(); });
}

} // namespace lob_qt
```

### Commit
`ui: OrderEntry panel — all order types, input validation, feedback label`

---

## Part 3.2 — Implement submitOrder and cancelOrder in EngineWorker

### Update `engine_worker.cpp`

```cpp
void EngineWorker::submitOrder(lob_qt::SubmitOrderCommand cmd) {
    lob::Order order;
    order.id         = cmd.id;
    order.side       = (cmd.side == lob_qt::SideUi::Buy) ? lob::Side::Buy : lob::Side::Sell;
    order.price      = cmd.price;
    order.quantity   = cmd.quantity;
    order.orig_qty   = cmd.quantity;
    order.peak_qty   = cmd.peak_qty;
    order.stop_price = cmd.stop_price;
    order.status     = lob::OrderStatus::New;

    switch (cmd.type) {
    case lob_qt::OrderTypeUi::Limit:     order.type = lob::OrderType::Limit;     break;
    case lob_qt::OrderTypeUi::Market:    order.type = lob::OrderType::Market;    break;
    case lob_qt::OrderTypeUi::Stop:      order.type = lob::OrderType::Stop;      break;
    case lob_qt::OrderTypeUi::StopLimit: order.type = lob::OrderType::StopLimit; break;
    case lob_qt::OrderTypeUi::Iceberg:   order.type = lob::OrderType::Iceberg;   break;
    }

    auto result = engine_->submit(lob::NewOrderEvent{order});
    ++event_count_;

    for (const auto& trade : result.trades) {
        lob_qt::TradeRecord rec;
        rec.aggressor_id  = trade.aggressor_id;
        rec.passive_id    = trade.passive_id;
        rec.price         = trade.price;
        rec.quantity      = trade.quantity;
        rec.buy_aggressor = (trade.aggressor_side == lob::AggressorSide::Buy);
        rec.timestamp     = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        emit tradeExecuted(rec);
    }

    for (const auto& rej : result.rejections) {
        lob_qt::RejectionRecord r;
        r.order_id = rej.order_id;
        r.reason   = QString::fromStdString(
            std::to_string(static_cast<int>(rej.reason)));
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
```

### Commit
`engine: submitOrder and cancelOrder implemented — converts UI commands to engine events`

---

## Part 3.3 — Replay controls panel

### File: `src/ui/replay_controls.hpp`

```cpp
#pragma once
#include <QWidget>

class QComboBox;
class QPushButton;
class QSlider;
class QLabel;

namespace lob_qt {

class ReplayControls : public QWidget {
    Q_OBJECT

public:
    explicit ReplayControls(QWidget* parent = nullptr);

public slots:
    void on_replay_finished();
    void update_progress(int current, int total);

signals:
    void load_csv(QString path);
    void start_replay(double speed);
    void pause_replay();
    void step_replay();
    void reset_engine();
    void start_scenario(QString name);
    void stop_scenario();

private slots:
    void on_load_clicked();
    void on_play_clicked();
    void on_pause_clicked();
    void on_step_clicked();
    void on_reset_clicked();
    void on_run_scenario_clicked();
    void on_stop_scenario_clicked();

private:
    void setup_ui();

    QPushButton* load_btn_     {nullptr};
    QPushButton* play_btn_     {nullptr};
    QPushButton* pause_btn_    {nullptr};
    QPushButton* step_btn_     {nullptr};
    QPushButton* reset_btn_    {nullptr};
    QSlider*     speed_slider_ {nullptr};
    QLabel*      progress_label_{nullptr};
    QLabel*      speed_label_  {nullptr};
    QComboBox*   scenario_combo_{nullptr};
    QPushButton* run_btn_      {nullptr};
    QPushButton* stop_btn_     {nullptr};
};

} // namespace lob_qt
```

### File: `src/ui/replay_controls.cpp`

```cpp
#include "ui/replay_controls.hpp"
#include <QComboBox>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace lob_qt {

ReplayControls::ReplayControls(QWidget* parent) : QWidget(parent) {
    setup_ui();
}

void ReplayControls::setup_ui() {
    load_btn_      = new QPushButton("Load CSV...", this);
    play_btn_      = new QPushButton("▶ Play",  this);
    pause_btn_     = new QPushButton("⏸ Pause", this);
    step_btn_      = new QPushButton("⏭ Step",  this);
    reset_btn_     = new QPushButton("↺ Reset", this);
    progress_label_= new QLabel("—", this);
    speed_label_   = new QLabel("Speed: 1.0x", this);

    speed_slider_  = new QSlider(Qt::Horizontal, this);
    speed_slider_->setRange(1, 100);   // 0.1x to 10x (divide by 10)
    speed_slider_->setValue(10);

    scenario_combo_ = new QComboBox(this);
    scenario_combo_->addItems({"medium", "small", "large", "high_cancel",
                                "market_heavy", "iceberg_stop"});
    run_btn_  = new QPushButton("Run Scenario", this);
    stop_btn_ = new QPushButton("Stop", this);

    auto* replay_buttons = new QHBoxLayout;
    replay_buttons->addWidget(play_btn_);
    replay_buttons->addWidget(pause_btn_);
    replay_buttons->addWidget(step_btn_);
    replay_buttons->addWidget(reset_btn_);

    auto* scenario_row = new QHBoxLayout;
    scenario_row->addWidget(scenario_combo_);
    scenario_row->addWidget(run_btn_);
    scenario_row->addWidget(stop_btn_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>Replay Controls</b>", this));
    layout->addWidget(load_btn_);
    layout->addLayout(replay_buttons);
    layout->addWidget(speed_label_);
    layout->addWidget(speed_slider_);
    layout->addWidget(progress_label_);
    layout->addSpacing(8);
    layout->addWidget(new QLabel("<b>Scenario</b>", this));
    layout->addLayout(scenario_row);
    layout->addStretch();
    setLayout(layout);

    connect(load_btn_,  &QPushButton::clicked, this, &ReplayControls::on_load_clicked);
    connect(play_btn_,  &QPushButton::clicked, this, &ReplayControls::on_play_clicked);
    connect(pause_btn_, &QPushButton::clicked, this, &ReplayControls::on_pause_clicked);
    connect(step_btn_,  &QPushButton::clicked, this, &ReplayControls::on_step_clicked);
    connect(reset_btn_, &QPushButton::clicked, this, &ReplayControls::on_reset_clicked);
    connect(run_btn_,   &QPushButton::clicked, this, &ReplayControls::on_run_scenario_clicked);
    connect(stop_btn_,  &QPushButton::clicked, this, &ReplayControls::on_stop_scenario_clicked);

    connect(speed_slider_, &QSlider::valueChanged, this, [this](int v) {
        speed_label_->setText(QString("Speed: %1x").arg(v / 10.0, 0, 'f', 1));
    });
}

void ReplayControls::on_load_clicked() {
    QString path = QFileDialog::getOpenFileName(this, "Load Event Log", "",
                                                "CSV Files (*.csv)");
    if (!path.isEmpty()) emit load_csv(path);
}

void ReplayControls::on_play_clicked()  { emit start_replay(speed_slider_->value() / 10.0); }
void ReplayControls::on_pause_clicked() { emit pause_replay(); }
void ReplayControls::on_step_clicked()  { emit step_replay(); }
void ReplayControls::on_reset_clicked() { emit reset_engine(); progress_label_->setText("—"); }
void ReplayControls::on_replay_finished() { progress_label_->setText("Replay complete"); }
void ReplayControls::on_run_scenario_clicked() { emit start_scenario(scenario_combo_->currentText()); }
void ReplayControls::on_stop_scenario_clicked() { emit stop_scenario(); }

void ReplayControls::update_progress(int current, int total) {
    progress_label_->setText(QString("Event %1 / %2").arg(current).arg(total));
}

} // namespace lob_qt
```

### Commit
`ui: ReplayControls — load/play/pause/step/reset with speed slider and scenario selector`

---

## Part 3.4 — Implement loadCsv and startReplay in EngineWorker

```cpp
void EngineWorker::loadCsv(QString path) {
    replay_reader_ = std::make_unique<lob::CsvReplayReader>(path.toStdString());
    replay_total_   = replay_reader_->total_events();
    replay_current_ = 0;
    emit replayProgress(0, replay_total_);
}

void EngineWorker::startReplay(double speed_multiplier) {
    if (!replay_reader_) return;
    // Base rate: 100 events/sec = 10ms interval. Speed multiplier scales it.
    int interval_ms = static_cast<int>(10.0 / speed_multiplier);
    interval_ms = std::max(1, interval_ms);
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

    for (const auto& trade : result.trades) {
        lob_qt::TradeRecord rec;
        rec.aggressor_id  = trade.aggressor_id;
        rec.passive_id    = trade.passive_id;
        rec.price         = trade.price;
        rec.quantity      = trade.quantity;
        rec.buy_aggressor = (trade.aggressor_side == lob::AggressorSide::Buy);
        rec.timestamp     = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
        emit tradeExecuted(rec);
    }

    emit replayProgress(replay_current_, replay_total_);
    emit_book_snapshot();
}
```

### Commit
`engine: loadCsv and startReplay implemented — QTimer-paced event delivery with progress signals`

---

## Part 3.5 — Wire order entry and replay into MainWindow

### Update `main_window.cpp` setup_ui and setup_connections

```cpp
void MainWindow::setup_ui() {
    book_panel_   = new OrderBookPanel(this);
    trade_tape_   = new TradeTape(this);
    order_entry_  = new OrderEntry(this);
    replay_ctrl_  = new ReplayControls(this);

    auto* left_widget  = new QWidget(this);
    auto* left_layout  = new QVBoxLayout(left_widget);
    left_layout->addWidget(book_panel_, 2);
    left_layout->addWidget(trade_tape_, 3);
    left_widget->setLayout(left_layout);

    auto* right_widget = new QWidget(this);
    auto* right_layout = new QVBoxLayout(right_widget);
    right_layout->addWidget(order_entry_);
    right_layout->addWidget(replay_ctrl_);
    right_widget->setLayout(right_layout);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(left_widget);
    splitter->addWidget(right_widget);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);

    setCentralWidget(splitter);
}

void MainWindow::setup_connections() {
    // Engine → UI (queued — cross-thread)
    connect(worker_, &EngineWorker::bookUpdated,
            book_panel_, &OrderBookPanel::update, Qt::QueuedConnection);
    connect(worker_, &EngineWorker::tradeExecuted,
            trade_tape_, &TradeTape::add_trade, Qt::QueuedConnection);
    connect(worker_, &EngineWorker::orderRejected,
            order_entry_, &OrderEntry::show_rejection, Qt::QueuedConnection);
    connect(worker_, &EngineWorker::replayFinished,
            replay_ctrl_, &ReplayControls::on_replay_finished, Qt::QueuedConnection);
    connect(worker_, &EngineWorker::replayProgress,
            replay_ctrl_, &ReplayControls::update_progress, Qt::QueuedConnection);

    // UI → Engine (auto — resolves to queued since worker is on different thread)
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
}
```

### Commit
`ui: MainWindow wired — order entry, replay controls, full signal/slot map complete`

---

## Phase 3 checklist

### Build
- [ ] Clean build zero warnings

### Order entry
- [ ] Submit a limit buy → appears on bid side of book
- [ ] Submit a limit sell at same price → trade appears in tape, order removed from book
- [ ] Submit with empty quantity → feedback label shows error, no crash
- [ ] Cancel non-existent ID → handled gracefully (rejection signal received, no crash)
- [ ] Iceberg: peak_qty field appears only when Iceberg type selected
- [ ] Stop: stop price field appears only for Stop and Stop-Limit types

### Replay
- [ ] Load `basic_limit.csv` → progress shows "Event 0 / 20"
- [ ] Press Step → book updates one event at a time
- [ ] Press Play → book animates through all events
- [ ] Press Pause → animation stops mid-replay
- [ ] Press Reset → book clears, progress resets
- [ ] Run "medium" scenario → live book activity resumes
- [ ] Speed slider changes replay pace visibly

### Git
- [ ] Parts 3.1 through 3.5 committed in order

---

---

# Phase 4 — Metrics panel and spread chart

**Goal:** Full observability layer. Numbers and a live chart.

---

## Part 4.1 — MetricsPanel widget

### File: `src/ui/metrics_panel.hpp`

```cpp
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
    QLabel* throughput_label_ {nullptr};
    QLabel* p50_label_        {nullptr};
    QLabel* p95_label_        {nullptr};
    QLabel* p99_label_        {nullptr};
    QLabel* max_label_        {nullptr};
    QLabel* fill_rate_label_  {nullptr};
    QLabel* cancel_rate_label_{nullptr};
    QLabel* best_bid_label_   {nullptr};
    QLabel* best_ask_label_   {nullptr};
    QLabel* spread_label_     {nullptr};
    QLabel* last_trade_label_ {nullptr};
    QLabel* order_count_label_{nullptr};
};

} // namespace lob_qt
```

### File: `src/ui/metrics_panel.cpp`

```cpp
#include "ui/metrics_panel.hpp"
#include <QFormLayout>
#include <QLabel>
#include <QVBoxLayout>

namespace lob_qt {

MetricsPanel::MetricsPanel(QWidget* parent) : QWidget(parent) {
    throughput_label_  = new QLabel("—", this);
    p50_label_         = new QLabel("—", this);
    p95_label_         = new QLabel("—", this);
    p99_label_         = new QLabel("—", this);
    max_label_         = new QLabel("—", this);
    fill_rate_label_   = new QLabel("—", this);
    cancel_rate_label_ = new QLabel("—", this);
    best_bid_label_    = new QLabel("—", this);
    best_ask_label_    = new QLabel("—", this);
    spread_label_      = new QLabel("—", this);
    last_trade_label_  = new QLabel("—", this);
    order_count_label_ = new QLabel("—", this);

    auto* form = new QFormLayout;
    form->addRow("Throughput:",  throughput_label_);
    form->addRow("p50 latency:", p50_label_);
    form->addRow("p95 latency:", p95_label_);
    form->addRow("p99 latency:", p99_label_);
    form->addRow("Max latency:", max_label_);
    form->addRow("Fill rate:",   fill_rate_label_);
    form->addRow("Cancel rate:", cancel_rate_label_);
    form->addRow("Best bid:",    best_bid_label_);
    form->addRow("Best ask:",    best_ask_label_);
    form->addRow("Spread:",      spread_label_);
    form->addRow("Last trade:",  last_trade_label_);
    form->addRow("Orders:",      order_count_label_);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>Metrics</b>", this));
    layout->addLayout(form);
    layout->addStretch();
    setLayout(layout);
}

void MetricsPanel::update(const lob_qt::MetricsSnapshot& snap) {
    throughput_label_->setText(QString("%1 ev/s")
                               .arg(snap.throughput_eps, 0, 'f', 0));
    p50_label_->setText(QString("%1 ns").arg(snap.p50_ns));
    p95_label_->setText(QString("%1 ns").arg(snap.p95_ns));
    p99_label_->setText(QString("%1 ns").arg(snap.p99_ns));
    max_label_->setText(QString("%1 ns").arg(snap.max_ns));
    fill_rate_label_->setText(QString("%1%")
                              .arg(snap.fill_rate * 100.0, 0, 'f', 1));
    cancel_rate_label_->setText(QString("%1%")
                                .arg(snap.cancel_rate * 100.0, 0, 'f', 1));
}

void MetricsPanel::update_book_info(const lob_qt::BookSnapshot& snap) {
    best_bid_label_->setText(QString::number(snap.best_bid));
    best_ask_label_->setText(QString::number(snap.best_ask));
    spread_label_->setText(QString::number(snap.spread));
    last_trade_label_->setText(QString::number(snap.last_trade));
    order_count_label_->setText(QString::number(snap.order_count));
}

} // namespace lob_qt
```

### Commit
`ui: MetricsPanel — throughput, latency, rates, book info labels`

---

## Part 4.2 — SpreadChart widget

### File: `src/ui/spread_chart.hpp`

```cpp
#pragma once
#include <QWidget>
#include <QtCharts>

namespace lob_qt {

class SpreadChart : public QWidget {
    Q_OBJECT

public:
    explicit SpreadChart(QWidget* parent = nullptr);

public slots:
    void add_point(qint64 spread_ticks);

private:
    static constexpr int kMaxPoints = 120;
    QChart*      chart_  {nullptr};
    QLineSeries* series_ {nullptr};
    QChartView*  view_   {nullptr};
    qint64       x_      {0};
};

} // namespace lob_qt
```

### File: `src/ui/spread_chart.cpp`

```cpp
#include "ui/spread_chart.hpp"
#include <QVBoxLayout>
#include <QLabel>

namespace lob_qt {

SpreadChart::SpreadChart(QWidget* parent) : QWidget(parent) {
    series_ = new QLineSeries(this);
    series_->setName("Spread (ticks)");

    chart_ = new QChart();
    chart_->addSeries(series_);
    chart_->setTitle("Spread over time");
    chart_->createDefaultAxes();
    chart_->legend()->hide();
    chart_->setMargins(QMargins(4, 4, 4, 4));

    view_ = new QChartView(chart_, this);
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setMinimumHeight(180);

    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("<b>Spread Chart</b>", this));
    layout->addWidget(view_);
    setLayout(layout);
}

void SpreadChart::add_point(qint64 spread_ticks) {
    series_->append(x_++, static_cast<double>(spread_ticks));

    if (series_->count() > kMaxPoints) {
        series_->remove(0);
    }

    // Rescale axes
    chart_->axes(Qt::Horizontal).first()->setRange(
        std::max(0LL, x_ - kMaxPoints), x_);
}

} // namespace lob_qt
```

### Commit
`ui: SpreadChart — QtCharts rolling 120-point line chart`

---

## Part 4.3 — Wire metrics and chart into MainWindow

Update `setup_ui` to add metrics panel and chart to right panel.
Update `setup_connections`:

```cpp
// Book snapshot drives both the book panel AND metrics book info AND spread chart
connect(worker_, &EngineWorker::bookUpdated,
        metrics_panel_, &MetricsPanel::update_book_info, Qt::QueuedConnection);
connect(worker_, &EngineWorker::bookUpdated, this,
        [this](const lob_qt::BookSnapshot& snap) {
            spread_chart_->add_point(snap.spread);
        }, Qt::QueuedConnection);

// Metrics timer drives the metrics panel
connect(worker_, &EngineWorker::metricsUpdated,
        metrics_panel_, &MetricsPanel::update, Qt::QueuedConnection);
```

### Commit
`ui: metrics panel and spread chart wired — full observability layer complete`

---

## Phase 4 checklist

### Build
- [ ] Clean build zero warnings including Qt Charts linkage

### Visual
- [ ] Metrics panel shows non-zero throughput after running a scenario for 5 seconds
- [ ] Spread chart shows a line that updates every ~500ms
- [ ] Best bid, best ask, spread labels update with the book
- [ ] Order count label reflects number of resting orders

### Data correctness
- [ ] Throughput number is in the same ballpark as lob-engine benchmark results
- [ ] Spread chart Y axis does not show negative values

### Git
- [ ] Parts 4.1 through 4.3 committed in order

---

---

# Phase 5 — Polish, styling, and README

**Goal:** The application looks intentional and presentable.

---

## Part 5.1 — Stylesheet and visual polish

### File: `src/style.qss`

```css
QMainWindow {
    background-color: #1e1e2e;
}

QWidget {
    background-color: #1e1e2e;
    color: #cdd6f4;
    font-family: "SF Mono", "JetBrains Mono", monospace;
    font-size: 12px;
}

QLabel[bold="true"] {
    font-weight: bold;
    color: #89b4fa;
}

QTableView {
    background-color: #181825;
    gridline-color: #313244;
    color: #cdd6f4;
    selection-background-color: #45475a;
    border: none;
}

QHeaderView::section {
    background-color: #313244;
    color: #89b4fa;
    padding: 4px;
    border: none;
}

QPushButton {
    background-color: #313244;
    color: #cdd6f4;
    border: 1px solid #45475a;
    border-radius: 4px;
    padding: 4px 12px;
}

QPushButton:hover {
    background-color: #45475a;
}

QPushButton:pressed {
    background-color: #585b70;
}

QLineEdit, QComboBox {
    background-color: #181825;
    color: #cdd6f4;
    border: 1px solid #45475a;
    border-radius: 3px;
    padding: 3px;
}

QSlider::groove:horizontal {
    height: 4px;
    background: #45475a;
    border-radius: 2px;
}

QSlider::handle:horizontal {
    background: #89b4fa;
    width: 14px;
    height: 14px;
    margin: -5px 0;
    border-radius: 7px;
}
```

### Load in `main.cpp`:

```cpp
QFile style_file(":/style.qss");
if (style_file.open(QFile::ReadOnly)) {
    app.setStyleSheet(style_file.readAll());
}
```

Add to CMakeLists.txt:
```cmake
qt_add_resources(lob_qt "resources"
    PREFIX "/"
    FILES src/style.qss
)
```

### Commit
`ui: dark theme stylesheet — Catppuccin Mocha colour palette`

---

## Part 5.2 — Final layout and sizing

- Set fixed column widths on all QTableViews: Price=100, Volume=100, Orders=60
- Set QSplitter initial sizes: left=900, right=380
- Ensure minimum window size is enforced: 1280x800
- Add `setContentsMargins(8, 8, 8, 8)` to all panel layouts
- Set monospace font on price and volume labels in metrics panel

### Commit
`ui: final layout — column widths, splitter sizes, margins, monospace labels`

---

## Part 5.3 — README.md

```markdown
# lob-qt

Qt 6 desktop visualization and control layer for the
[lob-engine](../lob-engine) high-performance C++ limit order book.

> Runs the matching engine on a dedicated worker thread with decoupled
> Qt signal/slot communication for live order book visualization,
> replay control, trade logs, and real-time latency metrics.

## Screenshot

![lob-qt screenshot](docs/screenshot.png)

## Features

- **Live order book** — top 10 bid/ask levels, color-coded, updates on every event
- **Trade tape** — scrolling list of all executed trades with aggressor side
- **Order entry** — submit limit, market, stop, stop-limit, and iceberg orders
- **Replay controls** — load CSV event logs, play/pause/step/reset, variable speed
- **Scenario runner** — six synthetic market scenarios from the benchmark suite
- **Metrics panel** — throughput, p50/p95/p99 latency, fill and cancel rates
- **Spread chart** — rolling 60-second line chart of bid/ask spread

## Architecture

```
Qt UI Thread          Engine Worker Thread
─────────────         ────────────────────
OrderBookPanel  ←──── bookUpdated signal
TradeTape       ←──── tradeExecuted signal
MetricsPanel    ←──── metricsUpdated signal
OrderEntry      ────► submitOrder slot
ReplayControls  ────► startReplay slot
```

The matching engine (liblob_core) does not know Qt exists.
All cross-thread communication is via Qt signals/slots with QueuedConnection.

## Building

### Prerequisites

- Qt 6.7+: `brew install qt6` (macOS)
- lob-engine built: `cd ../lob-engine && cmake -B build_release -DCMAKE_BUILD_TYPE=Release && cmake --build build_release`
- CMake 3.20+

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/lob_qt
```

## Related

- [lob-engine](../lob-engine) — the matching engine this UI wraps
```

### Commit
`docs: README with architecture summary, feature list, build instructions`

---

## Part 5.4 — Screenshot and final integration check

1. Run the app, select "large" scenario, let it run for 10 seconds
2. Take a screenshot: `Cmd+Shift+4` on macOS
3. Save to `docs/screenshot.png` and commit
4. Verify: metrics panel throughput > 100,000 ev/s, spread chart shows a line, trade tape is scrolling

### Commit
`docs: screenshot of running application`

---

## Phase 5 checklist

### Visual
- [ ] App looks consistent — no default grey Qt widgets visible
- [ ] All labels use the stylesheet colour, not system default
- [ ] Column widths are set — no auto-stretched columns that look uneven
- [ ] Window opens at correct minimum size

### Final integration
- [ ] Run medium scenario 30 seconds — no crash, no memory growth visible in Activity Monitor
- [ ] Submit 5 manual orders — all accepted or rejected cleanly with feedback
- [ ] Load and full-replay `basic_limit.csv` — "Replay complete" shown, book matches expected state
- [ ] Spread chart shows smooth line with no jumps to zero

### Documentation
- [ ] README renders correctly on GitHub
- [ ] Screenshot committed and displays in README
- [ ] Build instructions work on a clean clone

### Git
- [ ] Parts 5.1 through 5.4 committed in order
- [ ] `git log --oneline` shows clean history from part 1.1 to 5.4

---

---

## Appendix: commit message conventions

Same scopes as lob-engine plus:

- `ui` — widget files
- `models` — QAbstractTableModel files
- `engine` — EngineWorker
- `bridge` — bridge_types, command
- `style` — stylesheet
- `build` — CMakeLists

---

## Appendix: full file index

```
lob-qt/
├── src/
│   ├── main.cpp
│   ├── style.qss
│   ├── bridge/
│   │   ├── bridge_types.hpp
│   │   └── command.hpp
│   ├── engine/
│   │   ├── engine_worker.hpp
│   │   └── engine_worker.cpp
│   ├── models/
│   │   ├── order_book_model.hpp
│   │   ├── order_book_model.cpp
│   │   ├── trade_tape_model.hpp
│   │   └── trade_tape_model.cpp
│   └── ui/
│       ├── main_window.hpp
│       ├── main_window.cpp
│       ├── order_book_panel.hpp
│       ├── order_book_panel.cpp
│       ├── trade_tape.hpp
│       ├── trade_tape.cpp
│       ├── order_entry.hpp
│       ├── order_entry.cpp
│       ├── replay_controls.hpp
│       ├── replay_controls.cpp
│       ├── metrics_panel.hpp
│       ├── metrics_panel.cpp
│       ├── spread_chart.hpp
│       └── spread_chart.cpp
├── docs/
│   └── screenshot.png
├── CMakeLists.txt
├── .clang-format
├── .gitignore
└── README.md
```
