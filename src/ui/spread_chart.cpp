#include "ui/spread_chart.hpp"
#include <QLabel>
#include <QVBoxLayout>
#include <QtCharts/QValueAxis>

namespace lob_qt {

SpreadChart::SpreadChart(QWidget* parent) : QWidget(parent) {
    series_ = new QLineSeries(this);
    series_->setName("Spread");
    series_->setColor(QColor(0x89, 0xb4, 0xfa));  // Catppuccin blue

    chart_ = new QChart();
    chart_->addSeries(series_);
    chart_->setTitle("Spread over time");
    chart_->setTitleBrush(QBrush(QColor(0x89, 0xb4, 0xfa)));
    chart_->setBackgroundBrush(QBrush(QColor(0x18, 0x18, 0x25)));
    chart_->setPlotAreaBackgroundBrush(QBrush(QColor(0x18, 0x18, 0x25)));
    chart_->setPlotAreaBackgroundVisible(true);
    chart_->legend()->hide();
    chart_->setMargins(QMargins(4, 4, 4, 4));

    // X axis — sample index
    auto* x_axis = new QValueAxis(this);
    x_axis->setRange(0, kMaxPoints);
    x_axis->setLabelFormat("%d");
    x_axis->setLabelsColor(QColor(0x6c, 0x70, 0x86));
    x_axis->setGridLineColor(QColor(0x31, 0x32, 0x44));
    chart_->addAxis(x_axis, Qt::AlignBottom);
    series_->attachAxis(x_axis);

    // Y axis — spread ticks
    auto* y_axis = new QValueAxis(this);
    y_axis->setRange(0, 10);
    y_axis->setLabelFormat("%d");
    y_axis->setLabelsColor(QColor(0x6c, 0x70, 0x86));
    y_axis->setGridLineColor(QColor(0x31, 0x32, 0x44));
    chart_->addAxis(y_axis, Qt::AlignLeft);
    series_->attachAxis(y_axis);

    view_ = new QChartView(chart_, this);
    view_->setRenderHint(QPainter::Antialiasing);
    view_->setMinimumHeight(180);
    view_->setBackgroundBrush(QBrush(QColor(0x18, 0x18, 0x25)));

    auto* header = new QLabel("<b>Spread Chart</b>", this);
    header->setStyleSheet("color: #89b4fa; font-size: 13px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(header);
    layout->addWidget(view_);
    setLayout(layout);
}

void SpreadChart::add_point(qint64 spread_ticks) {
    if (spread_ticks <= 0) return;  // skip crossed/empty book

    series_->append(static_cast<double>(x_), static_cast<double>(spread_ticks));
    ++x_;

    if (series_->count() > kMaxPoints) {
        series_->remove(0);
    }

    // Slide X axis window
    auto axes_x = chart_->axes(Qt::Horizontal);
    if (!axes_x.isEmpty()) {
        axes_x.first()->setRange(
            static_cast<double>(std::max(qint64{0}, x_ - static_cast<qint64>(kMaxPoints))),
            static_cast<double>(x_));
    }

    // Auto-scale Y axis with a small margin
    auto axes_y = chart_->axes(Qt::Vertical);
    if (!axes_y.isEmpty() && series_->count() > 0) {
        double max_spread = 0;
        for (const auto& pt : series_->points()) {
            if (pt.y() > max_spread) max_spread = pt.y();
        }
        axes_y.first()->setRange(0, max_spread * 1.2 + 1);
    }
}

} // namespace lob_qt
