#ifndef CLIENTNOTIFIER_H
#define CLIENTNOTIFIER_H

#include "orderobserver.h"
#include <QObject>

class ClientNotifier : public QObject, public OrderObserver {
    Q_OBJECT

public:
    ClientNotifier(const QString& currentClient, QObject* parent = nullptr);

    void onOrderStatusChanged(const QString& orderId, const QString& newStatus, const QString& clientId) override;

signals:
    void orderStatusChanged(const QString& message);

private:
    QString m_currentClient;
};

#endif
