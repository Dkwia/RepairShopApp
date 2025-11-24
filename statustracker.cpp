#include "statustracker.h"
#include <QDebug>

StatusTracker::StatusTracker(QObject *parent)
    : QObject(parent) {}

void StatusTracker::onOrderStatusChanged(const QString& orderId, const QString& newStatus, const QString& /*clientId*/) {
    QString logEntry = QString("Заказ %1 перешёл в статус '%2'").arg(orderId).arg(newStatus);
    m_log.append(logEntry);
    qDebug() << logEntry;
}

void StatusTracker::printStatusLog() const {
    for (const QString& entry : m_log) {
        qDebug() << entry;
    }
}
