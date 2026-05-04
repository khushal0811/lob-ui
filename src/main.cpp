#include "ui/main_window.hpp"
#include <QApplication>
#include <QFile>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("lob-qt");
    app.setApplicationVersion("0.1.0");

    // Load stylesheet from Qt resources
    QFile style_file(":/style.qss");
    if (style_file.open(QFile::ReadOnly)) {
        app.setStyleSheet(style_file.readAll());
    }

    lob_qt::MainWindow window;
    window.show();

    return app.exec();
}
