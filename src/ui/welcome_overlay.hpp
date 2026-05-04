#pragma once
#include <QWidget>

namespace lob_qt {

class WelcomeOverlay : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeOverlay(QWidget* parent = nullptr);

signals:
    void start_requested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
};

} // namespace lob_qt
