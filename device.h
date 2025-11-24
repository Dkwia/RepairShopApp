#ifndef DEVICE_H
#define DEVICE_H
#include <QString>

class Device {
public:
    enum Type { Laptop, Phone, Tablet };

    Device(Type type, const QString& model);
    Type type() const;
    QString model() const;
    QString typeName() const;

private:
    Type m_type;
    QString m_model;
};

#endif
