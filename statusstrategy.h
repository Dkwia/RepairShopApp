#ifndef STATUSSTRATEGY_H
#define STATUSSTRATEGY_H
#include <QString>

class RepairOrder;

class StatusStrategy {
public:
    virtual ~StatusStrategy() = default;
    virtual QString statusName() const = 0;
    virtual void handle(RepairOrder* order) = 0;
};

class AcceptedStrategy : public StatusStrategy {
public:
    QString statusName() const override { return "Принят"; }
    void handle(RepairOrder* order) override;
};

class InProgressStrategy : public StatusStrategy {
public:
    QString statusName() const override { return "В работе"; }
    void handle(RepairOrder* order) override;
};

class ReadyStrategy : public StatusStrategy {
public:
    QString statusName() const override { return "Готов"; }
    void handle(RepairOrder* order) override;
};

class IssuedStrategy : public StatusStrategy {
public:
    QString statusName() const override { return "Выдан"; }
    void handle(RepairOrder* order) override;
};

#endif
