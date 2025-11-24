#include "loginwindow.h"
#include "mainwindow.h"
#include "datastorage.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    DataStorage::instance().load();

    while (true) {
        LoginWindow login;
        if (login.exec() != QDialog::Accepted) break;

        MainWindow w(login.getUserRole(), login.getUsername());
        QObject::connect(&w, &MainWindow::logoutRequested, &app, [&]() {
            w.close();
        });

        w.show();
        if (app.exec() != 0) break;
    }

    return 0;
}
