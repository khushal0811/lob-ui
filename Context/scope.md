# Project scope — lob-qt
# Qt 6 desktop visualization and control layer for the C++ limit order book engine

---

## Overview

A Qt 6 C++ desktop application that provides a live visualization and control interface for the `lob-engine` matching engine. The UI runs on the main thread. The engine runs on a dedicated worker thread. A thread-safe bridge connects them via Qt signals and slots. The engine library is unchanged — the UI is purely an observer and controller.

---

## What this project is

A professional desktop tool that demonstrates:
- How to architect a real-time C++ systems application with a Qt frontend
- Correct producer/consumer threading between a high-performance engine and a UI
- Live order book visualization driven by engine state
- Replay controls for deterministic event playback
- Latency and throughput metrics displayed in real time

---

## Qt modules used

| Module | Purpose |
|---|---|
| `Qt::Core` | QThread, QTimer, signals/slots, data types |
| `Qt::Widgets` | Main window, panels, buttons, labels, forms |
| `Qt::Charts` | Spread over time chart, latency histogram chart |
| `Qt::Concurrent` | Not used — threading handled manually via QThread |

No QML. No web engine. No network stack. Desktop widgets only.

---

## UI features — in scope

### Order book panel
- Live bid side table: price, quantity, order count per level
- Live ask side table: price, quantity, order count per level
- Top 10 levels on each side
- Color-coded rows: green for bids, red for asks
- Best bid and best ask highlighted
- Updates on every engine event

### Trade tape
- Scrolling list of all executed trades
- Columns: timestamp, aggressor side, price, quantity
- Color-coded: blue for buy-initiated, orange for sell-initiated
- Maximum 500 visible rows, older rows discarded
- Auto-scrolls to latest trade

### Order entry panel
- Fields: order type (limit/market/stop/iceberg), side, price, quantity, peak qty, stop price
- Submit button — sends NewOrderEvent to engine via bridge
- Cancel by ID field + cancel button
- Input validation before submission (non-empty, positive values)
- Feedback label: "Order accepted" / "Order rejected: [reason]"

### Replay controls
- Load CSV button — opens file dialog, loads event log
- Play button — starts replay at configurable speed
- Pause button — pauses replay
- Step button — advances one event at a time
- Reset button — resets engine and book to empty state
- Speed slider: 0.1x to 10x replay speed
- Event counter: "Event 142 / 500"

### Metrics panel
- Throughput: events/sec (rolling 1-second window)
- Latency: p50, p95, p99, max (updated every second)
- Fill rate, cancel rate, rejection rate
- Best bid, best ask, mid price, spread
- Last trade price
- Order count (total resting orders on book)

### Spread chart
- Rolling line chart of spread over last 60 seconds
- X axis: time, Y axis: spread in ticks
- Updates every 500ms

### Scenario selector
- Dropdown: small / medium / large / high_cancel / market_heavy / iceberg_stop
- "Run scenario" button — starts synthetic generator with selected config
- "Stop" button — stops generator

---

## Threading model — in scope

| Thread | Responsibility |
|---|---|
| Main thread (Qt UI) | All widget rendering, user input, signal dispatch |
| Engine worker thread | Matching engine, synthetic generator, CSV replay |
| (No third thread) | Logger and metrics export are synchronous in this context |

Communication:
- UI → Engine: command objects posted via `QMetaObject::invokeMethod` or a thread-safe command queue
- Engine → UI: Qt signals emitted from worker, connected to UI slots via `Qt::QueuedConnection`

The engine worker thread owns the `MatchingEngine` instance. Nothing else touches it.

---

## Excluded from scope

- QML or Qt Quick — desktop widgets only
- Network connectivity or remote feeds
- Database persistence
- Authentication
- Multiple simultaneous order books
- Custom widget painting beyond standard Qt styling
- macOS/Windows installers or packaging
- Plugin architecture
- Undo/redo for order entry
- Order modification in the UI (cancel + resubmit is sufficient)

---

## Build targets

| Target | Description |
|---|---|
| `lob_qt` | Main Qt application executable |
| `lob_core` | Static library imported from `../lob-engine/` |

---

## Platform

- Primary development: macOS (Apple Silicon)
- Qt version: Qt 6.7 or later
- Compiler: AppleClang (macOS) / GCC or Clang (Linux)
- CMake: 3.20+
- C++ standard: C++20

---

## Resume positioning

> Built a Qt 6 desktop interface for a high-performance C++ matching engine, running the UI and engine on separate threads with decoupled signal/slot communication for live order book visualization, replay control, trade logs, and real-time latency metrics.
