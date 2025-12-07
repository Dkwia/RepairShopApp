#include <QApplication> 
#include <datastorage.h>
#include <repairorder.h>
#include <QDir>

DataStorage& DataStorage::instance() {
    static DataStorage inst;
    return inst;
}

void DataStorage::load() {
    QString filePath = QDir::currentPath() + "/data.json";
    QFile file(filePath);

    if (!file.exists()) {
        qDebug() << "Файл data.json не найден в корне проекта. Будет создан при первом сохранении.";
        return;
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Не удалось открыть data.json для чтения:" << file.errorString();
        return;
    }

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    file.close();

    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "Ошибка парсинга JSON:" << parseError.errorString();
        return;
    }

    if (!doc.isObject()) return;
    QJsonObject root = doc.object();

    m_users.clear();
    if (root.contains("users") && root["users"].isArray()) {
        for (const QJsonValue& val : root["users"].toArray()) {
            QJsonObject uObj = val.toObject();
            QString login = uObj["login"].toString();
            QString pass = uObj["password"].toString();
            int roleInt = uObj["role"].toInt();
            User::Role role = static_cast<User::Role>(roleInt);
            m_users.append(User(login, pass, role));
        }
    }

    m_parts.clear();
    if (root.contains("parts") && root["parts"].isArray()) {
        for (const QJsonValue& val : root["parts"].toArray()) {
            QJsonObject pObj = val.toObject();
            QString article = pObj["article"].toString();
            QString name = pObj["name"].toString();
            int quantity = pObj["quantity"].toInt();
            double price = pObj["price"].toDouble(); // ← добавьте
            m_parts.append(Part(article, name, quantity, price));
        }
    }

    m_orders.clear();
    if (root.contains("orders") && root["orders"].isArray()) {
        for (const QJsonValue& val : root["orders"].toArray()) {
            QJsonObject oObj = val.toObject();
            QString id = oObj["id"].toString();
            QString clientId = oObj["clientId"].toString();
            QString issue = oObj["issue"].toString();
            QString status = oObj["status"].toString();
            QString createdAtStr = oObj["createdAt"].toString();


            QJsonObject devObj = oObj["device"].toObject();
            Device::Type devType = static_cast<Device::Type>(devObj["type"].toInt());
            QString model = devObj["model"].toString();
            Device device(devType, model);

            m_orders.append(RepairOrder(id, clientId, device, issue));
            RepairOrder& order = m_orders.back();
            order.setCreatedAt(QDateTime::fromString(createdAtStr, Qt::ISODate));

            if (oObj.contains("usedParts") && oObj["usedParts"].isArray()) {
                for (const QJsonValue& pVal : oObj["usedParts"].toArray()) {
                    QJsonObject p = pVal.toObject();
                    Part part(p["article"].toString(), p["name"].toString(), p["quantity"].toInt());
                    order.usePart(part);
                }
            }

            StatusStrategy* strat = nullptr;
            if (status == "Принят")        strat = new AcceptedStrategy();
            else if (status == "В работе") strat = new InProgressStrategy();
            else if (status == "Готов")    strat = new ReadyStrategy();
            else if (status == "Выдан")    strat = new IssuedStrategy();
            else                           strat = new WaitingStrategy();

            order.setStrategy(strat);
            // m_orders.append(order);
        }
    }
}

void DataStorage::save() {
    QString filePath = QApplication::applicationDirPath() + "/data.json";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Не удалось открыть data.json для записи:" << file.errorString();
        return;
    }

    QJsonObject root;

    QJsonArray usersArr;
    const auto& users = m_users;
    for (const User& u : users) {
        QJsonObject uObj;
        uObj["login"] = u.login();
        uObj["password"] = u.password();
        uObj["role"] = static_cast<int>(u.role());
        usersArr.append(uObj);
    }
    root["users"] = usersArr;

    QJsonArray partsArr;
    for (const Part& p : m_parts) {
        QJsonObject pObj;
        pObj["article"] = p.article();
        pObj["name"] = p.name();
        pObj["quantity"] = p.quantity();
        pObj["price"] = p.price();
        partsArr.append(pObj);
    }
    root["parts"] = partsArr;

    QJsonArray ordersArr;
    for (const RepairOrder& order : m_orders) {
        QJsonObject oObj;
        oObj["id"] = order.id();
        oObj["clientId"] = order.clientId();
        oObj["issue"] = order.issue();
        oObj["status"] = order.currentStatus();
        oObj["createdAt"] = order.createdAt().toString(Qt::ISODate);

        QJsonObject devObj;
        devObj["type"] = static_cast<int>(order.device().type());
        devObj["model"] = order.device().model();
        oObj["device"] = devObj;

        QJsonArray usedPartsArr;
        for (const Part& p : order.usedParts()) {
            QJsonObject pObj;
            pObj["article"] = p.article();
            pObj["name"] = p.name();
            pObj["quantity"] = p.quantity();
            usedPartsArr.append(pObj);
        }
        oObj["usedParts"] = usedPartsArr;

        ordersArr.append(oObj);
    }
    root["orders"] = ordersArr;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

QList<User>& DataStorage::users() {
    return m_users;
}

QList<RepairOrder>& DataStorage::orders() {
    return m_orders;
}

QList<Part>& DataStorage::parts() {
    return m_parts;
}
