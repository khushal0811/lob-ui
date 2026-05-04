#include "ui/replay_controls.hpp"
#include <QComboBox>
#include <QFileDialog>
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
    load_btn_       = new QPushButton("Load CSV...", this);
    play_btn_       = new QPushButton("▶ Play",  this);
    pause_btn_      = new QPushButton("⏸ Pause", this);
    step_btn_       = new QPushButton("⏭ Step",  this);
    reset_btn_      = new QPushButton("↺ Reset", this);
    speed_label_    = new QLabel("Speed: 1.0x", this);
    progress_label_ = new QLabel("—", this);

    speed_slider_ = new QSlider(Qt::Horizontal, this);
    speed_slider_->setRange(1, 100);   // 0.1x – 10x (divide by 10)
    speed_slider_->setValue(10);       // default 1.0x

    scenario_combo_ = new QComboBox(this);
    scenario_combo_->addItems({"medium", "small", "large",
                                "high_cancel", "market_heavy", "iceberg_stop"});
    run_btn_  = new QPushButton("Run Scenario", this);
    stop_btn_ = new QPushButton("Stop", this);

    auto* replay_buttons = new QHBoxLayout;
    replay_buttons->addWidget(play_btn_);
    replay_buttons->addWidget(pause_btn_);
    replay_buttons->addWidget(step_btn_);
    replay_buttons->addWidget(reset_btn_);

    auto* scenario_row = new QHBoxLayout;
    scenario_row->addWidget(scenario_combo_, 1);
    scenario_row->addWidget(run_btn_);
    scenario_row->addWidget(stop_btn_);

    auto* replay_header = new QLabel("<b>Replay Controls</b>", this);
    replay_header->setStyleSheet("color: #89b4fa; font-size: 13px;");
    auto* scenario_header = new QLabel("<b>Scenario</b>", this);
    scenario_header->setStyleSheet("color: #89b4fa; font-size: 13px;");

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->addWidget(replay_header);
    layout->addWidget(load_btn_);
    layout->addLayout(replay_buttons);
    layout->addWidget(speed_label_);
    layout->addWidget(speed_slider_);
    layout->addWidget(progress_label_);
    layout->addSpacing(8);
    layout->addWidget(scenario_header);
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
    QString path = QFileDialog::getOpenFileName(
        this, "Load Event Log", "", "CSV Files (*.csv);;All Files (*)");
    if (!path.isEmpty()) emit load_csv(path);
}

void ReplayControls::on_play_clicked()  { emit start_replay(speed_slider_->value() / 10.0); }
void ReplayControls::on_pause_clicked() { emit pause_replay(); }
void ReplayControls::on_step_clicked()  { emit step_replay(); }
void ReplayControls::on_reset_clicked() {
    emit reset_engine();
    progress_label_->setText("—");
}
void ReplayControls::on_run_scenario_clicked()  { emit start_scenario(scenario_combo_->currentText()); }
void ReplayControls::on_stop_scenario_clicked() { emit stop_scenario(); }

void ReplayControls::on_replay_finished() {
    progress_label_->setText("Replay complete ✓");
}

void ReplayControls::update_progress(int current, int total) {
    if (total > 0)
        progress_label_->setText(QString("Event %1 / %2").arg(current).arg(total));
    else
        progress_label_->setText(QString("Event %1").arg(current));
}

} // namespace lob_qt
