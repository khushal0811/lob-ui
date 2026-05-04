# Build plan — lob-qt
# 5-phase coding roadmap

Each phase is independently buildable and runnable. The application is launchable after phase 1 even if it does nothing yet. Every phase ends with a working, visible UI state.

---

## Phase summary

| Phase | Core output | Key signal |
|---|---|---|
| 1 | CMake setup, engine worker, blank window | App launches, engine runs on worker thread |
| 2 | Order book panel + trade tape live from engine | Real matching output visible in UI |
| 3 | Order entry panel + replay controls | User can submit orders and replay CSVs |
| 4 | Metrics panel + spread chart | Full observability layer complete |
| 5 | Polish, styling, README, final integration | Presentable portfolio project |

---

## Phase 1 — Project scaffold, engine worker, and blank window

**Goal:** A Qt application that launches, starts the engine on a worker thread, runs the synthetic generator, and proves the threading model works — even though nothing is displayed yet.

**Deliverables:**
- `CMakeLists.txt` linking Qt6 + lob_core
- `src/bridge/bridge_types.hpp` — all Qt-friendly snapshot structs, registered metatypes
- `src/bridge/command.hpp` — command structs
- `src/engine/engine_worker.hpp/.cpp` — full EngineWorker with all slots and signals stubbed
- `src/ui/main_window.hpp/.cpp` — blank QMainWindow, creates worker, starts thread
- `src/main.cpp` — QApplication entry point
- `.clang-format`, `.gitignore`, `README.md` stub

**Done when:** App launches without crash, worker thread starts, engine processes synthetic events silently in the background, app closes cleanly (worker thread joins).

---

## Phase 2 — Live order book panel and trade tape

**Goal:** The two most important visual elements. Real matching output from the engine rendered live in the UI.

**Deliverables:**
- `src/models/order_book_model.hpp/.cpp` — QAbstractTableModel for bid/ask levels
- `src/models/trade_tape_model.hpp/.cpp` — QAbstractTableModel for trades
- `src/ui/order_book_panel.hpp/.cpp` — two QTableViews (bids + asks), color-coded rows
- `src/ui/trade_tape.hpp/.cpp` — QTableView with auto-scroll, 500 row cap
- EngineWorker emitting real `bookUpdated` and `tradeExecuted` signals
- MainWindow connecting signals to panels

**Done when:** Running the medium scenario shows a live updating bid/ask book and a scrolling trade tape. Data matches what `lob_replay` produces for the same input.

---

## Phase 3 — Order entry panel and replay controls

**Goal:** User interaction. Submit orders manually and replay CSV event logs.

**Deliverables:**
- `src/ui/order_entry.hpp/.cpp` — form with all fields, submit + cancel buttons, feedback label
- `src/ui/replay_controls.hpp/.cpp` — load CSV, play/pause/step/reset, speed slider, event counter
- EngineWorker slots: `submitOrder`, `cancelOrder`, `loadCsv`, `startReplay`, `pauseReplay`, `stepReplay`, `resetEngine`
- QTimer-based replay tick mechanism
- Scenario selector dropdown + run/stop buttons

**Done when:** User can submit a limit order and see it appear on the book. User can load `basic_limit.csv` and replay it step by step, watching the book update on each event.

---

## Phase 4 — Metrics panel and spread chart

**Goal:** Full observability. Numbers and a chart that make the application look like a real monitoring tool.

**Deliverables:**
- `src/ui/metrics_panel.hpp/.cpp` — all metrics labels, updated every second
- `src/ui/spread_chart.hpp/.cpp` — QChart line chart, 60-second rolling window
- EngineWorker `metrics_timer_` emitting `metricsUpdated` every 1 second
- EngineWorker computing throughput from event counter delta
- Spread point added to chart on every `bookUpdated` signal (throttled to 500ms)

**Done when:** Running a scenario for 30 seconds shows a smooth spread chart and updating latency numbers. Throughput number matches the benchmark results from lob-engine phase 5.

---

## Phase 5 — Polish, styling, and README

**Goal:** The application looks intentional, not like a default Qt layout.

**Deliverables:**
- Consistent dark or light colour scheme applied via `QApplication::setStyle` and a stylesheet
- Column widths, fonts, and spacing set explicitly — no default Qt proportions
- Window title: "lob-engine — Order Book Visualizer"
- Minimum window size set: 1280x800
- All panels have section headers (bold QLabel above each widget)
- `README.md` complete: screenshot, build instructions, architecture summary, feature list
- At least one screenshot committed to `docs/screenshot.png`
- Final integration test: run medium scenario for 10 seconds, verify metrics panel shows non-zero values

**Done when:** App looks presentable in a screenshot. README renders cleanly on GitHub. A non-engineer could understand what they are looking at.
