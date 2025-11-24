#include "device.h"

Device::Device(Type type, const QString& model)
    : m_type(type), m_model(model) {}

Device::Type Device::type() const {
    return m_type;
}

QString Device::model() const {
    return m_model;
}

QString Device::typeName() const {
    switch (m_type) {
    case Laptop: return "Ноутбук";
    case Phone:  return "Телефон";
    case Tablet: return "Планшет";
    default:     return "Неизвестное устройство";
    }
}
