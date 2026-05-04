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
