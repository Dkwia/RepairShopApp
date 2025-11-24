#include "loginwindow.h"
#include "mainwindow.h"
#include "datastorage.h"
#include <QApplication>
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    DataStorage::instance().load();

    LoginWindow login;
    if (login.exec() == QDialog::Accepted) {
        if (login.getUsername().isEmpty()) {
            return -1;
        }
        MainWindow w(login.getUserRole(), login.getUsername());
        w.show();
        return app.exec();
    }

    return 0;
}
