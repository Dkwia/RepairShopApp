#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include "user.h"

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QDialog {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

    User::Role getUserRole() const;
    QString getUsername() const;

private slots:
    void on_loginButton_clicked();
    void on_btnRegister_clicked();

private:
    Ui::LoginWindow *ui;
    User::Role m_role = User::Client;
    QString m_username;
};

#endif
