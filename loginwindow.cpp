#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "datastorage.h"
#include "statustracker.h"
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent), ui(new Ui::LoginWindow) {
    ui->setupUi(this);
    setWindowTitle("Вход в систему");
}

LoginWindow::~LoginWindow() {
    delete ui;
}

User::Role LoginWindow::getUserRole() const {
    return m_role;
}

QString LoginWindow::getUsername() const {
    return m_username;
}

void LoginWindow::on_loginButton_clicked() {
    QString login = ui->loginEdit->text().trimmed();
    QString password = ui->passEdit->text();

    if (login.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        return;
    }

    auto& users = DataStorage::instance().users();
    for (const User& user : users) {
        if (user.login() == login && user.password() == password) {
            m_username = login;
            m_role = user.role();
            accept();
            return;
        }
    }

    QMessageBox::warning(this, "Ошибка", "Неверный логин или пароль");
}
