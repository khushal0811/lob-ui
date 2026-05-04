# lob-qt — Real-Time Order Book Visualizer

> **Exchanges have always felt like a black box.** This application opens that box.

Most people who trade — or learn about finance — never see what actually happens when they place an order. What does "best bid" mean? What is a spread? Why does your market order sometimes get a worse price than you expected? Why does liquidity disappear when volatility spikes?

**lob-qt** is a Qt6 desktop app that makes all of this visible — backed by [lob-engine](https://github.com/khushal0811/lob-engine), a low-latency C++ matching engine running on a dedicated background thread.

![lob-qt demo](https://github.com/khushal0811/lob-ui/assets/176611453/9c1d169e-9c93-4288-adce-dcab6e9f0547)

---

**lob-qt** is a desktop application that makes all of this visible in real time. **This is not a mock UI — it is backed by an exchange-style C++ matching engine.** Every order you submit — limit, market, stop, iceberg — immediately interacts with a live order book running on a dedicated background thread. You watch the bid/ask table shift, the spread widen or tighten, and the trade tape fill up — all as a direct result of your actions.

## Why I built this
I built **lob-qt** to make market microstructure visible and to demonstrate how low-latency systems, UI threading, and high-performance matching engines work together. It bridges the gap between raw systems engineering (C++ engine) and user-centric product design (Qt UI).

---

## Technical Edge: Real Thread Separation
One of the strongest parts of this architecture is the **absolute separation of concerns**:
*   **Zero UI Blocking:** The matching engine (`lob-engine`) runs on a dedicated background thread. Even if the engine is processing thousands of events, the UI remains perfectly responsive.
*   **No Shared State:** The UI never touches the engine's memory. All communication happens via thread-safe Qt signals using `QueuedConnection`.
*   **Passive Integration:** The UI "listens" to the engine; it does not mutate it directly, ensuring the integrity of the matching logic.

---

## What You Can See

```
┌─────────────────────────────────┬──────────────────────────────────┐
│        ORDER BOOK               │         ORDER ENTRY              │
│  Ask  ████████████████  9993    │  Type  [Limit      ▾]            │
│  Ask  ████████████  9992        │  Side  [Buy        ▾]            │
│  Ask  ██████████  9991          │  Price  [9991    ]               │
│  ─── Mid: 9991 · Spread: 1 ─── │  Qty    [100     ]               │
│  Bid  ██████████  9990          │  [      Submit Order      ]       │
│  Bid  ████████████  9989        │                                  │
│  Bid  ████████████████  9988    │  ID    Side  Ordered  Filled  St  │
│                                 │  1002  BUY     100      100  ✅   │
│        TRADE TAPE               │  1001  SELL    200       50  ⏳   │
│  13:45:02  BUY   9991  ×  100   │                                  │
│  13:45:02  SELL  9990  ×   50   │        METRICS                   │
│  13:45:01  BUY   9992  ×  200   │  Throughput   1,240 ev/s         │
│                                 │  p50 latency  820 ns             │
│                                 │  Spread chart ↗                  │
└─────────────────────────────────┴──────────────────────────────────┘
```

---

## Features

- **Live Order Book** — bid/ask price levels update in real time, best prices highlighted
- **Trade Tape** — every match printed with timestamp, side, price, and quantity
- **Order Entry** — submit Limit, Market, Stop, Stop-Limit, and Iceberg orders
- **Order Status Table** — see ordered vs filled quantity per order, live as trades come in
- **Metrics Panel** — throughput (ev/s), latency percentiles (p50/p99/max), fill rate, cancel rate
- **Spread Chart** — rolling 120-point line chart of bid/ask spread over time
- **Scenario Engine** — 6 built-in market scenarios running on an exchange-style engine
- **CSV Replay** — load any event log and replay step-by-step or at variable speed
- **Tutorial Overlays** — step-by-step exercises for each scenario that teach the concepts live
- **Welcome Guide** — explains every panel before the engine starts

---

## Architecture

```
┌────────────────────────────────────────────────────────┐
│                    Qt UI Thread                         │
│  MainWindow → OrderBookPanel, TradeTape, OrderEntry    │
│               MetricsPanel, SpreadChart, ReplayControls │
└──────────────────────┬─────────────────────────────────┘
                       │  Qt::QueuedConnection
                       │  (all cross-thread signals)
┌──────────────────────▼─────────────────────────────────┐
│                 Engine Worker Thread                     │
│  EngineWorker → lob::MatchingEngine (HFT-grade C++)    │
│  SyntheticGenerator / CsvReplayReader                   │
│  QTimer (1ms generator tick, 1s metrics tick)          │
└────────────────────────────────────────────────────────┘
```

The matching engine (`lob-engine`) is a separate C++ library compiled as a static archive. It runs entirely on a background thread. The UI never touches engine internals directly — all communication goes through typed Qt signals with `QueuedConnection`, making the bridge fully thread-safe.

---

## Build

### Prerequisites

| Dependency | Version |
|---|---|
| macOS | Ventura 13+ (tested on Sonoma, Sequoia) |
| Xcode Command Line Tools | 14+ |
| CMake | 3.20+ |
| Qt | 6.5+ (via Homebrew) |
| **lob-engine** | **Must be built first — see below** |

> **lob-qt links against `lob-engine` as a static library.** Both repos must sit as siblings:
> ```
> Project_1/
> ├── lob-engine/   ← clone and build this first
> └── lob-ui/       ← then build this
> ```
> Clone the engine: `git clone https://github.com/khushal0811/lob-engine`

```bash
# Install Qt via Homebrew if not already present
brew install qt

# Build the engine first (one-time)
cd ../lob-engine
cmake -B build_release -DCMAKE_BUILD_TYPE=Release
cmake --build build_release --parallel

# Build the UI
cd ../lob-ui
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build --parallel

# Run
./build/lob_qt
```

---

## Scenarios

Select any scenario from **Replay Controls → Run Scenario**:

| Scenario | Description | Best for learning |
|---|---|---|
| `medium` | Balanced order flow, ~1000 ev/s | Starting point — see the book breathe |
| `small` | Low volume, ~100 ev/s | Watch individual orders fill slowly |
| `large` | High volume, ~3000 ev/s | Market depth, fast level changes |
| `high_cancel` | 50% cancel rate | Order lifecycle: place → cancel |
| `market_heavy` | Aggressive market orders dominate | How market orders sweep levels |
| `iceberg_stop` | Advanced order types | Iceberg replenishment, stop triggers |

Each scenario launches an interactive tutorial overlay after 2 seconds with concrete exercises — exact prices to enter, what to watch for, and a bonus challenge.

---

## Custom CSV Replay

Load your own event log via **Replay Controls → Load CSV…**

Required columns:
```
event_type, order_id, side, price, quantity, order_type
```

Supported `event_type` values: `new_order`, `cancel_order`  
Supported `order_type` values: `limit`, `market`, `stop`, `stop_limit`, `iceberg`

Use **Step** to advance one event at a time, or **Play** at 0.1×–10× speed.

---

## Key Concepts Explained

**Spread** — the gap between the best ask (lowest sell) and best bid (highest buy). Narrow spread = liquid market. Wide spread = thin book.

**Market order** — executes immediately at whatever price is available. May fill across multiple levels if the size is large enough.

**Limit order** — rests on the book at your price. Only fills when the market reaches you. You see it in the order book table as a new level.

**Iceberg order** — shows only a fraction ("peak qty") on the book. Automatically replenishes as each slice fills. Used to hide large order intent.

**Stop order** — dormant until the market price touches your stop price, then triggers as a market order. Used as a loss-limit or breakout entry.

**Fill rate** — fraction of events in the last second that resulted in trades. High fill rate = aggressive, active market.

---

## Repository Structure

```
lob-ui/
├── src/
│   ├── main.cpp
│   ├── bridge/          # Thread-safe data types (BookSnapshot, TradeRecord, …)
│   ├── engine/          # EngineWorker — Qt wrapper around lob::MatchingEngine
│   ├── models/          # QAbstractTableModel subclasses for order book + trade tape
│   ├── ui/              # All QWidget panels + overlays
│   └── style.qss        # Catppuccin Mocha dark theme
├── CMakeLists.txt
└── README.md
```

---

## License

MIT — see `LICENSE`.