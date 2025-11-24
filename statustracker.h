#ifndef STATUSTRACKER_H
#define STATUSTRACKER_H

#include "orderobserver.h"
#include <QList>
#include <QString>
#include <QObject>  

class StatusTracker : public QObject, public OrderObserver {  
    Q_OBJECT  

public:
    explicit StatusTracker(QObject *parent = nullptr);  

    void onOrderStatusChanged(const QString& orderId, const QString& newStatus, const QString& clientId) override;
    void printStatusLog() const;

private:
    QList<QString> m_log;
};

#endif 
