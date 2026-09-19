#include "MainWindow.hpp"
#include <QApplication>
#include <QIcon>
#include <iostream>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("ApoNeo"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setOrganizationName(QStringLiteral("ApoNeo"));

    ApoNeo::UI::MainWindow win;
    win.show();

    return app.exec();
}
