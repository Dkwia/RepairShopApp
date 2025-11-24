#ifndef ORDEROBSERVER_H
#define ORDEROBSERVER_H
#include <QString>

class OrderObserver {
public:
    virtual ~OrderObserver() = default;
    virtual void onOrderStatusChanged(const QString& orderId, const QString& newStatus, const QString& clientId) = 0;
};
#endif 
