#ifndef CONNECTIONDIALOG_H
#define CONNECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QComboBox>
#include <QButtonGroup>
#include <QTableWidgetItem>

class UdpDetector;

namespace Ui {
    class ConnectionDialog;
}

class ConnectionDialog : public QDialog {
    Q_OBJECT

public:
    ConnectionDialog(QWidget *parent);
    ~ConnectionDialog();

private:
    Ui::ConnectionDialog *ui;
    int pheight;

private slots:
    void on_detectLANButton_clicked();
    void on_clearHistoryButton_clicked();
    void on_avatarList_itemDoubleClicked(QListWidgetItem* item);
    void on_changeAvatarButton_clicked();
    void on_connectButton_clicked();

    //
    void on_getNodeListButton_clicked();
    void on_nodeListTable_itemClicked(QTableWidgetItem* item);
    void on_nodeListTable_itemSelectionChanged();
    void on_nodeListTable_itemDoubleClicked(QTableWidgetItem* item);
    void updateNodeListTable(QString node);
signals:
    void qnodelist(QString ,int);
    void qnodeinfo(QString ,int);
};

class UdpDetectorDialog : public QDialog{
    Q_OBJECT

public:
    UdpDetectorDialog(QDialog *parent);

private:
    QListWidget *list;
    UdpDetector *detector;
    QPushButton *detect_button;

private slots:
    void startDetection();
    void stopDetection();
    void chooseAddress(QListWidgetItem *item);
    void addServerAddress(const QString &server_name, const QString &address);

signals:
    void address_chosen(const QString &address);
};

#endif // CONNECTIONDIALOG_H
