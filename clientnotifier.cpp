#include "clientnotifier.h"
#include <QDebug>

ClientNotifier::ClientNotifier(const QString& currentClient, QObject* parent)
    : QObject(parent), m_currentClient(currentClient) {}

void ClientNotifier::onOrderStatusChanged(const QString& orderId, const QString& newStatus, const QString& clientId) {
    qDebug() << "Уведомление получено:" << orderId << newStatus << "для клиента:" << clientId << "я:" << m_currentClient;
    if (clientId == m_currentClient) {
        QString msg = QString("🔔 Уведомление: статус вашего заказа %1 изменён на «%2»")
                          .arg(orderId).arg(newStatus);
        qDebug() << msg;
        emit orderStatusChanged(msg);
    }
}
