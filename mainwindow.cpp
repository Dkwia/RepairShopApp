#include <QTextStream>
#include <QDateTime>
#include <QHBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include "mainwindow.h"
#include "statustracker.h"
#include "ui_mainwindow.h"
#include "datastorage.h"
#include <QTableWidget>
#include <QTextEdit>
#include <QHeaderView>
#include <QInputDialog>
#include <QMessageBox>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QSpinBox>
#include <QJsonObject>

MainWindow::MainWindow(User::Role role, const QString& username, QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_role(role),
    m_username(username) {
    ui->setupUi(this);
    connect(ui->tableParts, &QTableWidget::doubleClicked, this, &MainWindow::on_tableParts_doubleClicked);
    setWindowTitle(QString("Ремонтная мастерская — %1 (%2)")
                       .arg(username)
                       .arg(role == User::Client ? "Клиент" : "Менеджер"));

    updateOrdersList();


    if (m_role == User::Client) {
        m_clientNotifier = new ClientNotifier(m_username, this);
        int count = 0;
        for (auto& order : DataStorage::instance().orders()) {
            if (order.clientId() == m_username) { 
                order.addObserver(m_clientNotifier);
            }
        }
        for (const auto& order : DataStorage::instance().orders()) {
            if (order.clientId() == m_username) {
                QString msg = QString("Статус заказа %1: %2").arg(order.id()).arg(order.currentStatus());
                statusBar()->showMessage(msg, 3000);
            }
        }
        qDebug() << "Клиент" << m_username << "подписан на" << count << "заказов";
    }
    if (m_clientNotifier) {
        connect(m_clientNotifier, &ClientNotifier::orderStatusChanged,
                this, &MainWindow::onOrderStatusChanged);
    }

    m_statusTracker = new StatusTracker(this); 

    if (role == User::Client) {
        setupClientView();
    } else {
        setupManagerView();
    }

    QTableWidget* partsTable = ui->tableParts;
    partsTable->setRowCount(0);
    auto& parts = DataStorage::instance().parts();
    partsTable->setRowCount(parts.size());
    for (int i = 0; i < parts.size(); ++i) {
        const Part& p = parts[i];
        partsTable->setItem(i, 0, new QTableWidgetItem(p.article()));
        partsTable->setItem(i, 1, new QTableWidgetItem(p.name()));
        QTableWidgetItem* qtyItem = new QTableWidgetItem(QString::number(p.quantity()));
        qtyItem->setData(Qt::EditRole, p.quantity());
        partsTable->setItem(i, 2, qtyItem);
        QTableWidgetItem* priceItem = new QTableWidgetItem(QString::number(p.price(), 'f', 2));
        priceItem->setData(Qt::EditRole, p.price());
        partsTable->setItem(i, 3, priceItem);
    }
    ui->tableParts->setSortingEnabled(true);
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::setupClientView() {
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabParts));
    ui->tabWidget->removeTab(ui->tabWidget->indexOf(ui->tabAllOrders));
    ui->actionExportHistory->setVisible(false);
}

void MainWindow::setupManagerView() {
}

void MainWindow::updateOrdersList() {
    auto& orders = DataStorage::instance().orders();

    QTableWidget* table = (m_role == User::Client) ? ui->tableMyOrders : ui->tableAllOrders;
    table->setRowCount(0);
    table->setSortingEnabled(false);

    if (m_role == User::Client) {
        table->setColumnCount(3);
        table->setHorizontalHeaderLabels({"Устройство", "Статус", "Дата"});
    } else {
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels({"ID", "Клиент", "Устройство", "Статус", "Дата"});
    }

    int row = 0;
    for (const auto& order : orders) {
        if (m_role == User::Client && order.clientId() != m_username)
            continue;

        table->insertRow(row);

        QTableWidgetItem* deviceItem = new QTableWidgetItem(
            QString("%1 (%2)").arg(order.device().typeName()).arg(order.device().model())
            );
        deviceItem->setData(Qt::UserRole, order.id());

        if (m_role == User::Client) {
            table->setItem(row, 0, deviceItem);
            table->setItem(row, 1, new QTableWidgetItem(order.currentStatus()));

            QTableWidgetItem* dateItem = new QTableWidgetItem(order.createdAt().toString("dd.MM.yyyy"));
            dateItem->setData(Qt::EditRole, order.createdAt().toString("yyyy-MM-dd"));
            table->setItem(row, 2, dateItem);
        } else {
            table->setItem(row, 0, new QTableWidgetItem(order.id()));
            table->setItem(row, 1, new QTableWidgetItem(order.clientId()));
            table->setItem(row, 2, deviceItem);
            table->setItem(row, 3, new QTableWidgetItem(order.currentStatus()));

            QTableWidgetItem* dateItem = new QTableWidgetItem(order.createdAt().toString("dd.MM.yyyy"));
            dateItem->setData(Qt::EditRole, order.createdAt().toString("yyyy-MM-dd"));
            table->setItem(row, 4, dateItem);
        }
        row++;
    }
    table->setSortingEnabled(true);
    table->horizontalHeader()->setStretchLastSection(true);
}

void MainWindow::on_btnChangeStatus_clicked() {
    if (m_role != User::Manager) return;

    QTableWidget* table = ui->tableAllOrders;
    QList<QTableWidgetSelectionRange> sel = table->selectedRanges();
    if (sel.isEmpty()) {
        QMessageBox::warning(this, "Внимание", "Выберите заказ");
        return;
    }

    int row = sel.first().topRow();
    QString orderId = table->item(row, 0)->text();

    auto& orders = DataStorage::instance().orders();
    RepairOrder* targetOrder = nullptr;
    for (auto& order : orders) {
        if (order.id() == orderId) {
            targetOrder = &order;
            break;
        }
    }

    if (!targetOrder) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Изменить статус");
    QFormLayout layout(&dialog);
    QComboBox combo;
    combo.addItems({"Принят", "В работе", "Готов", "Выдан"});
    combo.setCurrentText(targetOrder->currentStatus());
    layout.addRow("Новый статус:", &combo);

    QPushButton okBtn("OK");
    QPushButton cancelBtn("Отмена");
    layout.addRow(&okBtn, &cancelBtn);
    QObject::connect(&cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(&okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    QString newStatus = combo.currentText();
    StatusStrategy* newStrategy = nullptr;

    if (newStatus == "Принят") newStrategy = new AcceptedStrategy();
    else if (newStatus == "В работе") newStrategy = new InProgressStrategy();
    else if (newStatus == "Готов") newStrategy = new ReadyStrategy();
    else if (newStatus == "Выдан") newStrategy = new IssuedStrategy();

    if (newStrategy) {
        targetOrder->setStrategy(newStrategy);
        updateOrdersList(); 
        DataStorage::instance().save(); 
    }
}

void MainWindow::on_btnNewOrder_clicked() {
    if (m_role != User::Client) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Новый заказ");
    QFormLayout layout(&dialog);

    QLineEdit clientEdit(m_username);
    clientEdit.setReadOnly(true);
    QLineEdit deviceModel;
    QComboBox deviceType;
    deviceType.addItems({"Ноутбук", "Телефон", "Планшет"});
    QLineEdit issue;

    layout.addRow("Клиент:", &clientEdit);
    layout.addRow("Тип устройства:", &deviceType);
    layout.addRow("Модель:", &deviceModel);
    layout.addRow("Неисправность:", &issue);

    QPushButton ok("Создать");
    QPushButton cancel("Отмена");
    layout.addRow(&ok, &cancel);

    QObject::connect(&cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    auto& orders = DataStorage::instance().orders();
    QString id = "ORD-" + QString::number(orders.size() + 1);

    Device::Type type = static_cast<Device::Type>(deviceType.currentIndex());
    Device device(type, deviceModel.text());

    RepairOrder order(id, m_username, device, issue.text());
    orders.append(order);
    DataStorage::instance().save();

    updateOrdersList();
    QMessageBox::information(nullptr, "Успех", "Заказ оформлен: " + id);
}

void MainWindow::on_btnAddPart_clicked() {
    QDialog dialog(this);
    dialog.setWindowTitle("Добавить запчасть");
    QFormLayout layout(&dialog);

    QLineEdit article, name;
    QSpinBox quantity;
    quantity.setRange(0, 1000);

    layout.addRow("Артикул:", &article);
    layout.addRow("Наименование:", &name);
    layout.addRow("Количество:", &quantity);

    QPushButton ok("Добавить");
    QPushButton cancel("Отмена");
    layout.addRow(&ok, &cancel);

    QObject::connect(&cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    auto& parts = DataStorage::instance().parts();
    parts.append(Part(article.text(), name.text(), quantity.value()));
    DataStorage::instance().save();

    QTableWidget* table = ui->tableParts;
    table->setRowCount(parts.size());
    const Part& p = parts.last();
    int i = parts.size() - 1;
    table->setItem(i, 0, new QTableWidgetItem(p.article()));
    table->setItem(i, 1, new QTableWidgetItem(p.name()));
    table->setItem(i, 2, new QTableWidgetItem(QString::number(p.quantity())));
}

void MainWindow::on_btnRemovePart_clicked() {
    QTableWidget* table = ui->tableParts;
    auto sel = table->selectedRanges();
    if (sel.isEmpty()) {
        QMessageBox::warning(nullptr, "Ошибка", "Выберите запчасть");
        return;
    }

    int row = sel.first().topRow();
    auto& parts = DataStorage::instance().parts();
    if (row >= 0 && row < parts.size()) {
        parts.removeAt(row);
        DataStorage::instance().save();
        table->removeRow(row);
    }
}

void MainWindow::on_actionExportHistory_triggered() {
    QString filePath = QFileDialog::getSaveFileName(
        nullptr, "Экспорт истории заказов", "", "CSV (*.csv)"
        );
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".csv")) filePath += ".csv";

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Ошибка создания файла:" << file.errorString();
        return;
    }

    QTextStream out(&file);
    out.setGenerateByteOrderMark(true);
    out << "\"ID\",\"Клиент\",\"Устройство\",\"Неисправность\",\"Статус\",\"Дата создания\"\n";

    for (const auto& order : DataStorage::instance().orders()) {
        if (m_role == User::Client && order.clientId() != m_username)
            continue;

        QString line = QString("\"%1\",\"%2\",\"%3 (%4)\",\"%5\",\"%6\",\"%7\"")
                           .arg(order.id())
                           .arg(order.clientId())
                           .arg(order.device().typeName())
                           .arg(order.device().model())
                           .arg(order.issue())
                           .arg(order.currentStatus())
                           .arg(order.createdAt().toString("yyyy-MM-dd HH:mm"));
        out << line << "\n";
    }

    file.close();
    qDebug() << "Экспорт завершён:" << filePath;
}

void MainWindow::on_btnInvoice_clicked()
{
    generateInvoiceForSelectedOrder(ui->tableMyOrders);
}

void MainWindow::on_btnInvoice_2_clicked()
{
    generateInvoiceForSelectedOrder(ui->tableAllOrders);
}

void MainWindow::generateInvoiceForSelectedOrder(QTableWidget* table)
{
    auto selection = table->selectedRanges();
    if (selection.isEmpty()) {
        qDebug() << "Пустой выбор";
        return;
    }

    int row = selection.first().topRow();
    if (row < 0 || row >= table->rowCount()) return;

    QTableWidgetItem* deviceItem = nullptr;
    if (m_role == User::Client) {
        deviceItem = table->item(row, 0);
    } else {
        deviceItem = table->item(row, 2);
    }

    if (!deviceItem) return;

    QString orderId = deviceItem->data(Qt::UserRole).toString();
    if (orderId.isEmpty()) {
        qDebug() << "ID заказа не найден!";
        return;
    }

    RepairOrder* selectedOrder = nullptr;
    for (auto& order : DataStorage::instance().orders()) {
        if (order.id() == orderId) {
            selectedOrder = &order;
            break;
        }
    }
    if (!selectedOrder) return;

    QString invoice = "=== СЧЁТ НА РЕМОНТ ===\n";
    invoice += QString("Заказ: %1\n").arg(selectedOrder->id());
    invoice += QString("Клиент: %1\n").arg(selectedOrder->clientId());
    invoice += QString("Устройство: %1 (%2)\n")
                   .arg(selectedOrder->device().typeName())
                   .arg(selectedOrder->device().model());
    invoice += QString("Статус: %1\n").arg(selectedOrder->currentStatus());
    invoice += QString("Дата: %1\n\n").arg(selectedOrder->createdAt().toString("dd.MM.yyyy"));

    invoice += "Использованы запчасти:\n";
    double total = 0.0;
    const auto& stockParts = DataStorage::instance().parts();

    for (const Part& used : selectedOrder->usedParts()) {
        double price = 0.0;
        for (const Part& stock : stockParts) {
            if (stock.article() == used.article()) {
                price = stock.price();
                break;
            }
        }
        double lineTotal = price * used.quantity();
        total += lineTotal;
        invoice += QString("- %1 (x%2) — %3 ₽\n")
                       .arg(used.name())
                       .arg(used.quantity())
                       .arg(lineTotal, 0, 'f', 2);
    }

    invoice += "\n";
    invoice += QString("ИТОГО: %1 ₽\n").arg(total, 0, 'f', 2);
    invoice += "======================";

    QDialog dialog(nullptr);
    dialog.setWindowTitle("Счёт на оплату");
    dialog.resize(500, 400);

    QVBoxLayout layout(&dialog);
    QTextEdit textEdit;
    textEdit.setReadOnly(true);
    textEdit.setFont(QFont("Monospace", 12));
    textEdit.setText(invoice);
    layout.addWidget(&textEdit);

    QPushButton okBtn("OK");
    layout.addWidget(&okBtn);
    QObject::connect(&okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::on_searchEdit_textChanged(const QString &text) {
    QTableWidget* table = ui->tableMyOrders;
    for (int i = 0; i < table->rowCount(); ++i) {
        bool show = false;
        for (int j = 0; j < table->columnCount(); ++j) {
            auto* item = table->item(i, j);
            if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                show = true;
                break;
            }
        }
        table->setRowHidden(i, !show);
    }
}

void MainWindow::on_searchEdit_2_textChanged(const QString &text) {
    QTableWidget* table = ui->tableAllOrders;
    for (int i = 0; i < table->rowCount(); ++i) {
        bool show = false;
        for (int j = 0; j < table->columnCount(); ++j) {
            auto* item = table->item(i, j);
            if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                show = true;
                break;
            }
        }
        table->setRowHidden(i, !show);
    }
}

void MainWindow::on_tableParts_doubleClicked(const QModelIndex &index) {
    if (m_role != User::Manager) return;

    int row = index.row();
    QTableWidget* table = ui->tableParts;
    if (row < 0 || row >= table->rowCount()) return;

    QTableWidgetItem* item = table->item(row, 0); 
    if (!item) return;

    QString article = item->text();
    auto& parts = DataStorage::instance().parts();
    for (auto& part : parts) {
        if (part.article() == article) {
            bool ok;
            double newPrice = QInputDialog::getDouble(
                this,
                "Изменить цену",
                "Новая цена (₽):",
                part.price(),
                0.0,
                999999.99,
                2,
                &ok
                );
            if (ok) {
                part.setPrice(newPrice);
                table->setItem(row, 3, new QTableWidgetItem(QString::number(newPrice, 'f', 2)));
                DataStorage::instance().save();
            }
            break;
        }
    }
}

void MainWindow::on_btnExportMyOrders_clicked() {
    exportOrders(/* clientOnly = */ true);
}

void MainWindow::on_btnExportAllOrders_clicked() {
    exportOrders(/* clientOnly = */ false);
}

void MainWindow::exportOrders(bool clientOnly) {
    QString filePath = QFileDialog::getSaveFileName(
        nullptr, "Экспорт истории заказов", "", "CSV (*.csv)"
        );
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".csv")) filePath += ".csv";

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "Ошибка создания файла:" << file.errorString();
        return;
    }

    QTextStream out(&file);
    out.setGenerateByteOrderMark(true);
    out << "\"ID\",\"Клиент\",\"Устройство\",\"Неисправность\",\"Статус\",\"Дата создания\"\n";

    for (const auto& order : DataStorage::instance().orders()) {
        if (clientOnly && order.clientId() != m_username)
            continue;

        QString line = QString("\"%1\",\"%2\",\"%3 (%4)\",\"%5\",\"%6\",\"%7\"")
                           .arg(order.id())
                           .arg(order.clientId())
                           .arg(order.device().typeName())
                           .arg(order.device().model())
                           .arg(order.issue())
                           .arg(order.currentStatus())
                           .arg(order.createdAt().toString("yyyy-MM-dd HH:mm"));
        out << line << "\n";
    }

    file.close();
    qDebug() << "Экспорт завершён:" << filePath;
}

void MainWindow::onOrderStatusChanged(const QString& message) {
    statusBar()->showMessage(message, 5000);
}

void MainWindow::on_btnAssignParts_clicked() {
    if (m_role != User::Manager) return;

    QTableWidget* table = ui->tableAllOrders;
    auto sel = table->selectedRanges();
    if (sel.isEmpty()) {
        QMessageBox::warning(nullptr, "Ошибка", "Выберите заказ");
        return;
    }

    QString orderId = table->item(sel.first().topRow(), 0)->text();

    RepairOrder* targetOrder = nullptr;
    for (auto& order : DataStorage::instance().orders()) {
        if (order.id() == orderId) {
            targetOrder = &order;
            break;
        }
    }
    if (!targetOrder) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Назначить запчасти на заказ " + orderId);
    dialog.resize(500, 400);

    QVBoxLayout* layout = new QVBoxLayout(&dialog); 

    QTableWidget* partsTable = new QTableWidget(&dialog); 
    partsTable->setColumnCount(4);
    partsTable->setHorizontalHeaderLabels({"Артикул", "Наименование", "Остаток", "Назначить"});
    partsTable->horizontalHeader()->setStretchLastSection(true);

    auto& parts = DataStorage::instance().parts();
    partsTable->setRowCount(parts.size());
    for (int i = 0; i < parts.size(); ++i) {
        const Part& p = parts[i];
        partsTable->setItem(i, 0, new QTableWidgetItem(p.article()));
        partsTable->setItem(i, 1, new QTableWidgetItem(p.name()));
        partsTable->setItem(i, 2, new QTableWidgetItem(QString::number(p.quantity())));
        QSpinBox* spin = new QSpinBox();
        spin->setRange(0, p.quantity());
        spin->setValue(0);
        partsTable->setCellWidget(i, 3, spin);
    }

    layout->addWidget(partsTable);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("Назначить");
    QPushButton* cancelBtn = new QPushButton("Отмена");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    bool anyAssigned = false;
    for (int i = 0; i < parts.size(); ++i) {
        QSpinBox* spin = qobject_cast<QSpinBox*>(partsTable->cellWidget(i, 3));
        if (spin && spin->value() > 0) {
            const Part& p = parts[i];
            Part assignedPart(p.article(), p.name(), spin->value(), p.price());
            targetOrder->usePart(assignedPart);
            anyAssigned = true;
        }
    }

    if (anyAssigned) {
        DataStorage::instance().save();
        QMessageBox::information(nullptr, "Успех", "Запчасти назначены на заказ");
    }
}

void MainWindow::on_btnLogout_clicked() {
    emit logoutRequested();
    this->close();
}
