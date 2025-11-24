#include "user.h"

User::User(const QString& login, const QString& password, Role role)
    : m_login(login), m_password(password), m_role(role) {}

QString User::login() const {
    return m_login;
}

QString User::password() const {
    return m_password;
}

User::Role User::role() const {
    return m_role;
}
