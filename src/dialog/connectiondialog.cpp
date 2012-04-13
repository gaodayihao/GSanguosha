#include "connectiondialog.h"
#include "ui_connectiondialog.h"
#include "settings.h"
#include "engine.h"
#include "detector.h"

#include <QMessageBox>
#include <QTimer>
#include <QRadioButton>
#include <QBoxLayout>

static const int ShrinkWidth = 285;
static const int ExpandWidth = 826;

ConnectionDialog::ConnectionDialog(QWidget *parent) :
    QDialog(parent), ui(new Ui::ConnectionDialog)
{
    ui->setupUi(this);
    ui->nameLineEdit->setText(Config.UserName);
    ui->nameLineEdit->setMaxLength(32);

    ui->hostComboBox->addItems(Config.HistoryIPs);
    ui->hostComboBox->lineEdit()->setText(Config.HostAddress);

    ui->getNodeListButton->setFocus();

    ui->portLineEdit->setText(QString::number(Config.ServerPort));
    ui->portLineEdit->setValidator(new QIntValidator(1, 9999, ui->portLineEdit));
//    ui->connectButton->setFocus();

    const General *avatar_general = Sanguosha->getGeneral(Config.UserAvatar);
    if(avatar_general){
        QPixmap avatar(avatar_general->getPixmapPath("big"));
        ui->avatarPixmap->setPixmap(avatar);
    }

    QList<const General*> generals = Sanguosha->findChildren<const General*>();
    foreach(const General *general, generals){
        QIcon icon(general->getPixmapPath("big"));
        QString text = Sanguosha->translate(general->objectName());
        QListWidgetItem *item = new QListWidgetItem(icon, text, ui->avatarList);
        item->setData(Qt::UserRole, general->objectName());
    }

    ui->avatarList->hide();



    ui->nodeListTable->hide();
    ui->nodeListTable->setSelectionBehavior(QAbstractItemView::SelectRows); //整行选中的方式
    ui->nodeListTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->nodeListTable->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->reconnectionCheckBox->setChecked(Config.value("EnableReconnection", false).toBool());

    setFixedHeight(height());
    setFixedWidth(ShrinkWidth);

    ui->reconnectionCheckBox->setChecked(false);
    ui->reconnectionCheckBox->setVisible(false);
    ui->reconnectionCheckBox->setEnabled(false);

    pheight = parent->height();
}

ConnectionDialog::~ConnectionDialog()
{
    delete ui;
}

void ConnectionDialog::on_connectButton_clicked()
{
    QString username = ui->nameLineEdit->text();

    if(username.isEmpty()){
        QMessageBox::warning(this, tr("Warning"), tr("The user name can not be empty!"));
        return;
    }

    QRegExp rx("[<>]");
    if(rx.indexIn(username)>-1)
    {
        QMessageBox::warning(this, tr("Warning"), tr("The user name contains invalid chars!"));
        return;
    }

    bool ok;
    int port = ui->portLineEdit->text().toInt(&ok);
    if(port){
        Config.ServerPort = port;
        Config.setValue("ServerPort", Config.ServerPort);
    }

    Config.UserName = username;
    Config.HostAddress = ui->hostComboBox->lineEdit()->text();
    Config.Password = ui->passwordLineEdit->text();

    Config.setValue("UserName", Config.UserName);
    Config.setValue("HostAddress", Config.HostAddress);
    Config.setValue("EnableReconnection", ui->reconnectionCheckBox->isChecked());

    accept();
}

void ConnectionDialog::on_changeAvatarButton_clicked()
{
    if(ui->avatarList->isVisible()){
        QListWidgetItem *selected = ui->avatarList->currentItem();
        if(selected)
            on_avatarList_itemDoubleClicked(selected);
        else{
            ui->avatarList->hide();
            setFixedWidth(ShrinkWidth);
        }
    }else{
        ui->avatarList->show();
        setFixedWidth(ExpandWidth);
        ui->nodeListTable->hide();
    }
}

void ConnectionDialog::on_avatarList_itemDoubleClicked(QListWidgetItem* item)
{
    QString general_name = item->data(Qt::UserRole).toString();
    const General *general = Sanguosha->getGeneral(general_name);
    if(general){
        QPixmap avatar(general->getPixmapPath("big"));
        ui->avatarPixmap->setPixmap(avatar);
        Config.UserAvatar = general_name;
        Config.setValue("UserAvatar", general_name);
        ui->avatarList->hide();

        setFixedWidth(ShrinkWidth);
    }
}

void ConnectionDialog::on_clearHistoryButton_clicked()
{
    ui->hostComboBox->clear();
    ui->hostComboBox->lineEdit()->clear();

    Config.HistoryIPs.clear();
    Config.remove("HistoryIPs");
}

void ConnectionDialog::on_detectLANButton_clicked()
{
    UdpDetectorDialog *detector_dialog = new UdpDetectorDialog(this);
    connect(detector_dialog, SIGNAL(address_chosen(QString)),
            ui->hostComboBox->lineEdit(), SLOT(setText(QString)));

    detector_dialog->exec();
}

// -----------------------------------

UdpDetectorDialog::UdpDetectorDialog(QDialog *parent)
    :QDialog(parent)
{
    setWindowTitle(tr("Detect available server's addresses at LAN"));
    detect_button = new QPushButton(tr("Refresh"));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(detect_button);

    list = new QListWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(list);
    layout->addLayout(hlayout);

    setLayout(layout);

    detector = NULL;
    connect(detect_button, SIGNAL(clicked()), this, SLOT(startDetection()));
    connect(list, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(chooseAddress(QListWidgetItem*)));

    detect_button->click();
}

void UdpDetectorDialog::startDetection(){
    list->clear();
    detect_button->setEnabled(false);

    detector = new UdpDetector;
    connect(detector, SIGNAL(detected(QString,QString)), this, SLOT(addServerAddress(QString,QString)));
    QTimer::singleShot(2000, this, SLOT(stopDetection()));

    detector->detect();
}

void UdpDetectorDialog::stopDetection(){
    detect_button->setEnabled(true);
    detector->stop();
    detector = NULL;
}

void UdpDetectorDialog::addServerAddress(const QString &server_name, const QString &address){
    QString label = QString("%1 [%2]").arg(server_name).arg(address);
    QListWidgetItem *item = new QListWidgetItem(label);
    item->setData(Qt::UserRole, address);

    list->addItem(item);
}

void UdpDetectorDialog::chooseAddress(QListWidgetItem *item){
    accept();

    QString address = item->data(Qt::UserRole).toString();
    emit address_chosen(address);
}

void ConnectionDialog::on_getNodeListButton_clicked()
{
    ui->connectButton->setEnabled(true);
    ui->avatarList->hide();

    QString username = ui->nameLineEdit->text();

    if(username.isEmpty()){
        QMessageBox::warning(this, tr("Warning"), tr("The user name can not be empty!"));
        return;
    }

    QRegExp rx("[<>]");
    if(rx.indexIn(username)>-1)
    {
        QMessageBox::warning(this, tr("Warning"), tr("The user name contains invalid chars!"));
        return;
    }

    Config.UserName = username;
    Config.HostAddress = ui->hostComboBox->lineEdit()->text();
    Config.Password = ui->passwordLineEdit->text();

    bool ok;
    int port = ui->portLineEdit->text().toInt(&ok);
    if(port){
        Config.ServerPort = port;
        Config.setValue("ServerPort", Config.ServerPort);
    }

    Config.setValue("UserName", Config.UserName);
    Config.setValue("HostAddress", Config.HostAddress);

    // clear table
    for(;ui->nodeListTable->rowCount();)
    {
       ui->nodeListTable->removeRow(0);
    }
    ui->nodeListTable->setColumnWidth(0,200);
    ui->nodeListTable->setColumnWidth(1,60);
    ui->nodeListTable->setColumnWidth(2,160);
    ui->nodeListTable->setColumnWidth(3,120);
    ui->nodeListTable->setColumnWidth(4,60);
    ui->nodeListTable->setColumnWidth(5,60);
    ui->nodeListTable->setColumnWidth(6,60);
    ui->nodeListTable->setColumnWidth(7,60);

    ui->nodeListTable->show();
    QStringList headers;
    headers << tr("Address:") << tr("Port:") << tr("ServerName:") << tr("Version:") << tr("Mode:") << tr("Rooms:") << tr("Players:") << tr("Network:");
    ui->nodeListTable->setHorizontalHeaderLabels(headers);
    ui->nodeListTable->setColumnHidden(0,false);
    ui->nodeListTable->setColumnHidden(1,false);
    ui->nodeListTable->setColumnHidden(2,true);
    ui->nodeListTable->setColumnHidden(3,true);
    setFixedWidth(ExpandWidth);
    move(QPoint(ExpandWidth / 2 - ShrinkWidth / 2 + 60, pheight - height() / 2));
    emit(qnodelist(ui->hostComboBox->lineEdit()->text(), port)) ;
}

void ConnectionDialog::updateNodeListTable(QString node)
{
    QStringList tmp=node.split(":");
    if(tmp.count()==3)
    {
        ui->nodeListTable->setColumnHidden(0,false);
        ui->nodeListTable->setColumnHidden(1,false);
        ui->nodeListTable->setColumnHidden(2,true);
        ui->nodeListTable->setColumnHidden(3,true);
        int newRowCount =ui->nodeListTable->rowCount();
        ui->nodeListTable->insertRow(newRowCount);
        ui->nodeListTable->setItem(newRowCount,0,new QTableWidgetItem(tmp[0]));
        ui->nodeListTable->setItem(newRowCount,1,new QTableWidgetItem(tmp[1]));
        ui->nodeListTable->setItem(newRowCount,2,new QTableWidgetItem(tmp[0]));
        ui->nodeListTable->setItem(newRowCount,4,new QTableWidgetItem(tmp[2]));
        emit(qnodeinfo(tmp[0] ,tmp[1].toInt()));
    }
    else if(tmp.count()==9)
    {
        ui->nodeListTable->setColumnHidden(0,true);
        ui->nodeListTable->setColumnHidden(1,true);
        ui->nodeListTable->setColumnHidden(2,false);
        ui->nodeListTable->setColumnHidden(3,false);
        for (int row = 0; row < ui->nodeListTable->rowCount(); ++row)
        {
            QString addr,port,mode;
            addr=ui->nodeListTable->item(row, 0)->text();
            port=ui->nodeListTable->item(row, 1)->text();
            mode=ui->nodeListTable->item(row, 4)->text();
            if(addr==tmp[0] && port==tmp[1] && mode==tmp[4])
            {
                ui->nodeListTable->setItem(row,2,new QTableWidgetItem(tmp[2]));
                ui->nodeListTable->setItem(row,3,new QTableWidgetItem(tmp[3]));
                ui->nodeListTable->setItem(row,4,new QTableWidgetItem(tmp[4]+tmp[5]));
                ui->nodeListTable->setItem(row,5,new QTableWidgetItem(tmp[6]));
                ui->nodeListTable->setItem(row,6,new QTableWidgetItem(tmp[7]));
                ui->nodeListTable->setItem(row,7,new QTableWidgetItem(tmp[8]+"ms"));
            }
        }
        ui->nodeListTable->sortByColumn(3);
    }
}

void ConnectionDialog::on_nodeListTable_itemDoubleClicked(QTableWidgetItem* item)
{
    int row = ui->nodeListTable->currentRow();
    ui->hostComboBox->lineEdit()->setText(ui->nodeListTable->item(row, 0)->text());
    ui->portLineEdit->setText( ui->nodeListTable->item(row,1)->text());
    on_connectButton_clicked();
}

void ConnectionDialog::on_nodeListTable_itemSelectionChanged()
{
//    int row = ui->nodeListTable->currentRow();
//    if(row >= 0){
//        ui->hostComboBox->lineEdit()->setText(ui->nodeListTable->item(row, 0)->text());
//        ui->portLineEdit->setText( ui->nodeListTable->item(row,1)->text());
//    }
}

void ConnectionDialog::on_nodeListTable_itemClicked(QTableWidgetItem* item)
{
    int row = ui->nodeListTable->currentRow();
    if(row >= 0){
        ui->hostComboBox->lineEdit()->setText(ui->nodeListTable->item(row, 0)->text());
        ui->portLineEdit->setText( ui->nodeListTable->item(row,1)->text());
    }
}
