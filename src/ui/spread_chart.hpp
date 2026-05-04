#pragma once
#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

namespace lob_qt {

class SpreadChart : public QWidget {
    Q_OBJECT

public:
    explicit SpreadChart(QWidget* parent = nullptr);

public slots:
    void add_point(qint64 spread_ticks);

private:
    static constexpr int kMaxPoints = 120;   // 60 seconds at 0.5s intervals
    QChart*      chart_  {nullptr};
    QLineSeries* series_ {nullptr};
    QChartView*  view_   {nullptr};
    qint64       x_      {0};
};

} // namespace lob_qt
