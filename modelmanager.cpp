#include "modelmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QDir>

ModelManager& ModelManager::instance() {
    static ModelManager inst;
    return inst;
}

void ModelManager::load() {
    QString filePath = QDir::currentPath() + "/models.json";
    QFile file(filePath);
    if (!file.exists()) return;

    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!doc.isObject()) return;
    QJsonObject root = doc.object();

    m_models.clear();
    if (root.contains("models") && root["models"].isArray()) {
        for (const QJsonValue& val : root["models"].toArray()) {
            QJsonObject obj = val.toObject();
            QString name = obj["name"].toString();
            int type = obj["type"].toInt();
            if (!name.isEmpty()) {
                m_models.append(Model(name, type));
            }
        }
    }
}

void ModelManager::save() {
    QString filePath = QDir::currentPath() + "/models.json";
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) return;

    QJsonArray modelsArray;
    for (const Model& m : m_models) {
        QJsonObject obj;
        obj["name"] = m.name;
        obj["type"] = m.type;
        modelsArray.append(obj);
    }

    QJsonObject root;
    root["models"] = modelsArray;

    QJsonDocument doc(root);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
}

QList<ModelManager::Model> ModelManager::getModels(int deviceType) const {
    QList<Model> result;
    for (const Model& m : m_models) {
        if (deviceType == -1 || m.type == deviceType) {
            result.append(m);
        }
    }
    return result;
}

void ModelManager::addModel(const QString& name, int type) {
    for (const Model& m : m_models) {
        if (m.name == name && m.type == type) return;
    }
    m_models.append(Model(name, type));
    save();
}

void ModelManager::removeModel(const QString& name, int type) {
    for (auto it = m_models.begin(); it != m_models.end(); ++it) {
        if (it->name == name && it->type == type) {
            m_models.erase(it);
            save();
            return;
        }
    }
}
