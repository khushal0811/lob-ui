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
    double   throughput_eps {0.0};   // events per second (rolling 1s)
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
