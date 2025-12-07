#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <QString>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

class ModelManager {
public:
    struct Model {
        QString name;
        int type;

        Model(const QString& n = "", int t = 0) : name(n), type(t) {}
    };

    static ModelManager& instance();
    void load();
    void save();
    QList<Model> getModels(int deviceType) const;
    void addModel(const QString& name, int type);
    void removeModel(const QString& name, int type);
    QList<Model> m_models;

private:
    ModelManager() = default;
};

#endif
