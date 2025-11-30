#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "clientnotifier.h"
#include <QTableWidget>
#include "user.h"

class StatusTracker;
class Ui_MainWindow;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow  
{
    Q_OBJECT

public:
    explicit MainWindow(User::Role role, const QString& username, QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void logoutRequested();

private slots:
    void onOrderDoubleClicked(const QModelIndex &index);
    void on_btnChangeStatus_clicked();
    void on_btnNewOrder_clicked();
    void on_actionExportHistory_triggered();
    void on_btnInvoice_clicked();
    void on_btnLogout_clicked();
    void on_btnExportMyOrders_clicked();
    void on_btnExportAllOrders_clicked();
    void on_btnAddPart_clicked();
    void on_btnRemovePart_clicked();
    void on_btnInvoice_2_clicked();
    void on_btnAssignParts_clicked();
    void onOrderStatusChanged(const QString& message);
    void on_tableParts_doubleClicked(const QModelIndex &index);
    void on_searchEdit_textChanged(const QString &text);
    void on_searchEdit_2_textChanged(const QString &text);

private:
    void exportOrders(bool clientOnly);
    void setupClientView();
    ClientNotifier* m_clientNotifier = nullptr;
    void setupManagerView();
    void updateOrdersList();
    void generateInvoiceForSelectedOrder(QTableWidget* table);

    Ui::MainWindow *ui;
    User::Role m_role;
    QString m_username;
    StatusTracker* m_statusTracker = nullptr; 
};

#endif 
