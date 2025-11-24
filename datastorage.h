#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include "user.h"
#include "repairorder.h"
#include "part.h"

class DataStorage {
public:
    static DataStorage& instance();

    void load();
    void save();

    QList<User>& users();
    QList<RepairOrder>& orders();
    QList<Part>& parts();

private:
    DataStorage() = default;
    DataStorage(const DataStorage&) = delete;
    DataStorage& operator=(const DataStorage&) = delete;

    QList<User> m_users;
    QList<RepairOrder> m_orders;
    QList<Part> m_parts;
};

#endif
