#include "repairorder.h"
#include "statusstrategy.h"
#include <QDateTime>
#include <datastorage.h>

#include "repairorder.h"
#include "statusstrategy.h"
#include <QDateTime>

RepairOrder::RepairOrder(const QString& id, const QString& clientId,
                         const Device& device, const QString& issue)
    : m_id(id)
    , m_clientId(clientId)
    , m_device(device)
    , m_issue(issue)
    , m_createdAt(QDateTime::currentDateTime())
    , m_strategy(nullptr)
{
    setStrategy(new WaitingStrategy());
}

/*RepairOrde/*r::RepairOrder(RepairOrder&& other) noexcept
    : m_id(std::move(other.m_id))
    , m_clientId(std::move(other.m_clientId))
    , m_device(std::move(other.m_device))
    , m_issue(std::move(other.m_issue))
    , m_createdAt(std::move(other.m_createdAt))
    , m_usedParts(std::move(other.m_usedParts))
    , m_strategy(other.m_strategy)
    , m_observers(std::move(other.m_observers))
{
    other.m_strategy = nullptr;
}*/

/*RepairO/*rder& RepairOrder::operator=(RepairOrder&& other) noexcept {
    if (this != &other) {
        delete m_strategy;

        m_id = std::move(other.m_id);
        m_clientId = std::move(other.m_clientId);
        m_device = std::move(other.m_device);
        m_issue = std::move(other.m_issue);
        m_createdAt = std::move(other.m_createdAt);
        m_usedParts = std::move(other.m_usedParts);
        m_strategy = other.m_strategy;
        m_observers = std::move(other.m_observers);

        other.m_strategy = nullptr;
    }
    return *this;
}*/

QString RepairOrder::id() const {
    return m_id;
}

QString RepairOrder::clientId() const {
    return m_clientId;
}

Device RepairOrder::device() const {
    return m_device;
}

QString RepairOrder::issue() const {
    return m_issue;
}

QString RepairOrder::currentStatus() const {
    return m_statusName;
}

QDateTime RepairOrder::createdAt() const {
    return m_createdAt;
}

QList<Part> RepairOrder::usedParts() const {
    return m_usedParts;
}

void RepairOrder::usePart(const Part& part) {
    m_usedParts.append(part);
}

void RepairOrder::addObserver(OrderObserver* obs) {
    m_observers.append(obs);
}

void RepairOrder::setStrategy(StatusStrategy* strategy) {
    if (m_strategy) delete m_strategy;
    m_strategy = strategy;
    m_statusName = m_strategy ? m_strategy->statusName() : "Неизвестно";
    notifyObservers();
}

void RepairOrder::notifyObservers() {
    for (auto* obs : m_observers) {
        obs->onOrderStatusChanged(m_id, currentStatus(), m_clientId);
    }
}

void RepairOrder::setCreatedAt(const QDateTime& dt) {
    m_createdAt = dt;
}

double RepairOrder::calculateTotal() const {
    double total = 0.0;
    const auto& stockParts = DataStorage::instance().parts();

    for (const Part& used : m_usedParts) {
        for (const Part& stock : stockParts) {
            if (stock.article() == used.article()) {
                total += stock.price() * used.quantity();
                break;
            }
        }
    }
    return total;
}
