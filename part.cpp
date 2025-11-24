#include "part.h"

Part::Part(const QString& article, const QString& name, int quantity, double price)
    : m_article(article), m_name(name), m_quantity(quantity), m_price(price) {}

QString Part::article() const {
    return m_article;
}

QString Part::name() const {
    return m_name;
}

int Part::quantity() const {
    return m_quantity;
}

void Part::setQuantity(int q) {
    m_quantity = q;
}

void Part::use(int amount) {
    if (amount > 0 && m_quantity >= amount) {
        m_quantity -= amount;
    }
}

double Part::price() const {
    return m_price;
}

void Part::setPrice(double p) {
    m_price = p;
}
