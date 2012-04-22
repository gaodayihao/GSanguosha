#include "server.h"
#include "settings.h"
#include "room.h"
#include "engine.h"
#include "nativesocket.h"
#include "banpair.h"
#include "scenario.h"
#include "contestdb.h"
#include "choosegeneraldialog.h"
#include "customassigndialog.h"
#include "time.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QRadioButton>
#include <QApplication>
#include <QHttp>
#include <QAction>
#include <QTimer>

static QLayout *HLay(QWidget *left, QWidget *right){
    QHBoxLayout *layout = new QHBoxLayout;
    layout->addWidget(left);
    layout->addWidget(right);

    return layout;
}

ServerDialog::ServerDialog(QWidget *parent)
    :QDialog(parent)
{
    setWindowTitle(tr("Start server"));

    QTabWidget *tab_widget = new QTabWidget;
    tab_widget->addTab(createBasicTab(), tr("Basic"));
    tab_widget->addTab(createPackageTab(), tr("Game Pacakge Selection"));
    tab_widget->addTab(createAdvancedTab(), tr("Advanced"));
    tab_widget->addTab(createAITab(), tr("Artificial intelligence"));

    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(tab_widget);
    layout->addLayout(createButtonLayout());
    setLayout(layout);

    setMinimumWidth(300);
}

QWidget *ServerDialog::createBasicTab(){
    server_name_edit = new QLineEdit;
    server_name_edit->setText(Config.ServerName);

    timeout_spinbox = new QSpinBox;
    timeout_spinbox->setMinimum(5);
    timeout_spinbox->setMaximum(30);
    timeout_spinbox->setValue(Config.OperationTimeout);
    timeout_spinbox->setSuffix(tr(" seconds"));
    nolimit_checkbox = new QCheckBox(tr("No limit"));
    nolimit_checkbox->setChecked(false);
    connect(nolimit_checkbox, SIGNAL(toggled(bool)), timeout_spinbox, SLOT(setDisabled(bool)));
    nolimit_checkbox->setChecked(Config.OperationNoLimit);

    // add 1v1 banlist edit button
    QPushButton *edit_button = new QPushButton(tr("Banlist ..."));
    edit_button->setFixedWidth(100);
    connect(edit_button, SIGNAL(clicked()), this, SLOT(edit1v1Banlist()));

    QFormLayout *form_layout = new QFormLayout;
    QHBoxLayout *hlay = new QHBoxLayout;
    hlay->addWidget(timeout_spinbox);
    hlay->addWidget(nolimit_checkbox);
    hlay->addWidget(edit_button);
    form_layout->addRow(tr("Server name"), server_name_edit);
    QHBoxLayout * lay = new QHBoxLayout;
    lay->addWidget(timeout_spinbox);
    lay->addWidget(nolimit_checkbox);
    lay->addWidget(edit_button);
    form_layout->addRow(tr("Operation timeout"), lay);
    form_layout->addRow(createGameModeBox());

    QWidget *widget = new QWidget;
    widget->setLayout(form_layout);
    return widget;
}

QWidget *ServerDialog::createPackageTab(){
    extension_group = new QButtonGroup;
    extension_group->setExclusive(false);

    QStringList extensions = Sanguosha->getExtensions();
    QSet<QString> ban_packages = Config.BanPackages.toSet();

    QGroupBox *box1 = new QGroupBox(tr("General package"));
    QGroupBox *box2 = new QGroupBox(tr("Card package"));

    QGridLayout *layout1 = new QGridLayout;
    QGridLayout *layout2 = new QGridLayout;
    box1->setLayout(layout1);
    box2->setLayout(layout2);

    int i = 0, j = 0;
    int row = 0, column = 0;
    foreach(QString extension, extensions){
        const Package *package = Sanguosha->findChild<const Package *>(extension);
        if(package == NULL)
            continue;

        bool forbid_package = Config.value("ForbidPackages").toString().contains(extension);
        QCheckBox *checkbox = new QCheckBox;
        checkbox->setObjectName(extension);
        checkbox->setText(Sanguosha->translate(extension));
        checkbox->setChecked(!ban_packages.contains(extension) && !forbid_package);
        checkbox->setEnabled(!forbid_package);

        extension_group->addButton(checkbox);

        switch(package->getType()){
        case Package::GeneralPack: {
                row = i / 5;
                column = i % 5;
                i++;

                layout1->addWidget(checkbox, row, column+1);
                break;
            }

        case Package::CardPack: {
                row = j / 5;
                column = j % 5;
                j++;

                layout2->addWidget(checkbox, row, column+1);
                break;
            }

        default:
            break;
        }
    }

    QWidget *widget = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;
    layout->addWidget(box1);
    layout->addWidget(box2);

    widget->setLayout(layout);
    return widget;
}

QWidget *ServerDialog::createAdvancedTab(){
    QVBoxLayout *layout = new QVBoxLayout;

    god_selectlimited_spinbox = new QSpinBox;
    god_selectlimited_spinbox->setMinimum(1);
    god_selectlimited_spinbox->setMaximum(99);
    god_selectlimited_spinbox->setValue(Config.GodSelectLimited);

    contest_mode_checkbox = new QCheckBox(tr("Contest mode"));
    contest_mode_checkbox->setChecked(Config.ContestMode);
    contest_mode_checkbox->setToolTip(tr("Requires password to login, hide screen name and disable kicking"));

    free_choose_checkbox = new QCheckBox(tr("Choose generals and cards freely"));
    free_choose_checkbox->setToolTip(tr("This option enables the cheat menu"));
    free_choose_checkbox->setChecked(Config.FreeChoose);

    free_assign_checkbox = new QCheckBox(tr("Assign role and seat freely"));
    free_assign_checkbox->setChecked(Config.value("FreeAssign").toBool());

    free_assign_self_checkbox = new QCheckBox(tr("Assign only your own role"));
    free_assign_self_checkbox->setChecked(Config.FreeAssignSelf);
    free_assign_self_checkbox->setEnabled(free_assign_checkbox->isChecked());
    connect(free_assign_checkbox,SIGNAL(toggled(bool)), free_assign_self_checkbox, SLOT(setEnabled(bool)));

    maxchoice_spinbox = new QSpinBox;
    maxchoice_spinbox->setRange(3, 10);
    maxchoice_spinbox->setValue(Config.value("MaxChoice", 5).toInt());

    forbid_same_ip_checkbox = new QCheckBox(tr("Forbid same IP with multiple connection"));
    forbid_same_ip_checkbox->setChecked(Config.ForbidSIMC);

    disable_chat_checkbox = new QCheckBox(tr("Disable chat"));
    disable_chat_checkbox->setChecked(Config.DisableChat);

    second_general_checkbox = new QCheckBox(tr("Enable second general"));

    scene_checkbox  = new QCheckBox(tr("Enable Scene"));

    scene_checkbox->setChecked(Config.EnableScene);	//changjing
    //changjing

    max_hp_label = new QLabel(tr("Max HP scheme"));

    max_hp_scheme_combobox = new QComboBox;
    max_hp_scheme_combobox->addItem(tr("Sum - 3"));
    max_hp_scheme_combobox->addItem(tr("Minimum"));
    max_hp_scheme_combobox->addItem(tr("Average"));
    max_hp_scheme_combobox->setCurrentIndex(Config.MaxHpScheme);
    second_general_checkbox->setChecked(Config.Enable2ndGeneral);


    basara_checkbox = new QCheckBox(tr("Enable Basara"));
    basara_checkbox->setChecked(Config.EnableBasara);
    updateButtonEnablility(mode_group->checkedButton());
    connect(mode_group,SIGNAL(buttonClicked(QAbstractButton*)),this,SLOT(updateButtonEnablility(QAbstractButton*)));

    hegemony_checkbox = new QCheckBox(tr("Enable Hegemony"));
    hegemony_checkbox->setChecked(Config.EnableHegemony);
    hegemony_checkbox->setEnabled(basara_checkbox->isChecked());
    connect(basara_checkbox,SIGNAL(toggled(bool)),hegemony_checkbox, SLOT(setEnabled(bool)));

    announce_ip_checkbox = new QCheckBox(tr("Annouce my IP in WAN"));
    announce_ip_checkbox->setChecked(Config.AnnounceIP);
    announce_ip_checkbox->setEnabled(true); // not support now

    address_edit = new QLineEdit;
    address_edit->setText(Config.Address);

    #if QT_VERSION >= 0x040700
    address_edit->setPlaceholderText(tr("Public IP or domain"));
    #endif

    QPushButton *detect_button = new QPushButton(tr("Detect my WAN IP"));
    connect(detect_button, SIGNAL(clicked()), this, SLOT(onDetectButtonClicked()));

    //address_edit->setEnabled(announce_ip_checkbox->isChecked());
    // connect(announce_ip_checkbox, SIGNAL(toggled(bool)), address_edit, SLOT(setEnabled(bool)));

    port_edit = new QLineEdit;
    port_edit->setText(QString::number(Config.ServerPort));
    port_edit->setValidator(new QIntValidator(1, 9999, port_edit));


    //
    node_address_edit = new QLineEdit;
    node_address_edit->setText(Config.value("NodeAddress", "119.146.144.144").toString());

    node_port_edit = new QLineEdit;
    node_port_edit->setText(QString::number(Config.value("NodePort", 9527u).toUInt()));
    node_port_edit->setValidator(new QIntValidator(1, 9999, node_port_edit));

    clearserverlog_checkbox = new QCheckBox(tr("Clear server log automatic"));
    clearserverlog_checkbox->setChecked(Config.ClearServerLog);
    clearserverlog_checkbox->setEnabled(true); //

    layout->addLayout(HLay(new QLabel(tr("GodSelectLimited")), god_selectlimited_spinbox));
    layout->addWidget(contest_mode_checkbox);
    layout->addWidget(forbid_same_ip_checkbox);
    layout->addWidget(disable_chat_checkbox);
    layout->addLayout(HLay(free_choose_checkbox, free_assign_checkbox));
    layout->addWidget(free_assign_self_checkbox);
    layout->addLayout(HLay(new QLabel(tr("Upperlimit for general")), maxchoice_spinbox));
    layout->addWidget(second_general_checkbox);
    layout->addLayout(HLay(max_hp_label, max_hp_scheme_combobox));
    layout->addLayout(HLay(basara_checkbox, hegemony_checkbox));
    layout->addWidget(scene_checkbox);		//changjing
    layout->addWidget(announce_ip_checkbox);
    layout->addLayout(HLay(new QLabel(tr("Address")), address_edit));
    layout->addWidget(detect_button);
    layout->addLayout(HLay(new QLabel(tr("Port")), port_edit));
    layout->addLayout(HLay(new QLabel(tr("NodeAddress")), node_address_edit)); //20111218
    layout->addLayout(HLay(new QLabel(tr("NodePort")), node_port_edit)); //20111218
    layout->addWidget(clearserverlog_checkbox);
    layout->addStretch();

    QWidget *widget = new QWidget;
    widget->setLayout(layout);

    max_hp_label->setVisible(Config.Enable2ndGeneral);
    connect(second_general_checkbox, SIGNAL(toggled(bool)), max_hp_label, SLOT(setVisible(bool)));
    max_hp_scheme_combobox->setVisible(Config.Enable2ndGeneral);
    connect(second_general_checkbox, SIGNAL(toggled(bool)), max_hp_scheme_combobox, SLOT(setVisible(bool)));

    return widget;
}

QWidget *ServerDialog::createAITab(){
    QVBoxLayout *layout = new QVBoxLayout;

    ai_enable_checkbox = new QCheckBox(tr("Enable AI"));
    ai_enable_checkbox->setChecked(Config.EnableAI);

    always_trustai_checkbox = new QCheckBox(tr("Always TrustAI"));
    always_trustai_checkbox->setChecked(Config.alwaysTrustAI);
    always_trustai_checkbox->setEnabled(ai_enable_checkbox->isChecked());
    connect(ai_enable_checkbox, SIGNAL(toggled(bool)), always_trustai_checkbox, SLOT(setEnabled(bool)));

    role_predictable_checkbox = new QCheckBox(tr("Role predictable"));
    role_predictable_checkbox->setChecked(Config.value("RolePredictable", false).toBool());

    ai_chat_checkbox = new QCheckBox(tr("AI Chat"));
    ai_chat_checkbox->setChecked(Config.value("AIChat", true).toBool());

    ai_delay_spinbox = new QSpinBox;
    ai_delay_spinbox->setMinimum(0);
    ai_delay_spinbox->setMaximum(5000);
    ai_delay_spinbox->setValue(Config.AIDelay);
    ai_delay_spinbox->setSuffix(tr(" millisecond"));

    layout->addWidget(ai_enable_checkbox);
    layout->addWidget(always_trustai_checkbox);
    layout->addWidget(role_predictable_checkbox);
    layout->addWidget(ai_chat_checkbox);
    layout->addLayout(HLay(new QLabel(tr("AI delay")), ai_delay_spinbox));
    layout->addStretch();

    QWidget *widget = new QWidget;
    widget->setLayout(layout);
    return widget;
}

void ServerDialog::ensureEnableAI(){
    ai_enable_checkbox->setChecked(true);
}

void ServerDialog::updateButtonEnablility(QAbstractButton *button)
{
    if(!button)return;
    if(button->objectName().contains("scenario")
            || button->objectName().contains("mini")
            || button->objectName().contains("1v1")
            || button->objectName().contains("1v3"))
    {
        basara_checkbox->setChecked(false);
        basara_checkbox->setEnabled(false);
    }
    else
    {
        basara_checkbox->setEnabled(true);
    }

    if(button->objectName().contains("mini")){
        mini_scene_button->setEnabled(true);
        second_general_checkbox->setChecked(false);
        second_general_checkbox->setEnabled(false);
    }
    else
    {
        second_general_checkbox->setEnabled(true);
        mini_scene_button->setEnabled(false);
    }
}

void BanlistDialog::switchTo(int item)
{
    this->item = item;
    list = lists.at(item);
    if(add2nd) add2nd->setVisible((list->objectName()=="Pairs"));
}


BanlistDialog::BanlistDialog(QWidget *parent, bool view)
    :QDialog(parent),add2nd(NULL)
{
    setWindowTitle(tr("Select generals that are excluded"));

    if(ban_list.isEmpty())
        ban_list << "Roles" << "1v1" << "Basara" << "Hegemony" << "Pairs";
    QVBoxLayout *layout = new QVBoxLayout;

    QTabWidget *tab = new QTabWidget;
    layout->addWidget(tab);
    connect(tab,SIGNAL(currentChanged(int)),this,SLOT(switchTo(int)));

    foreach(QString item, ban_list)
    {
        if(item == "Pairs") continue;
        QWidget *apage = new QWidget;

        list = new QListWidget;
        list->setObjectName(item);

        QStringList banlist = Config.value(QString("Banlist/%1").arg(item)).toStringList();
        foreach(QString name, banlist){
            addGeneral(name);
        }

        lists << list;

        QVBoxLayout * vlay = new QVBoxLayout;
        vlay->addWidget(list);
        //vlay->addLayout(hlayout);
        apage->setLayout(vlay);

        tab->addTab(apage,Sanguosha->translate(item));
    }

    QWidget *apage = new QWidget;

    list = new QListWidget;
    list->setObjectName("Pairs");
    this->list = list;
    foreach(QString banned, BanPair::getAllBanSet().toList()){
        addGeneral(banned);
    }
    foreach(QString banned, BanPair::getSecondBanSet().toList()){
        add2ndGeneral(banned);
    }
    foreach(BanPair pair, BanPair::getBanPairSet().toList()){
        addPair(pair.first, pair.second);
    }

    QVBoxLayout *vlay = new QVBoxLayout;
    vlay->addWidget(list);
    apage->setLayout(vlay);
    tab->addTab(apage,Sanguosha->translate("Pairs"));
    lists << list;

    QPushButton *add = new QPushButton(tr("Add ..."));
    QPushButton *remove = new QPushButton(tr("Remove"));
    if(!view)add2nd = new QPushButton(tr("Add 2nd general ..."));
    QPushButton *ok = new QPushButton(tr("OK"));

    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(this, SIGNAL(accepted()), this, SLOT(saveAll()));
    connect(remove, SIGNAL(clicked()), this, SLOT(doRemoveButton()));
    connect(add, SIGNAL(clicked()), this, SLOT(doAddButton()));
    if(!view)connect(add2nd, SIGNAL(clicked()), this, SLOT(doAdd2ndButton()));

    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    if(!view){
        hlayout->addWidget(add2nd);
        add2nd->hide();
        hlayout->addWidget(add);
        hlayout->addWidget(remove);
        list = lists.first();
    }

    hlayout->addWidget(ok);
    layout->addLayout(hlayout);

    setLayout(layout);

    foreach(QListWidget * alist , lists)
    {
        if(alist->objectName() == "Pairs")continue;
        alist->setIconSize(General::TinyIconSize);
        alist->setViewMode(QListView::IconMode);
        alist->setDragDropMode(QListView::NoDragDrop);
    }
}

void BanlistDialog::addGeneral(const QString &name){
    if(list->objectName() == "Pairs"){
        QString text = QString(tr("Banned for all: %1")).arg(Sanguosha->translate(name));
        QListWidgetItem *item = new QListWidgetItem(text);
        item->setData(Qt::UserRole, QVariant::fromValue(name));
        list->addItem(item);
    }
    else{
        const General *general = Sanguosha->getGeneral(name);
        QIcon icon(general->getPixmapPath("tiny"));
        QString text = Sanguosha->translate(name);
        QListWidgetItem *item = new QListWidgetItem(icon, text, list);
        item->setSizeHint(QSize(60,60));
        item->setData(Qt::UserRole, name);
    }
}

void BanlistDialog::add2ndGeneral(const QString &name){
    QString text = QString(tr("Banned for second general: %1")).arg(Sanguosha->translate(name));
    QListWidgetItem *item = new QListWidgetItem(text);
    item->setData(Qt::UserRole, QVariant::fromValue(QString("+%1").arg(name)));
    list->addItem(item);
}

void BanlistDialog::addPair(const QString &first, const QString &second){
    QString trfirst = Sanguosha->translate(first);
    QString trsecond = Sanguosha->translate(second);
    QListWidgetItem *item = new QListWidgetItem(QString("%1 + %2").arg(trfirst, trsecond));
    item->setData(Qt::UserRole, QVariant::fromValue(QString("%1+%2").arg(first, second)));
    list->addItem(item);
}

void BanlistDialog::doAddButton(){
    FreeChooseDialog *chooser = new FreeChooseDialog(this, (list->objectName() == "Pairs"));
    connect(chooser, SIGNAL(general_chosen(QString)), this, SLOT(addGeneral(QString)));
    connect(chooser, SIGNAL(pair_chosen(QString,QString)), this, SLOT(addPair(QString, QString)));
    chooser->exec();
}

void BanlistDialog::doAdd2ndButton(){
    FreeChooseDialog *chooser = new FreeChooseDialog(this, false);
    connect(chooser, SIGNAL(general_chosen(QString)), this, SLOT(add2ndGeneral(QString)));
    chooser->exec();
}

void BanlistDialog::doRemoveButton(){
    int row = list->currentRow();
    if(row != -1)
        delete list->takeItem(row);
}

void BanlistDialog::save(){
    QSet<QString> banset;

    int i;
    for(i=0; i<list->count(); i++){
        banset << list->item(i)->data(Qt::UserRole).toString();
    }

    QStringList banlist = banset.toList();
    Config.setValue(QString("Banlist/%1").arg(ban_list.at(item)), QVariant::fromValue(banlist));
}

void BanlistDialog::saveAll()
{
    for(int i=0;i<lists.length();i++)
    {
        switchTo(i);
        save();
    }
    BanPair::loadBanPairs();
}

void ServerDialog::edit1v1Banlist(){
    BanlistDialog *dialog = new BanlistDialog(this);
    dialog->exec();
}

QGroupBox *ServerDialog::create3v3Box(){
    QGroupBox *box = new QGroupBox(tr("3v3 options"));
    box->setEnabled(Config.GameMode == "06_3v3");

    QVBoxLayout *vlayout = new QVBoxLayout;

    standard_3v3_radiobutton = new QRadioButton(tr("Standard mode"));
    new_3v3_radiobutton = new QRadioButton(tr("New Mode"));
    QRadioButton *extend = new QRadioButton(tr("Extension mode"));
    QPushButton *extend_edit_button = new QPushButton(tr("General selection ..."));
    extend_edit_button->setEnabled(false);
    connect(extend, SIGNAL(toggled(bool)), extend_edit_button, SLOT(setEnabled(bool)));
    connect(extend_edit_button, SIGNAL(clicked()), this, SLOT(select3v3Generals()));

    exclude_disaster_checkbox = new QCheckBox(tr("Exclude disasters"));
    exclude_disaster_checkbox->setChecked(Config.value("3v3/ExcludeDisasters", true).toBool());

    {
        QComboBox *combobox = new QComboBox;
        combobox->addItem(tr("Normal"), "Normal");
        combobox->addItem(tr("Random"), "Random");
        combobox->addItem(tr("All roles"), "AllRoles");

        role_choose_combobox = combobox;

        QString scheme = Config.value("3v3/RoleChoose", "Normal").toString();
        if(scheme == "Random")
            combobox->setCurrentIndex(1);
        else if(scheme == "AllRoles")
            combobox->setCurrentIndex(2);
    }

    vlayout->addWidget(standard_3v3_radiobutton);
    vlayout->addWidget(new_3v3_radiobutton);
    vlayout->addLayout(HLay(extend, extend_edit_button));
    vlayout->addWidget(exclude_disaster_checkbox);
    vlayout->addLayout(HLay(new QLabel(tr("Role choose")), role_choose_combobox));
    box->setLayout(vlayout);

    bool using_extension = Config.value("3v3/UsingExtension", false).toBool();
    bool using_new_mode = Config.value("3v3/UsingNewMode", false).toBool();
    if(using_extension)
        extend->setChecked(true);
    else if(using_new_mode)
        new_3v3_radiobutton->setChecked(true);
    else
        standard_3v3_radiobutton->setChecked(true);

    return box;
}

QGroupBox *ServerDialog::createGameModeBox(){
    QGroupBox *mode_box = new QGroupBox(tr("Game mode"));
    mode_group = new QButtonGroup;

    QObjectList item_list;

    {
        // normal modes
        QMap<QString, QString> modes = Sanguosha->getAvailableModes();
        QMapIterator<QString, QString> itor(modes);
        while(itor.hasNext()){
            itor.next();

            QRadioButton *button = new QRadioButton(itor.value());
            button->setObjectName(itor.key());
            mode_group->addButton(button);

            if(itor.key() == "06_3v3"){
                // add 3v3 options
                QGroupBox *box = create3v3Box();
                connect(button, SIGNAL(toggled(bool)), box, SLOT(setEnabled(bool)));

                item_list << button << box;
            }else{
                item_list << button;
            }

            if(itor.key() == Config.GameMode)
                button->setChecked(true);
        }
    }

    {
        // add scenario modes
        QRadioButton *scenario_button = new QRadioButton(tr("Scenario mode"));
        scenario_button->setObjectName("scenario");
        mode_group->addButton(scenario_button);

        scenario_combobox = new QComboBox;
        QStringList names = Sanguosha->getScenarioNames();
        foreach(QString name, names){
            QString scenario_name = Sanguosha->translate(name);
            const Scenario *scenario = Sanguosha->getScenario(name);
            int count = scenario->getPlayerCount();
            QString text = tr("%1 (%2 persons)").arg(scenario_name).arg(count);
            scenario_combobox->addItem(text, name);
        }

        if(mode_group->checkedButton() == NULL){
            int index = names.indexOf(Config.GameMode);
            if(index != -1){
                scenario_button->setChecked(true);
                scenario_combobox->setCurrentIndex(index);
            }
        }
        //mini scenes
        QRadioButton *mini_scenes = new QRadioButton(tr("Mini Scenes"));
        mini_scenes->setObjectName("mini");
        mode_group->addButton(mini_scenes);

        mini_scene_combobox = new QComboBox;
        int index = -1;
        int stage = Config.value("MiniSceneStage",1).toInt();
        for(int i =1;i<=stage;i++)
        {
            QString name = QString::number(i);
            name = name.rightJustified(2,'0');
            name = name.prepend("_mini_");
            QString scenario_name = Sanguosha->translate(name);
            const Scenario *scenario = Sanguosha->getScenario(name);
            int count = scenario->getPlayerCount();
            QString text = tr("%1 (%2 persons)").arg(scenario_name).arg(count);
            mini_scene_combobox->addItem(text, name);

            if(name == Config.GameMode)index = i-1;
        }

        if(index>=0)
        {
            mini_scene_combobox->setCurrentIndex(index);
            mini_scenes->setChecked(true);
        }
        else if(Config.GameMode == "custom_scenario")
            mini_scenes->setChecked(true);



        mini_scene_button = new QPushButton(tr("Custom Mini Scene"));
        connect(mini_scene_button, SIGNAL(clicked()), this, SLOT(doCustomAssign()));

        mini_scene_button->setEnabled(mode_group->checkedButton() ?
                                          mode_group->checkedButton()->objectName() == "mini" :
                                          false);

        item_list << HLay(scenario_button, scenario_combobox);
        item_list << HLay(mini_scenes, mini_scene_combobox);
        item_list << HLay(mini_scenes, mini_scene_button);
    }

    QVBoxLayout *left = new QVBoxLayout;
    QVBoxLayout *right = new QVBoxLayout;

    for(int i=0; i<item_list.length(); i++){
        QObject *item = item_list.at(i);

        QVBoxLayout *side = i < item_list.length()/2 - 2 ? left : right;

        if(item->isWidgetType()){
            QWidget *widget = qobject_cast<QWidget *>(item);
            side->addWidget(widget);
        }else{
            QLayout *item_layout = qobject_cast<QLayout *>(item);
            side->addLayout(item_layout);
        }
    }

    right->addStretch();

    QHBoxLayout *layout = new QHBoxLayout;
    layout->addLayout(left);
    layout->addLayout(right);

    mode_box->setLayout(layout);

    return mode_box;
}


QLayout *ServerDialog::createButtonLayout(){
    QHBoxLayout *button_layout = new QHBoxLayout;
    button_layout->addStretch();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    QPushButton *cancel_button = new QPushButton(tr("Cancel"));

    button_layout->addWidget(ok_button);
    button_layout->addWidget(cancel_button);

    connect(ok_button, SIGNAL(clicked()), this, SLOT(onOkButtonClicked()));
    connect(cancel_button, SIGNAL(clicked()), this, SLOT(reject()));

    return button_layout;
}

void ServerDialog::onDetectButtonClicked(){
    QString host = "www.net.cn";
    QString path = "/static/customercare/yourIP.asp";
    QHttp *http = new QHttp(this);
    http->setHost(host);

    connect(http, SIGNAL(done(bool)), this, SLOT(onHttpDone(bool)));
    http->get(path);
}

void ServerDialog::onHttpDone(bool error){
    QHttp *http = qobject_cast<QHttp *>(sender());

    if(error){
        QMessageBox::warning(this, tr("Warning"), http->errorString());
    }else{
        QRegExp rx("(\\d+\\.\\d+\\.\\d+\\.\\d+)");
        int index = rx.indexIn(http->readAll());
        if(index != -1){
            QString addr = rx.capturedTexts().at(0);
            address_edit->setText(addr);
        }

        http->deleteLater();
    }
}

void ServerDialog::onOkButtonClicked(){
    if(announce_ip_checkbox->isChecked() && address_edit->text().isEmpty()){
        QMessageBox::warning(this, tr("Warning"), tr("Please fill address when you want to annouce your server's IP"));
    }else
        accept();
}

Select3v3GeneralDialog::Select3v3GeneralDialog(QDialog *parent)
    :QDialog(parent)
{
    setWindowTitle(tr("Select generals in extend 3v3 mode"));

    ex_generals = Config.value("3v3/ExtensionGenerals").toStringList().toSet();

    QVBoxLayout *layout = new QVBoxLayout;

    tab_widget = new QTabWidget;
    fillTabWidget();

    QPushButton *ok_button = new QPushButton(tr("OK"));
    connect(ok_button, SIGNAL(clicked()), this, SLOT(accept()));
    QHBoxLayout *hlayout = new QHBoxLayout;
    hlayout->addStretch();
    hlayout->addWidget(ok_button);

    layout->addWidget(tab_widget);
    layout->addLayout(hlayout);

    setLayout(layout);

    setMinimumWidth(550);

    connect(this, SIGNAL(accepted()), this, SLOT(save3v3Generals()));
}

void Select3v3GeneralDialog::fillTabWidget(){
    QList<const Package *> packages = Sanguosha->findChildren<const Package *>();
    foreach(const Package *package, packages){
        switch(package->getType()){
        case Package::GeneralPack:
        case Package::MixedPack: {
                QListWidget *list = new QListWidget;
                list->setIconSize(General::TinyIconSize);
                list->setViewMode(QListView::IconMode);
                list->setDragDropMode(QListView::NoDragDrop);
                fillListWidget(list, package);

                tab_widget->addTab(list, Sanguosha->translate(package->objectName()));
            }
        default:
            break;
        }
    }
}

void Select3v3GeneralDialog::fillListWidget(QListWidget *list, const Package *pack){
    QList<const General *> generals = pack->findChildren<const General *>();
    foreach(const General *general, generals){
        if(general->isHidden())
            continue;

        QListWidgetItem *item = new QListWidgetItem(list);
        item->setData(Qt::UserRole, general->objectName());
        item->setIcon(QIcon(general->getPixmapPath("tiny")));

        bool checked = false;
        if(ex_generals.isEmpty()){
            checked = (pack->objectName() == "standard" || pack->objectName() == "wind")
                      && general->objectName() != "yuji";
        }else
            checked = ex_generals.contains(general->objectName());

        if(checked)
            item->setCheckState(Qt::Checked);
        else
            item->setCheckState(Qt::Unchecked);
    }

    QAction *action = new QAction(tr("Check/Uncheck all"), list);
    list->addAction(action);
    list->setContextMenuPolicy(Qt::ActionsContextMenu);
    list->setResizeMode(QListView::Adjust);

    connect(action, SIGNAL(triggered()), this, SLOT(toggleCheck()));
}

void ServerDialog::doCustomAssign(){
    CustomAssignDialog *dialog = new CustomAssignDialog(this);

    connect(dialog, SIGNAL(scenario_changed()), this, SLOT(setMiniCheckBox()));
    dialog->exec();
}

void ServerDialog::setMiniCheckBox(){
    mini_scene_combobox->setEnabled(false);
}

void Select3v3GeneralDialog::toggleCheck(){
    QWidget *widget = tab_widget->currentWidget();
    QListWidget *list = qobject_cast<QListWidget *>(widget);

    if(list == NULL || list->item(0) == NULL)
        return;

    bool checked = list->item(0)->checkState() != Qt::Checked;

    int i;
    for(i=0; i<list->count(); i++)
        list->item(i)->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
}

void Select3v3GeneralDialog::save3v3Generals(){
    ex_generals.clear();

    int i;
    for(i=0; i<tab_widget->count(); i++){
        QWidget *widget = tab_widget->widget(i);
        QListWidget *list = qobject_cast<QListWidget *>(widget);
        if(list){
            int i;
            for(i=0; i<list->count(); i++){
                QListWidgetItem *item = list->item(i);
                if(item->checkState() == Qt::Checked)
                    ex_generals << item->data(Qt::UserRole).toString();
            }
        }
    }

    QStringList list = ex_generals.toList();
    QVariant data = QVariant::fromValue(list);
    Config.setValue("3v3/ExtensionGenerals", data);
}

void ServerDialog::select3v3Generals(){
    QDialog *dialog = new Select3v3GeneralDialog(this);
    dialog->exec();
}

bool ServerDialog::config(){
    exec();

    if(result() != Accepted)
        return false;

    Config.ServerName = server_name_edit->text();
    Config.OperationTimeout = timeout_spinbox->value();
    Config.OperationNoLimit = nolimit_checkbox->isChecked();
    Config.ContestMode = contest_mode_checkbox->isChecked();
    Config.FreeChoose = free_choose_checkbox->isChecked();
    Config.FreeAssignSelf = free_assign_self_checkbox->isChecked() && free_assign_checkbox->isEnabled();
    Config.ForbidSIMC = forbid_same_ip_checkbox->isChecked();
    Config.DisableChat = disable_chat_checkbox->isChecked();
    Config.Enable2ndGeneral = second_general_checkbox->isChecked();
    Config.EnableScene = scene_checkbox->isChecked();		//changjing
    Config.EnableBasara= basara_checkbox->isChecked() && basara_checkbox->isEnabled();
    Config.EnableHegemony = hegemony_checkbox->isChecked() && hegemony_checkbox->isEnabled();
    Config.MaxHpScheme = max_hp_scheme_combobox->currentIndex();
    Config.AnnounceIP = announce_ip_checkbox->isChecked();
    Config.Address = address_edit->text();
    Config.EnableAI = ai_enable_checkbox->isChecked();
    Config.AIDelay = ai_delay_spinbox->value();
    Config.ServerPort = port_edit->text().toInt();
    Config.GodSelectLimited = god_selectlimited_spinbox->value();

    // 20111220 by highlandz
    Config.NodeAddress=node_address_edit->text();
    Config.NodePort=node_port_edit->text().toUShort();
    Config.ClearServerLog = clearserverlog_checkbox->isChecked();
    Config.alwaysTrustAI = always_trustai_checkbox->isChecked() && always_trustai_checkbox->isEnabled();

    // game mode
    QString objname = mode_group->checkedButton()->objectName();
    if(objname == "scenario")
        Config.GameMode = scenario_combobox->itemData(scenario_combobox->currentIndex()).toString();
    else if(objname == "mini"){
        if(mini_scene_combobox->isEnabled())
            Config.GameMode = mini_scene_combobox->itemData(mini_scene_combobox->currentIndex()).toString();
        else
            Config.GameMode = "custom_scenario";
    }
    else
        Config.GameMode = objname;

    Config.setValue("ServerName", Config.ServerName);
    Config.setValue("GameMode", Config.GameMode);
    Config.setValue("OperationTimeout", Config.OperationTimeout);
    Config.setValue("OperationNoLimit", Config.OperationNoLimit);
    Config.setValue("ContestMode", Config.ContestMode);
    Config.setValue("FreeChoose", Config.FreeChoose);
    Config.setValue("FreeAssign", free_assign_checkbox->isChecked());
    Config.setValue("FreeAssignSelf", Config.FreeAssignSelf);
    Config.setValue("MaxChoice", maxchoice_spinbox->value());
    Config.setValue("ForbidSIMC", Config.ForbidSIMC);
    Config.setValue("DisableChat", Config.DisableChat);
    Config.setValue("Enable2ndGeneral", Config.Enable2ndGeneral);
    Config.setValue("EnableScene", Config.EnableScene);	//changjing
    Config.setValue("EnableBasara",Config.EnableBasara);
    Config.setValue("EnableHegemony",Config.EnableHegemony);
    Config.setValue("MaxHpScheme", Config.MaxHpScheme);
    Config.setValue("EnableAI", Config.EnableAI);
    Config.setValue("RolePredictable", role_predictable_checkbox->isChecked());
    Config.setValue("AIChat", ai_chat_checkbox->isChecked());
    Config.setValue("AIDelay", Config.AIDelay);
    Config.setValue("ServerPort", Config.ServerPort);
    Config.setValue("AnnounceIP", Config.AnnounceIP);
    Config.setValue("Address", Config.Address);

    Config.setValue("GodSelectLimited", Config.GodSelectLimited);

    Config.setValue("NodeAddress", Config.NodeAddress);
    Config.setValue("NodePort", Config.NodePort);
    Config.setValue("ClearServerLog", Config.ClearServerLog);
    Config.setValue("alwaysTrustAI", Config.alwaysTrustAI);

    Config.beginGroup("3v3");
    Config.setValue("UsingExtension", !standard_3v3_radiobutton->isChecked() && !new_3v3_radiobutton->isChecked());
    Config.setValue("RoleChoose", role_choose_combobox->itemData(role_choose_combobox->currentIndex()).toString());
    Config.setValue("ExcludeDisaster", exclude_disaster_checkbox->isChecked());
    Config.setValue("UsingNewMode", new_3v3_radiobutton->isChecked());
    Config.endGroup();

    QSet<QString> ban_packages;
    QList<QAbstractButton *> checkboxes = extension_group->buttons();
    foreach(QAbstractButton *checkbox, checkboxes){
        if(!checkbox->isChecked()){
            QString package_name = checkbox->objectName();
            Sanguosha->addBanPackage(package_name);
            ban_packages.insert(package_name);
        }
    }

    Config.BanPackages = ban_packages.toList();
    Config.setValue("BanPackages", Config.BanPackages);

    if(Config.ContestMode){
        ContestDB *db = ContestDB::GetInstance();
        return db->loadMembers();
    }

    return true;
}

int Server::TimerCounts=0;

Server::Server(QObject *parent)
    :QObject(parent)
{
    server = new NativeServerSocket;
    server->setParent(this);

    //synchronize ServerInfo on the server side to avoid ambiguous usage of Config and ServerInfo
    ServerInfo.parse(Sanguosha->getSetupString());

    createNewRoom();

    connect(server, SIGNAL(new_connection(ClientSocket*)), this, SLOT(processNewConnection(ClientSocket*)));
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(deleteLater()));

    current = NULL;

    if(!Config.Address.isEmpty() && Config.ServerPort){
        nodeList[Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+Config.GameMode]=0;
    }

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timerTrigger()));
    timer->start(60*10*1000); // 10 mins zzzz

    ssclient=new NativeClientSocket;
    ssclient->setParent(this);
    connect(ssclient, SIGNAL(message_got(char*)), this, SLOT(process_SS_Reply(char*)));
    connect(ssclient, SIGNAL(error_message(QString)), this, SLOT(process_SS_error_message(QString)));
    if(Config.AnnounceIP && !Config.Address.isEmpty() && !QString::number(Config.ServerPort).isEmpty()) //20111220
    {
        foreach( QString tmp, Config.HistoryNodeList ){
            QStringList tmplist = tmp.split(":");
            ssclient->connectToNode(tmplist[0],tmplist[1].toInt());
            ssclient->send("nodealive "+Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+Config.GameMode);
            ssclient->disconnectFromHost();
        }
        Config.HistoryNodeList.clear();
        Config.remove("HistoryNodeList");

        ssclient->connectToNode(Config.NodeAddress,Config.NodePort);
        ssclient->send("iamnode "+Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+Config.GameMode);
    }
}

void Server::broadcast(const QString &msg){
    QString to_sent = msg.toUtf8().toBase64();
    to_sent = ".:" + to_sent;
    foreach(Room *room, rooms)
        room->broadcastInvoke("speak", to_sent);
}

bool Server::listen(){
    return server->listen();
}

void Server::daemonize(){
    server->daemonize();
}

Room *Server::createNewRoom(){
    // 20111218 add RoomID
    static int RoomID = 0;
    RoomID ++;
    Room *new_room = new Room(this, Config.GameMode);
    new_room->setTag("RoomID",RoomID); // set room id
    new_room->setTag("RoomOwnerScreenName"," "); // set blank tag
    new_room->setTag("DrawPileCount",QString::number(new_room->getDrawPileCount())); // set draw pile count
    QString error_msg = new_room->createLuaState();

    if(!error_msg.isEmpty()){
        QMessageBox::information(NULL, tr("Lua scripts error"), error_msg);
        return NULL;
    }

    current = new_room;
    rooms.insert(current);

    connect(current, SIGNAL(room_message(QString)), this, SIGNAL(server_message(QString)));
    connect(current, SIGNAL(game_over(QString)), this, SLOT(gameOver()));
    connect(current, SIGNAL(room_finished()), this, SLOT(roomFinished()));

    return current;
}

void Server::processNewConnection(ClientSocket *socket){
    if(Config.ForbidSIMC){
        QString addr = socket->peerAddress();
        if(addresses.contains(addr)){
            socket->disconnectFromHost();
            emit server_message(tr("Forbid the connection of address %1").arg(addr));
            return;
        }else
            addresses.insert(addr);
    }

    connect(socket, SIGNAL(disconnected()), this, SLOT(cleanup()));
    socket->send("checkVersion " + Sanguosha->getVersion());
    socket->send("setup " + Sanguosha->getSetupString());
    emit server_message(tr("%1 connected").arg(socket->peerName()));

    connect(socket, SIGNAL(message_got(char*)), this, SLOT(processRequest(char*)));
}

static inline QString ConvertFromBase64(const QString &base64){
    QByteArray data = QByteArray::fromBase64(base64.toAscii());
    return QString::fromUtf8(data);
}

void Server::processRequest(char *request){

    ClientSocket *socket = qobject_cast<ClientSocket *>(sender());
    socket->disconnect(this, SLOT(processRequest(char*)));

    QString cmd(request);
    cmd=cmd.trimmed();
    if(cmd.indexOf("signup")==-1)
    {
        QString msg="";
        // cmd of HallDialog
        if(cmd.indexOf("iamnode")!=-1) // new node
        {
            QStringList tmplist = cmd.split(" ");
            if (!nodeList.contains(tmplist[1]))
            {
                nodeList.insert(tmplist[1], clock());
                //emit server_message("Receive and iamnode 0: "+tmplist[1]);
                Config.HistoryNodeList << tmplist[1];
                Config.HistoryNodeList.removeDuplicates();
                Config.HistoryNodeList.sort();
                Config.setValue("HistoryNodeList", Config.HistoryNodeList);
            }
            else if(nodeList[tmplist[1]]!=0)
            {
                nodeList.insert(tmplist[1], clock());
                Config.HistoryNodeList << tmplist[1];
                Config.HistoryNodeList.removeDuplicates();
                Config.HistoryNodeList.sort();
                Config.setValue("HistoryNodeList", Config.HistoryNodeList);
                //emit server_message("Receive and iamnode 1: "+tmplist[1]);
            }

            QHashIterator <QString, long> i(nodeList); // send my nodelist to new node
            while (i.hasNext()) {
                i.next();

                if(clock()-i.value()>30*1*1000*60 && i.value()!=0)
                {
                    nodeList.remove(i.key());
                    //emit server_message("Remove node: "+i.key());
                }
                else
                {
                    socket->send("nodelist "+i.key());
                    //emit server_message("Send nodelist: "+i.key());
                }
            }
            socket->disconnectFromHost();
            return;
        }
        else if(cmd.indexOf("nodealive")!=-1) // received announcement of I am still online
        {
            QStringList tmplist = cmd.split(" ");
            if (!nodeList.contains(tmplist[1])) // a new one
            {
                nodeList.insert(tmplist[1], clock());
                Config.HistoryNodeList << tmplist[1];
                Config.HistoryNodeList.removeDuplicates();
                Config.HistoryNodeList.sort();
                Config.setValue("HistoryNodeList", Config.HistoryNodeList);
                //emit server_message("Receive nodealive 0: "+tmplist[1]);
            }
            else if(nodeList[tmplist[1]]!=0) // update last time
            {
                nodeList.insert(tmplist[1], clock());
                Config.HistoryNodeList << tmplist[1];
                Config.HistoryNodeList.removeDuplicates();
                Config.HistoryNodeList.sort();
                Config.setValue("HistoryNodeList", Config.HistoryNodeList);
                //emit server_message("Receive nodealive 1: "+tmplist[1]);
            }
            else
            {;}
            socket->disconnectFromHost();
            return;
        }
        else if(cmd.indexOf("Qnodelist .")!=-1) // for nodelist request
        {
            QHashIterator <QString, long> i(nodeList);
            while (i.hasNext()) {
                i.next();

                if(clock()-i.value()>30*1*1000*60 && i.value()!=0)
                {
                    nodeList.remove(i.key());
                    //emit server_message("Qnodelist Remove node: "+i.key());
                }
                else
                {
                    socket->send("nodelist "+i.key());
                    //emit server_message("Qnodelist: "+i.key());
                }
            }
            socket->disconnectFromHost();
            return;
        }
        else if(cmd.indexOf("Qnodeinfo")!=-1)
        {
            QStringList tmp=cmd.split(" ");
            if(tmp.count()==2)
            {
                // 20111218 count toggle ready players
                int robotcount=0;
                int playercount=0;
                int roomcount=0; // 20111220
                foreach(Room *room, rooms){
                    // playercount+=room->players.length();
                    foreach(ServerPlayer *player, room->getPlayers()){
                        if(player->getState() != "robot")
                            playercount ++;
                        else if(player->getState() == "robot")
                            robotcount++;
                    }
                    // 20111220
                    QString roomname=room->getTag("RoomOwnerScreenName").toString().trimmed();
                    if(!roomname.isEmpty())
                    {
                        roomcount++;
                    }
                    //emit server_message("Qnodeinfo: "+reply);
                }
                //                QString reply=Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+Config.GameMode+":"
                //                              +QString::number(Config.Enable2ndGeneral?1:0)+":"+QString::number(this->rooms.count()-1)
                //                              +":"+QString::number(playercount) +":"+tmp[1] ;
                QString base64=Config.ServerName.toUtf8().toBase64();
                QString version;
//                if(Sanguosha->getVersion().contains(QChar(':')))
//                    version = QString("%1%2").arg(Sanguosha->getVersion().split(QChar(':')).at(0)).arg(Sanguosha->getVersion().split(QChar(':')).at(1));
//                else
                version = Sanguosha->getVersion();
                QString reply=Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+base64+":"
                              +version+":"+Config.GameMode+":"+QString::number(Config.Enable2ndGeneral?1:0)+":"
                              +QString::number(roomcount)+":"+QString::number(playercount) +":"+tmp[1] ;
                socket->send("nodeinfo "+reply);
            }
            socket->disconnectFromHost();
            return;
        }
        else if(cmd.indexOf("refreshRooms")!=-1) // 20111220 for reconnect
        {
            // emit server_message("refreshRooms");
            if(rooms.count()<=1){socket->send("room 0"); socket->disconnectFromHost(); return;}
            else if(cmd=="refreshRooms .")
            {
                foreach(Room *room, rooms)
                {
                    QString roomstatus="";

                    if(room->game_finished){ roomstatus="Finished"; }
                    else{
                        if(room->game_started){ roomstatus="Playing"; }
                        else{
                            if(room->isFull()){roomstatus="HALT!!!";} // is prepareing
                            else{roomstatus="Waiting";}
                        }
                    }

                    int robotcount=0;
                    int playercount=0;
                    foreach(ServerPlayer *player, room->getPlayers()){
                        if(player->getState() != "robot")
                            playercount ++;
                        else if(player->getState() == "robot")
                            robotcount++;
                    }
                    QString base64=room->getTag("RoomOwnerScreenName").toString().trimmed();
                    // 20111220
                    if(!base64.isEmpty())
                    {
                        base64=base64.toUtf8().toBase64();
                        // 20111222
                        socket->send("room " + room->getTag("RoomID").toString() +":"+ base64 +":"+ QString::number(playercount) + ":" + roomstatus );
                    }
                }
                socket->disconnectFromHost();
                return;
            }
            else{
                QRegExp rx("(refreshRooms?) (.+):(.+)?\n");
                if(!rx.exactMatch(request)){
                    emit server_message(tr("Invalid refreshRooms string: %1").arg(request));
                    socket->send("warn INVALID_FORMAT");
                    socket->disconnectFromHost();
                    return;
                }
                QStringList texts = rx.capturedTexts();
                QString command = texts.at(1);
                QString screen_name = ConvertFromBase64(texts.at(2));
                QString lastobjname = texts.at(3);
                QString reply;

                QHashIterator <QString, QString> i(name2objname);
                while (i.hasNext()) {
                    i.next();
                    if(i.key()==screen_name && i.value()==lastobjname)
                    {
                        ServerPlayer *player = players.value(lastobjname);
                        if(player && player->getState() == "offline" && player->isAlive()){
                            reply=player->getRoom()->getTag("RoomID").toString();
                            command ="foundRoomID "+reply;
                            socket->send(command);
                            socket->disconnectFromHost();
                            return;
                        }
                    }
                }
                // 20111220 can't find room
                command="refreshRooms .";
                processRequest(command.toLatin1().data());
                return;
            }
            socket->disconnectFromHost();
            return;
        }
        else if(cmd.indexOf("reJoinRoom")!=-1) // 20111220 rejoinroom
        {
            QRegExp rx("(reJoinRoom?) (.+):(.+):(.+):(.+):(.+)?\n");
            if(!rx.exactMatch(request)){
                emit server_message(tr("Invalid reJoinRoom string: %1").arg(request));
                socket->send("warn INVALID_FORMAT");
                socket->disconnectFromHost();
                return;
            }

            QStringList texts = rx.capturedTexts();
            //QString command = texts.at(1);
            QString screen_name = ConvertFromBase64(texts.at(2));
            //QString avatar = texts.at(3);
            //QString roomid = texts.at(4);
            QString version = texts.at(5);
            QString sgsname = texts.at(6);

            if(version!=Sanguosha->getVersion()){
                socket->send("warn INVALID_VERSION");
                socket->disconnectFromHost();
                return;
            }

            // 20111222
            ServerPlayer *player = players.value(sgsname);
            if(player && player->getState() == "offline" && screen_name==player->screenName() && player->isAlive()){
                Room *room=player->getRoom();
                if(room->game_started && !room->game_finished){
                    player->getRoom()->reconnect(player, socket);
                }
            }
        }
        else if(cmd.indexOf("joinRoom")!=-1)
        {
            QRegExp rx("(joinRoom?) (.+):(.+):(.+):(.+)?\n");
            if(!rx.exactMatch(request)){
                emit server_message(tr("Invalid joinRoom string: %1").arg(request));
                socket->send("warn INVALID_FORMAT");
                socket->disconnectFromHost();
                return;
            }

            QStringList texts = rx.capturedTexts();
            QString command = texts.at(1);
            QString screen_name = ConvertFromBase64(texts.at(2));
            QString avatar = texts.at(3);
            QString roomid = texts.at(4);
            QString version = texts.at(5);

            if(version!=Sanguosha->getVersion()){
                socket->send("warn INVALID_VERSION");
                socket->disconnectFromHost();
                return;
            }

            foreach(Room *room, rooms){
                QString rid=room->getTag("RoomID").toString();
                if(rid==roomid && !room->game_started && !room->game_finished && !(room->isFull()) )
                {
                    ServerPlayer *player = room->addSocket(socket);
                    room->signup(player, screen_name, avatar, false);
                    return;
                }
            }

            socket->send("warn JOIN_ROOM_FAIL");
            socket->disconnectFromHost();
            return;
        }
        else if(cmd.indexOf("createRoom")!=-1)
        {
            QRegExp rx("(createRoom?) (.+):(.+):(.+)?\n");
            if(!rx.exactMatch(request)){
                emit server_message(tr("Invalid createRoom string: %1").arg(request));
                socket->send("warn INVALID_FORMAT");
                socket->disconnectFromHost();
                return;
            }

            QStringList texts = rx.capturedTexts();
            QString command = texts.at(1);
            QString screen_name = ConvertFromBase64(texts.at(2));
            QString avatar = texts.at(3);
            QString version = texts.at(4);

            if(version!=Sanguosha->getVersion()){
                socket->send("warn INVALID_VERSION");
                socket->disconnectFromHost();
                return;
            }

            //Room* newRoom =  createNewRoom(); 20111222
            createNewRoom();
            ServerPlayer *player = current->addSocket(socket);
            current->setTag("RoomOwnerScreenName",screen_name);
            current->signup(player, screen_name, avatar, false);
            // socket->send(".roomEntered " + newRoom->getTag("RoomID").toString()); // ��֪�ͻ��˵�ClientPlayer����ķ����;
            current->broadcastProperty(player, "owner");
            return;
        }
        else
        {
            // unknow msg;
            socket->send("warn UNKNOW_" + QString(request));
            socket->disconnectFromHost();
            return;
        }
    }
    else // ԭsignup�Ĺ���
    {
        QRegExp rx("(signupr?) (.+):(.+)(:.+)?\n");
        if(!rx.exactMatch(request)){
            emit server_message(tr("Invalid signup string: %1").arg(request));
            socket->send("warn INVALID_FORMAT");
            socket->disconnectFromHost();
            return;
        }

        QStringList texts = rx.capturedTexts();
        QString command = texts.at(1);
        QString screen_name = ConvertFromBase64(texts.at(2));
        QString avatar = texts.at(3);

        if(Config.ContestMode){
            QString password = texts.value(4);
            if(password.isEmpty()){
                socket->send("warn REQUIRE_PASSWORD");
                socket->disconnectFromHost();
                return;
            }

            password.remove(QChar(':'));
            ContestDB *db = ContestDB::GetInstance();
            if(!db->checkPassword(screen_name, password)){
                socket->send("warn WRONG_PASSWORD");
                socket->disconnectFromHost();
                return;
            }
        }

        if(command == "signupr"){
            foreach(QString objname, name2objname.values(screen_name)){
                ServerPlayer *player = players.value(objname);
                if(player && player->getState() == "offline"){
                    player->getRoom()->reconnect(player, socket);
                    return;
                }
            }
        }

        //  if(current == NULL || current->isFull())
        //       createNewRoom();

        //  ServerPlayer *player = current->addSocket(socket);
        //  current->signup(player, screen_name, avatar, false);
        if(current == NULL || current->isFull()){
            createNewRoom();
            current->setTag("RoomOwnerScreenName",screen_name);
        }
        ServerPlayer *player = current->addSocket(socket);
        current->signup(player, screen_name, avatar, false);
    }
}

void Server::cleanup(){
    const ClientSocket *socket = qobject_cast<const ClientSocket *>(sender());

    if(Config.ForbidSIMC)
        addresses.remove(socket->peerAddress());
}

void Server::signupPlayer(ServerPlayer *player){
    name2objname.insert(player->screenName(), player->objectName());
    players.insert(player->objectName(), player);
}

void Server::gameOver(){
    Room *room = qobject_cast<Room *>(sender());
    rooms.remove(room);

    foreach(ServerPlayer *player, room->findChildren<ServerPlayer *>()){
        name2objname.remove(player->screenName(), player->objectName());
        players.remove(player->objectName());
    }
}

void Server::gamesOver(){
    name2objname.clear();
    players.clear();
    rooms.clear();
}

void Server::process_SS_Reply(char *reply){
    if(strlen(reply) <= 2) return;
    QString cmd=QString(reply);
    cmd=cmd.trimmed();
    if(cmd.indexOf("nodelist")!=-1)
    {
        QStringList tmplist = cmd.split(" ");
        if (!nodeList.contains(tmplist[1])) // new node
        {
            nodeList.insert(tmplist[1], clock());
        }
        else if(nodeList[tmplist[1]]!=0) // update old node
        {
            nodeList.insert(tmplist[1], clock());
        }
    }
}

void Server::process_SS_error_message(QString msg){
    ;
}

void Server::timerTrigger() // clear node list and announce
{
    TimerCounts++;
    if(Config.ClearServerLog){emit(clearlog());}

    // check room stats
    foreach(Room *room, rooms){
        int roomid=room->getTag("RoomID").toString().toInt();
        QString lastDrawPile=room->getTag("DrawPileCount").toString().trimmed();
        QString currentDrawPile=QString::number(room->getDrawPileCount());
        QString swapTimes=room->getTag("SwapPile").toString().trimmed();

        if(roomid!=1){
            if(lastDrawPile!=swapTimes+currentDrawPile){
                room->setTag("DrawPileCount",swapTimes+currentDrawPile);
            }
            else{
                // the room is halt!!!!
                emit server_message("FOUND RoomID:" + QString::number(roomid) + " HALT!!!");
            }
        }
    }


    if(!Config.AnnounceIP)
        return;

    //emit server_message("timerTrigger");
    QHashIterator <QString, long> i(nodeList);
    while (i.hasNext()) {
        i.next();
        if(i.value()!=0)
        {
            if(clock()-i.value()>30*1*1000*60)
            {
                nodeList.remove(i.key());
                Config.HistoryNodeList.removeOne(i.key());
                Config.HistoryNodeList.removeDuplicates();
                Config.HistoryNodeList.sort();
                Config.setValue("HistoryNodeList", Config.HistoryNodeList);
            }
            else
            {
                QString tmp=i.key();
                QStringList tmplist = tmp.split(":");
                NativeClientSocket *socket=new NativeClientSocket;
                socket->connectToNode(tmplist[0],tmplist[1].toInt());
                if(TimerCounts>48){
                    TimerCounts=0;
                    socket->send("iamnode "+Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+Config.GameMode);
                }
                else{
                    socket->send("nodealive "+Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+Config.GameMode);
                    socket->disconnectFromHost();
                }
            }
        }
    }
}

void Server::roomFinished(){
    Room *room = qobject_cast<Room *>(sender());
    if(current==room){current=NULL;}
    rooms.remove(room);

    foreach(ServerPlayer *player, room->findChildren<ServerPlayer *>()){
        name2objname.remove(player->screenName(), player->objectName());
        players.remove(player->objectName());
    }
    room->releaseSource();
}

void Server::processCmdLine()
{
    QLineEdit *cmd = qobject_cast<QLineEdit *>(sender());
    QString servercmd=cmd->text().toLower();
    emit server_message(servercmd);
    cmd->clear();
    if(servercmd.indexOf("cls")!=-1){ emit(clearlog()); return;}
    else if(servercmd.indexOf("msg on")!=-1){
        foreach(Room *room, rooms){
            connect(room, SIGNAL(room_message(QString)), this, SIGNAL(server_message(QString)));
        }
        return;
    }
    else if(servercmd.indexOf("msg off")!=-1){
        foreach(Room *room, rooms){
            disconnect(room, SIGNAL(room_message(QString)), this, SIGNAL(server_message(QString)));
        }
        return;
    }
    else if(servercmd.startsWith("broadcast ")){
//        QStringList tmp=servercmd.split(" ");
        QString temp = servercmd.mid(10);
        // QString base64 = tmp[1].toUtf8().toBase64();
        this->broadcast(temp);
        return;
    }
    else if(servercmd.indexOf("splayerlist")!=-1){
        QHashIterator <QString, QString> i(name2objname);
        while (i.hasNext()) {
            i.next();
            emit server_message(i.key()+" -> "+i.value());
        }
        return;
    }
    else if(servercmd.indexOf("playerlist")!=-1){
        foreach(Room *room, rooms)
        {
            foreach(ServerPlayer *player, room->getPlayers()){
                if(player->getState()!="robot"){
                    QString tmp;
                    tmp="RoomID:"+room->getTag("RoomID").toString()+" -> "; // +room->getTag("RoomOwnerScreenName").toString();
                    tmp+="     PlayerID:" + player->objectName() + " - " + player->screenName() + " - " + player->getState();
                    tmp+=" - IP:" + player->getIp();
                    emit server_message(tmp);
                }
            }
        }
        return;
    }
    else if(servercmd.indexOf("ailist")!=-1){
        foreach(Room *room, rooms)
        {
            foreach(ServerPlayer *player, room->getPlayers()){
                if(player->getState()=="robot"){
                    QString tmp;
                    tmp="RoomID:"+room->getTag("RoomID").toString()+" -> "; // +room->getTag("RoomOwnerScreenName").toString();
                    tmp+="     PlayerID:" + player->objectName() + " - " + player->screenName() + " - " + player->getState();
                    tmp+=" - IP:" + player->getIp();
                    emit server_message(tmp);
                }
            }
        }
        return;
    }
    else if(servercmd.indexOf("nodelist")!=-1)
    {
        QHashIterator <QString, long> i(nodeList);
        while (i.hasNext()) {
            i.next();

            if(clock()-i.value()>30*1*1000*60 && i.value()!=0)
            {
                nodeList.remove(i.key());
                emit server_message("cmd nodelist: Remove node-> "+i.key());
            }
            else
            {
                emit server_message("cmd nodelist: "+i.key());
            }
        }
        return;
    }
    /*else if(servercmd.indexOf("iamnode")!=-1)
    {
        QStringList tmp=servercmd.split(" ");
        QStringList node= tmp[1].split(":");
        ssclient->connectToNode(node[0],node[1].toInt());
        ssclient->send("iamnode "+Config.Address.toLower()+":"+QString::number(Config.ServerPort)+":"+Config.GameMode);
    }
    else if(servercmd.indexOf("addnode")!=-1)
    {
        QStringList tmp=servercmd.split(" ");
        nodeList.insert(tmp[1], clock());
        Config.HistoryNodeList << tmp[1];
        Config.HistoryNodeList.removeDuplicates();
        Config.HistoryNodeList.sort();
        Config.setValue("HistoryNodeList", Config.HistoryNodeList);
        return;
    }*/
    else if(servercmd.indexOf("roomlist")!=-1)
    {
        foreach(Room *room, rooms)
        {
            QString roomstatus="";
            if(room->game_started){ roomstatus="Playing"; } else {roomstatus="Waiting";}
            int robotcount=0;
            int playercount=0;
            foreach(ServerPlayer *player, room->getPlayers()){
                if(player->getState() != "robot")
                    playercount ++;
                else if(player->getState() == "robot")
                    robotcount++;
            }
            QString base64=room->getTag("RoomOwnerScreenName").toString();// .toUtf8().toBase64();
            // 20111220
            if(!base64.trimmed().isEmpty())
            {
                // 20111222
                emit server_message("cmd roomlist: " + room->getTag("RoomID").toString() +":"+ base64 +":"+ QString::number(playercount) + ":" + roomstatus );
            }
        }
        return;
    }
    else if(servercmd.indexOf("myconfig")!=-1)
    {
        QString tmp;
        emit server_message("ServerName: "+Config.ServerName);
        emit server_message("Version: "+Sanguosha->getVersion());
        emit server_message("GameMode: "+Config.GameMode);
        // emit server_message("BanPackages: "+Sanguosha->getSetupString());
        tmp=Config.Enable2ndGeneral?"Yes":"No";
        emit server_message("Enable2ndGeneral: "+tmp);
        emit server_message("AddresssPort: "+Config.Address+":"+QString::number(Config.ServerPort));
        tmp=Config.EnableAI?"Yes":"No";
        emit server_message("EnableAI: "+tmp);
        tmp=Config.alwaysTrustAI?"Yes":"No";
        emit server_message("AlwaysTrustAI: "+tmp);
        emit server_message("AIDelay: "+QString::number(Config.AIDelay));
        tmp=Config.AnnounceIP?"Yes":"No";
        emit server_message("AnnounceIP: "+tmp);
        emit server_message("NodeAddressPort: "+Config.NodeAddress+":"+QString::number(Config.NodePort));
        return;
    }
    else if(servercmd.indexOf("help")!=-1){
        QString show="Current support commands:\n=============================================\n";
        show.append("help\t\tshow support commands list\n");
        show.append("cls\t\tclear the log box\n");
        show.append("msg on\t\tturn on room message\n");
        show.append("msg off\t\tturn off room message\n");
        show.append("broadcast\tsend message to all rooms\n");
        show.append("splayerlist\tserver side players with sgs name\n");
        show.append("playerlist\t\tcurrent players on server\n");
        show.append("ailist\t\tcurrent AIs on server\n");
        show.append("roomlist\t\tcurrent rooms on server\n");
        show.append("nodelist\t\tall nodes found on Inet\n");
        show.append("myconfig\t\tshow server settings\n");
        show.append("delroom\t\tRoom del\n");
        show.append("kick\t\tKICK player by object name\n");
        //show.append("iamnode\t\tdeclare I am a node to another node\n");
        //show.append("addnode\t\tadd a new node manually\n");
        emit server_message(show);
        return;
    }
    else if(servercmd.indexOf("delroom ")!=-1){
        QStringList tmplist=servercmd.split(" ");
        QString tmp=tmplist[1];
        int roomid=tmp.toInt();
        bool result=delRoom(roomid);
        if(result){
            emit server_message("Room del success.");
        }
        else{emit server_message("Room del fail.");}
        return;
    }
    else if(servercmd.startsWith("kick ")!=-1){
        QStringList tmplist=servercmd.split(" ");
        QString name=tmplist[1];
        bool result = false;
        foreach(Room *room, rooms)
        {
            foreach(ServerPlayer *player, room->getPlayers()){
                if(player->objectName() == name && player->getState()!="robot" && player->getState()!="offline"){
                    result = true;
                    player->emitDisconnect();
                    break;
                }
            }
            if(result)
                break;
        }
        if(result){
            emit server_message(QString("%1 kick success.").arg(name));
        }
        else{emit server_message(QString("%1 kick fail.").arg(name));}
        return;
    }
    else{;}
}


bool Server::delRoom(int roomid){
    //killPlayer lord
    //    ServerPlayer *theLord;
    //    QStringList text;
    bool result=false;
    QString rid;
    foreach(Room *room, rooms){
        rid=room->getTag("RoomID").toString();
        if(rid==QString::number(roomid))
        {
            if(current==room){current=NULL;}
            foreach(ServerPlayer *player, room->findChildren<ServerPlayer *>()){
                name2objname.remove(player->screenName(), player->objectName());
                players.remove(player->objectName());
                if(player->getState() == "online" || player->getState() == "trust"){
                    // player->setSocket(NULL);
                    player->emitDisconnect();
                }
            }
            if(rooms.contains(room))
            {
                rooms.remove(room);
                room->releaseSource();
            }
            result=true;
        }
    }
    return result;
}
