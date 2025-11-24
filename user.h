#ifndef USER_H
#define USER_H
#include <QString>

class User {
public:
    enum Role { Client, Manager };

    User(const QString& login, const QString& password, Role role);
    QString login() const;
    QString password() const;
    Role role() const;

private:
    QString m_login;
    QString m_password;
    Role m_role;
};

#endif 
