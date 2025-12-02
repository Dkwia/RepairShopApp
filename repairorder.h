#ifndef REPAIRORDER_H
#define REPAIRORDER_H
#include <QString>
#include <QDateTime>
#include <QList>
#include "device.h"
#include "part.h"
#include "statusstrategy.h"
#include "orderobserver.h"



class RepairOrder {
public:
    RepairOrder(const QString& id, const QString& clientId,
                const Device& device, const QString& issue);

    QString id() const;
    QString clientId() const;
    Device device() const;
    QString issue() const;
    void usePart(const Part& part);
    QString currentStatus() const;
    void setCreatedAt(const QDateTime& dt);
    QDateTime createdAt() const;
    QList<Part> usedParts() const;
    void setStrategy(StatusStrategy* strategy);
    QList<Part>& usedPartsRef() { return m_usedParts; }
    void addObserver(OrderObserver* obs);
    void notifyObservers();
    double calculateTotal() const; // ← новое
    // RepairOrder(RepairOrder&& other) noexcept;
    // RepairOrder& operator=(RepairOrder&& other) noexcept;
    // RepairOrder(RepairOrder&& other) noexcept;
    // RepairOrder& operator=(RepairOrder&& other) noexcept;

private:
    QString m_id;
    QString m_clientId;
    Device m_device;
    QString m_statusName;
    QString m_issue;
    QDateTime m_createdAt;
    QList<Part> m_usedParts;
    StatusStrategy* m_strategy = nullptr;
    QList<OrderObserver*> m_observers;
};

#endif 
