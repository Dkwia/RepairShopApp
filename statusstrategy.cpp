#include "statusstrategy.h"
#include "repairorder.h"
#include "datastorage.h"
#include <QDebug>
#include <QMessageBox>


void WaitingStrategy::handle(RepairOrder* order) {
    order->notifyObservers();
}

void AcceptedStrategy::handle(RepairOrder* order) {
    order->notifyObservers();
}

void InProgressStrategy::handle(RepairOrder* order) {
    auto& storageParts = DataStorage::instance().parts();
    bool success = true;
    QString errorMsg;

    for (const Part& used : order->usedParts()) {
        bool found = false;
        for (Part& stockPart : storageParts) {
            if (stockPart.article() == used.article()) {
                if (stockPart.quantity() >= used.quantity()) {
                    stockPart.use(used.quantity());
                    found = true;
                } else {
                    success = false;
                    errorMsg = QString("Недостаточно запчастей: %1 (нужно %2, есть %3)")
                                   .arg(used.name()).arg(used.quantity()).arg(stockPart.quantity());
                }
                break;
            }
        }
        if (!found && used.quantity() > 0) {
            success = false;
            errorMsg = QString("Запчасть не найдена: %1").arg(used.article());
        }
        if (!success) break;
    }

    if (success) {
        order->notifyObservers();
        DataStorage::instance().save();
    } else {
        order->setStrategy(new AcceptedStrategy());
        QMessageBox::critical(nullptr, "Ошибка", "Невозможно начать ремонт: " + errorMsg);
    }
}

void ReadyStrategy::handle(RepairOrder* order) {
    order->notifyObservers();
}

void IssuedStrategy::handle(RepairOrder* order) {
    order->notifyObservers();
}
