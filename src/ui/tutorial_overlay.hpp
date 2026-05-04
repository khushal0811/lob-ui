#pragma once
#include <QWidget>
#include <QString>
#include <QStringList>

// Full includes — forward declarations inside a namespace confuse the compiler
#include <QFrame>
#include <QLabel>
#include <QPushButton>

namespace lob_qt {

struct TutorialContent {
    QString      title;
    QString      intro;
    QStringList  steps;
    QString      bonus_tip;
};

class TutorialOverlay : public QWidget {
    Q_OBJECT

public:
    explicit TutorialOverlay(QWidget* parent = nullptr);

    void show_for_scenario(const QString& scenario_name);

signals:
    void dismissed();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void on_dismiss();

private:
    void build_card();
    void populate(const TutorialContent& content);

    static TutorialContent content_for(const QString& scenario);

    QLabel*      title_label_  {nullptr};
    QLabel*      intro_label_  {nullptr};
    QLabel*      steps_label_  {nullptr};
    QLabel*      bonus_label_  {nullptr};
    QPushButton* dismiss_btn_  {nullptr};
    QPushButton* skip_btn_     {nullptr};
    QFrame*      card_         {nullptr};
};

} // namespace lob_qt
