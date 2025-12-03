#include <QTextStream>
#include <QPainter>
#include <QPageSize>
#include <QDateTime>
#include <QHBoxLayout>
#include <QComboBox>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include "mainwindow.h"
#include <QPrinter>
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

    setWindowTitle(QString("Ремонтная мастерская — %1 (%2)")
                       .arg(username)
                       .arg(role == User::Client ? "Клиент" : "Менеджер"));

    connect(ui->tableParts, &QTableWidget::doubleClicked, this, &MainWindow::on_tableParts_doubleClicked);
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

    ui->tableParts->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableParts->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableParts->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->tableParts->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableParts->setColumnWidth(0, 100);
    ui->tableParts->setColumnWidth(2, 80);
    ui->tableParts->setColumnWidth(3, 100);
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
    ui->tableParts->setAlternatingRowColors(true);
    connect(ui->tableMyOrders, &QTableWidget::doubleClicked, this, &MainWindow::onOrderDoubleClicked);
    connect(ui->tableAllOrders, &QTableWidget::doubleClicked, this, &MainWindow::onOrderDoubleClicked);
    ui->tableMyOrders->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableAllOrders->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->tableMyOrders, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::on_table_customContextMenuRequested);

    connect(ui->tableAllOrders, &QTableWidget::customContextMenuRequested,
            this, &MainWindow::on_table_customContextMenuRequested);
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
        if (m_role == User::Client && order.clientId() != m_username) {
            continue;
        }
        if (m_role == User::Client) {
            table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
            table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
            table->setColumnWidth(1, 100);
            table->setColumnWidth(2, 120);
        } else {
            table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
            table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
            table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);
            table->setColumnWidth(0, 80);
            table->setColumnWidth(1, 120);
            table->setColumnWidth(3, 100);
            table->setColumnWidth(4, 120);
        }

        table->insertRow(row);

        QTableWidgetItem* deviceItem = new QTableWidgetItem(
            QString("%1 (%2)").arg(order.device().typeName()).arg(order.device().model())
            );
        deviceItem->setData(Qt::UserRole, order.id());

        if (m_role == User::Client) {
            table->setItem(row, 0, deviceItem);
            QString statusWithEmoji = order.currentStatus();
            if (order.currentStatus() == "Принят") {
                statusWithEmoji = "📥 Принят";
            } else if (order.currentStatus() == "В работе") {
                statusWithEmoji = "🔧 В работе";
            } else if (order.currentStatus() == "Готов") {
                statusWithEmoji = "✅ Готов";
            } else if (order.currentStatus() == "Выдан") {
                statusWithEmoji = "🤝 Выдан";
            }
            table->setItem(row, 1, new QTableWidgetItem(statusWithEmoji));
            QTableWidgetItem* dateItem = new QTableWidgetItem(order.createdAt().toString("dd.MM.yyyy"));
            dateItem->setData(Qt::EditRole, order.createdAt().toString("yyyy-MM-dd"));
            table->setItem(row, 2, dateItem);
        } else {
            table->setItem(row, 0, new QTableWidgetItem(order.id()));
            table->setItem(row, 1, new QTableWidgetItem(order.clientId()));
            table->setItem(row, 2, deviceItem);
            QString statusWithEmoji = order.currentStatus();
            if (order.currentStatus() == "Принят") {
                statusWithEmoji = "📥 Принят";
            } else if (order.currentStatus() == "В работе") {
                statusWithEmoji = "🔧 В работе";
            } else if (order.currentStatus() == "Готов") {
                statusWithEmoji = "✅ Готов";
            } else if (order.currentStatus() == "Выдан") {
                statusWithEmoji = "🤝 Выдан";
            }
            table->setItem(row, 3, new QTableWidgetItem(statusWithEmoji));
            QTableWidgetItem* dateItem = new QTableWidgetItem(order.createdAt().toString("dd.MM.yyyy"));
            dateItem->setData(Qt::EditRole, order.createdAt().toString("yyyy-MM-dd"));
            table->setItem(row, 4, dateItem);
        }
        row++;
    }
    table->setSortingEnabled(true);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setAlternatingRowColors(true);
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
    layout.addRow("Клиент:", &clientEdit);

    QComboBox deviceType;
    deviceType.addItems({"Ноутбук", "Телефон", "Планшет"});
    layout.addRow("Тип устройства:", &deviceType);

    QComboBox* modelCombo = new QComboBox();
    modelCombo->setEditable(true);
    modelCombo->setPlaceholderText("Выберите или введите модель");

    auto updateModelList = [&](int deviceIndex) {
        modelCombo->clear();
        if (deviceIndex == Device::Laptop) {
            modelCombo->addItems({"HP Pavilion 15", "Dell Latitude 5420", "MacBook Air M2", "Lenovo ThinkPad X1"});
        } else if (deviceIndex == Device::Phone) {
            modelCombo->addItems({"iPhone 13", "Samsung Galaxy S21", "Xiaomi Redmi Note 12", "Google Pixel 6"});
        } else if (deviceIndex == Device::Tablet) {
            modelCombo->addItems({"iPad Pro 11\"", "Samsung Galaxy Tab S8", "Huawei MatePad Pro", "Amazon Fire HD 10"});
        }
        modelCombo->setCurrentText("");
    };

    updateModelList(Device::Laptop);

    QObject::connect(&deviceType, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [&](int index) {
                         updateModelList(index);
                         modelCombo->setCurrentText("");
                     });

    layout.addRow("Модель:", modelCombo);

    QTextEdit* issueEdit = new QTextEdit();
    issueEdit->setPlaceholderText("Опишите неисправность подробно:\n• Что не работает?\n• Когда началось?\n• Были ли попытки ремонта?");
    issueEdit->setMinimumHeight(100);
    layout.addRow("Неисправность:", issueEdit);

    QPushButton okBtn("Создать");
    QPushButton cancelBtn("Отмена");
    layout.addRow(&okBtn, &cancelBtn);

    QObject::connect(&cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(&okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    QString model = modelCombo->currentText().trimmed();
    if (model.isEmpty()) {
        QMessageBox::warning(nullptr, "Ошибка", "Укажите модель устройства");
        return;
    }

    QString issueText = issueEdit->toPlainText().trimmed();
    if (issueText.isEmpty()) {
        QMessageBox::warning(nullptr, "Ошибка", "Опишите неисправность");
        return;
    }

    auto& orders = DataStorage::instance().orders();
    QString id = "ORD-" + QString::number(orders.size() + 1);
    Device::Type type = static_cast<Device::Type>(deviceType.currentIndex());
    Device device(type, model);

    RepairOrder order(id, m_username, device, issueText);
    orders.append(order);
    DataStorage::instance().save();

    updateOrdersList();
    QMessageBox::information(nullptr, "Успех", "Заказ успешно оформлен:\n" + id);
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

    double total = 0.0;
    const auto& stockParts = DataStorage::instance().parts();
    QString partsHtml = "";

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
        partsHtml += QString(
                         "<tr style=\"background: #1e1e1e; color: #ffffff;\">"
                         "<td style=\"padding: 8px; border: 1px solid #444; color: #ffffff;\">%1</td>"
                         "<td style=\"padding: 8px; border: 1px solid #444; text-align: right; color: #ffffff;\">%2</td>"
                         "<td style=\"padding: 8px; border: 1px solid #444; text-align: right; color: #ffffff;\">%3</td>"
                         "<td style=\"padding: 8px; border: 1px solid #444; text-align: right; color: #ffffff;\">%4</td>"
                         "</tr>"
                         )
                         .arg(used.name())
                         .arg(used.quantity())
                         .arg(QString::number(price, 'f', 2))
                         .arg(QString::number(lineTotal, 'f', 2));
    }

    QString invoiceTemplate = R"(
    <div style="font-family: Arial, sans-serif; padding: 20px; max-width: 600px; margin: auto; background: #121212; color: #ffffff;">
        <h2 style="text-align: center; font-size: 24px; margin-bottom: 5px; color: #ffffff;">Счёт на ремонт</h2>
        <p style="text-align: center; font-size: 14px; color: #cccccc; margin: 5px 0;">
            Дата: %1 | Номер заказа: %2
        </p>

        <div style="margin: 20px 0; padding: 15px; background: #1e1e1e; border: 1px solid #333; border-radius: 5px;">
            <p><strong>Клиент:</strong> %3</p>
            <p><strong>Устройство:</strong> %4 (%5)</p>
            <p><strong>Статус:</strong> %6</p>
            <p><strong>Дата создания:</strong> %7</p>
        </div>

        <h3 style="margin-top: 25px; font-size: 18px; border-bottom: 2px solid #ffffff; padding-bottom: 5px; color: #ffffff;">Использованные запчасти</h3>
        <table style="width: 100%; border-collapse: collapse; margin: 15px 0; font-size: 14px;">
            <thead>
                <tr style="background: #2d2d2d; text-align: left;">
                    <th style="padding: 8px; border: 1px solid #444; color: #ffffff;">Наименование</th>
                    <th style="padding: 8px; border: 1px solid #444; text-align: right; color: #ffffff;">Кол-во</th>
                    <th style="padding: 8px; border: 1px solid #444; text-align: right; color: #ffffff;">Цена (₽)</th>
                    <th style="padding: 8px; border: 1px solid #444; text-align: right; color: #ffffff;">Сумма (₽)</th>
                </tr>
            </thead>
            <tbody>
                %8
            </tbody>
        </table>

        <!-- ИСПРАВЛЕНО: таблица итогов с заливкой -->
        <div style="margin: 20px 0; padding: 15px; background: #1e1e1e; border: 1px solid #333; border-radius: 5px;">
            <table style="width: 100%; border-collapse: collapse; font-size: 14px;">
                <tr style="background: #1e1e1e;">
                    <td style="padding: 8px; border: 1px solid #444; font-weight: bold; color: #ffffff;">Итого</td>
                    <td style="padding: 8px; border: 1px solid #444; text-align: right; font-weight: bold; color: #ffffff;">%9</td>
                </tr>
                <tr style="background: #1e1e1e;">
                    <td style="padding: 8px; border: 1px solid #444; color: #ffffff;">НДС (10%)</td>
                    <td style="padding: 8px; border: 1px solid #444; text-align: right; color: #ffffff;">%10</td>
                </tr>
                <tr style="background: #2d2d2d;">
                    <td style="padding: 8px; border: 1px solid #444; font-weight: bold; color: #ffffff;">Общая сумма</td>
                    <td style="padding: 8px; border: 1px solid #444; text-align: right; font-weight: bold; color: #ffffff;">%11</td>
                </tr>
            </table>
        </div>

        <div style="margin: 20px 0; padding: 15px; background: #1e1e1e; border: 1px solid #333; border-radius: 5px;">
            <h4 style="margin: 5px 0; font-size: 16px; font-weight: bold; color: #ffffff;">Оплата</h4>
            <p style="margin: 5px 0; font-size: 14px; color: #ffffff;">
                Оплатите счёт в течение 7 дней.<br>
                <strong>Реквизиты:</strong><br>
                Банк: Сбербанк<br>
                Расчётный счёт: 40817810000000000000<br>
                БИК: 044525225<br>
                ИНН: 7701010101
            </p>
        </div>

        <div style="margin: 20px 0; padding: 15px; background: #1e1e1e; border: 1px solid #333; border-radius: 5px;">
            <h4 style="margin: 5px 0; font-size: 16px; font-weight: bold; color: #ffffff;">Условия</h4>
            <p style="margin: 5px 0; font-size: 14px; color: #ffffff;">
                • Гарантия 30 дней на все работы.<br>
                • Повторный ремонт по гарантии — бесплатно.<br>
                • Неисправности из-за неправильного использования — не гарантийные.
            </p>
        </div>

        <div style="margin-top: 20px; font-size: 12px; color: #888; text-align: center;">
            Счёт сформирован автоматически. Подпись не требуется.
        </div>
    </div>
)";
    QString statusWithEmoji = selectedOrder->currentStatus();
    if (statusWithEmoji == "Принят") {
        statusWithEmoji = "📥 Принят";
    } else if (statusWithEmoji == "В работе") {
        statusWithEmoji = "🔧 В работе";
    } else if (statusWithEmoji == "Готов") {
        statusWithEmoji = "✅ Готов";
    } else if (statusWithEmoji == "Выдан") {
        statusWithEmoji = "🤝 Выдан";
    }
    QString invoice = invoiceTemplate
                          .arg(selectedOrder->createdAt().toString("dd.MM.yyyy"))
                          .arg(selectedOrder->id())
                          .arg(selectedOrder->clientId())
                          .arg(selectedOrder->device().typeName())
                          .arg(selectedOrder->device().model())
                          .arg(statusWithEmoji)
                          .arg(selectedOrder->createdAt().toString("dd.MM.yyyy HH:mm"))
                          .arg(partsHtml)
                          .arg(QString::number(total, 'f', 2))
                          .arg(QString::number(total * 0.1, 'f', 2))
                          .arg(QString::number(total * 1.1, 'f', 2));

    QDialog dialog(nullptr);
    dialog.setWindowTitle("Счёт на оплату");
    dialog.resize(650, 600);

    QVBoxLayout layout(&dialog);
    QTextEdit textEdit;
    textEdit.setReadOnly(true);
    textEdit.setFont(QFont("Arial", 10));
    textEdit.setStyleSheet("background: #121212; color: white;");
    textEdit.setHtml(invoice);
    layout.addWidget(&textEdit);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* closeBtn = new QPushButton("Закрыть");
    QPushButton* savePdfBtn = new QPushButton("Сохранить как PDF");
    btnLayout->addStretch();
    btnLayout->addWidget(closeBtn);
    btnLayout->addWidget(savePdfBtn);
    layout.addLayout(btnLayout);

    QObject::connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    QObject::connect(savePdfBtn, &QPushButton::clicked, [&]() {
        QString filePath = QFileDialog::getSaveFileName(
            &dialog,
            "Сохранить счёт как PDF",
            "счёт_" + selectedOrder->id() + ".pdf",
            "PDF (*.pdf)"
            );
        if (filePath.isEmpty()) return;

        QPrinter printer(QPrinter::HighResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filePath);
        printer.setPageSize(QPageSize(QPageSize::A4));
        printer.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter);

        QTextDocument doc;
        doc.setHtml(invoice);

        QFont font = doc.defaultFont();
        font.setPointSize(16);
        doc.setDefaultFont(font);

        QPainter painter(&printer);
        painter.scale(1.5, 1.5);

        doc.drawContents(&painter);

        QMessageBox::information(&dialog, "Успех", "Счёт сохранён как PDF");
    });

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
    dialog.resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QHBoxLayout* searchLayout = new QHBoxLayout();
    QLineEdit* searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Поиск по артикулу или наименованию...");
    searchLayout->addWidget(searchEdit);
    layout->addLayout(searchLayout);

    QTableWidget* partsTable = new QTableWidget(&dialog);
    partsTable->setColumnCount(5);
    partsTable->setHorizontalHeaderLabels({"Артикул", "Наименование", "Остаток", "Назначено", "Назначить"});
    partsTable->horizontalHeader()->setStretchLastSection(true);

    auto& parts = DataStorage::instance().parts();
    partsTable->setRowCount(parts.size());

    for (int i = 0; i < parts.size(); ++i) {
        const Part& p = parts[i];
        partsTable->setItem(i, 0, new QTableWidgetItem(p.article()));
        partsTable->setItem(i, 1, new QTableWidgetItem(p.name()));
        partsTable->setItem(i, 2, new QTableWidgetItem(QString::number(p.quantity())));

        int assigned = 0;
        for (const Part& used : targetOrder->usedParts()) {
            if (used.article() == p.article()) {
                assigned = used.quantity();
                break;
            }
        }
        partsTable->setItem(i, 3, new QTableWidgetItem(QString::number(assigned)));

        QSpinBox* spin = new QSpinBox();
        spin->setRange(0, p.quantity() - assigned);
        spin->setValue(0);
        partsTable->setCellWidget(i, 4, spin);
    }

    partsTable->setSortingEnabled(true);
    partsTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    layout->addWidget(partsTable);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("Назначить");
    QPushButton* cancelBtn = new QPushButton("Отмена");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    QObject::connect(searchEdit, &QLineEdit::textChanged, [&](const QString& text) {
        for (int i = 0; i < partsTable->rowCount(); ++i) {
            bool show = false;
            for (int j = 0; j < 2; ++j) {
                auto* item = partsTable->item(i, j);
                if (item && item->text().contains(text, Qt::CaseInsensitive)) {
                    show = true;
                    break;
                }
            }
            partsTable->setRowHidden(i, !show);
        }
    });

    QObject::connect(cancelBtn, &QPushButton::clicked, &dialog, &QDialog::reject);
    QObject::connect(okBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted) return;

    bool anyAssigned = false;
    for (int i = 0; i < parts.size(); ++i) {
        QSpinBox* spin = qobject_cast<QSpinBox*>(partsTable->cellWidget(i, 4));
        if (!spin || spin->value() == 0) continue;

        const Part& stockPart = parts[i];
        int newQuantity = spin->value();

        bool updated = false;
        for (Part& used : targetOrder->usedPartsRef()) {
            if (used.article() == stockPart.article()) {
                used.setQuantity(newQuantity);
                updated = true;
                break;
            }
        }

        if (!updated) {
            Part newPart(stockPart.article(), stockPart.name(), newQuantity, stockPart.price());
            targetOrder->usePart(newPart);
        }

        anyAssigned = true;
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

void MainWindow::onOrderDoubleClicked(const QModelIndex &index) {
    QTableWidget* table = qobject_cast<QTableWidget*>(sender());
    if (!table) return;

    int row = index.row();
    if (row < 0 || row >= table->rowCount()) return;

    QTableWidgetItem* deviceItem = nullptr;
    if (m_role == User::Client) {
        deviceItem = table->item(row, 0);
    } else {
        deviceItem = table->item(row, 2);
    }

    if (!deviceItem) return;

    QString orderId = deviceItem->data(Qt::UserRole).toString();
    if (orderId.isEmpty()) return;

    RepairOrder* order = nullptr;
    for (auto& o : DataStorage::instance().orders()) {
        if (o.id() == orderId) {
            order = &o;
            break;
        }
    }

    if (!order) return;

    QString details = QString(
                          "<b>Детали заказа</b><br><br>"
                          "<b>ID заказа:</b> %1<br>"
                          "<b>Клиент:</b> %2<br>"
                          "<b>Устройство:</b> %3 (%4)<br>"
                          "<b>Статус:</b> %5<br>"
                          "<b>Дата создания:</b> %6<br><br>"
                          "<b>Причина неисправности:</b><br>"
                          "<i>%7</i>"
                          )
                          .arg(order->id())
                          .arg(order->clientId())
                          .arg(order->device().typeName())
                          .arg(order->device().model())
                          .arg(order->currentStatus())
                          .arg(order->createdAt().toString("dd.MM.yyyy HH:mm"))
                          .arg(order->issue().isEmpty() ? "Не указана" : order->issue());

    QDialog dialog(this);
    dialog.setWindowTitle("Детали заказа " + orderId);
    dialog.resize(500, 300);

    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QTextEdit* textEdit = new QTextEdit();
    textEdit->setReadOnly(true);
    textEdit->setHtml(details);
    layout->addWidget(textEdit);

    QPushButton* closeBtn = new QPushButton("Закрыть");
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::on_table_customContextMenuRequested(const QPoint& pos) {
    QTableWidget* table = qobject_cast<QTableWidget*>(sender());
    if (!table) return;

    auto selectedRows = table->selectedRanges();
    if (selectedRows.isEmpty()) return;

    QMenu menu(this);
    QAction* exportSelectedAction = menu.addAction("Экспортировать выбранное");
    connect(exportSelectedAction, &QAction::triggered, [=]() {
        exportSelectedOrders(table);
    });

    menu.exec(table->viewport()->mapToGlobal(pos));
}

void MainWindow::exportSelectedOrders(QTableWidget* table) {
    auto selectedRows = table->selectedRanges();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Выберите строки для экспорта");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        nullptr, "Экспорт выбранных заказов", "", "CSV (*.csv)"
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

    for (const auto& range : selectedRows) {
        for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
            QTableWidgetItem* deviceItem = nullptr;
            if (m_role == User::Client) {
                deviceItem = table->item(row, 0);
            } else {
                deviceItem = table->item(row, 2);
            }

            if (!deviceItem) continue;

            QString orderId = deviceItem->data(Qt::UserRole).toString();
            if (orderId.isEmpty()) continue;

            RepairOrder* order = nullptr;
            for (auto& o : DataStorage::instance().orders()) {
                if (o.id() == orderId) {
                    order = &o;
                    break;
                }
            }

            if (!order) continue;

            QString line = QString("\"%1\",\"%2\",\"%3 (%4)\",\"%5\",\"%6\",\"%7\"")
                               .arg(order->id())
                               .arg(order->clientId())
                               .arg(order->device().typeName())
                               .arg(order->device().model())
                               .arg(order->issue())
                               .arg(order->currentStatus())
                               .arg(order->createdAt().toString("yyyy-MM-dd HH:mm"));
            out << line << "\n";
        }
    }

    file.close();
    qDebug() << "Экспорт завершён:" << filePath;
}
