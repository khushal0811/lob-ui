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

    QPushButton* load_btn_      {nullptr};
    QPushButton* play_btn_      {nullptr};
    QPushButton* pause_btn_     {nullptr};
    QPushButton* step_btn_      {nullptr};
    QPushButton* reset_btn_     {nullptr};
    QSlider*     speed_slider_  {nullptr};
    QLabel*      speed_label_   {nullptr};
    QLabel*      progress_label_{nullptr};
    QComboBox*   scenario_combo_{nullptr};
    QPushButton* run_btn_       {nullptr};
    QPushButton* stop_btn_      {nullptr};
};

} // namespace lob_qt
