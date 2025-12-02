#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "datastorage.h"
#include "statustracker.h"
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QRegularExpression>
#include "datastorage.h"

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

void LoginWindow::on_btnRegister_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Регистрация клиента");
    QFormLayout layout(&dialog);

    QLineEdit loginEdit;
    QLineEdit passEdit;
    QLineEdit passConfirmEdit;

    loginEdit.setPlaceholderText("Логин (только латиница и цифры)");
    passEdit.setPlaceholderText("Пароль");
    passConfirmEdit.setPlaceholderText("Подтвердите пароль");
    passEdit.setEchoMode(QLineEdit::Password);
    passConfirmEdit.setEchoMode(QLineEdit::Password);

    layout.addRow("Логин:", &loginEdit);
    layout.addRow("Пароль:", &passEdit);
    layout.addRow("Повтор пароля:", &passConfirmEdit);

    QPushButton ok("Зарегистрироваться");
    QPushButton cancel("Отмена");
    layout.addRow(&ok, &cancel);

    QObject::connect(&cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    QString login = loginEdit.text().trimmed();
    QString pass = passEdit.text();
    QString passConfirm = passConfirmEdit.text();

    if (login.isEmpty() || pass.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Заполните все поля");
        return;
    }

    if (pass != passConfirm) {
        QMessageBox::warning(this, "Ошибка", "Пароли не совпадают");
        return;
    }

    if (!login.contains(QRegularExpression("^[a-zA-Z0-9_]+$"))) {
        QMessageBox::warning(this, "Ошибка", "Логин может содержать только латинские буквы, цифры и подчёркивание");
        return;
    }

    auto& users = DataStorage::instance().users();
    for (const User& user : users) {
        if (user.login() == login) {
            QMessageBox::warning(this, "Ошибка", "Логин уже занят");
            return;
        }
    }

    users.append(User(login, pass, User::Client));
    DataStorage::instance().save();

    QMessageBox::information(this, "Успех", "Регистрация прошла успешно!\nТеперь вы можете войти.");
}
