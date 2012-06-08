#include "photo.h"
#include "clientplayer.h"
#include "settings.h"
#include "carditem.h"
#include "engine.h"
#include "standard.h"
#include "client.h"
#include "playercarddialog.h"
#include "rolecombobox.h"
#include "SkinBank.h"

#include <QPainter>
#include <QDrag>
#include <QGraphicsScene>
#include <QGraphicsSceneHoverEvent>
#include <QMessageBox>
#include <QGraphicsProxyWidget>
#include <QTimer>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QMenu>
#include <QGraphicsDropShadowEffect>
#include <QPixmapCache>

#include "pixmapanimation.h"

using namespace QSanProtocol;

Photo::Photo(): player(NULL),
    action_item(NULL), save_me_item(NULL), permanent(false),
    weapon(NULL), armor(NULL), defensive_horse(NULL), offensive_horse(NULL),
    order_item(NULL), hide_avatar(false)
{
    setAcceptHoverEvents(true);

    frame_item = new QGraphicsPixmapItem(this);
    frame_item->setPos(-14, -14);
    frame_item->setZValue(-1.0);

    _m_roomSkin = &QSanSkinFactory::getInstance().getCurrentSkinScheme().getRoomSkin();
    _m_photoLayout = &_m_roomSkin->getPhotoLayout();
    _m_commonLayout = &_m_roomSkin->getCommonLayout();
    _m_mainFrame = _m_roomSkin->getPixmap(QSanRoomSkin::S_SKIN_KEY_PHOTO_MAINFRAME);
    _m_handCardIcon = _m_roomSkin->getPixmap(QSanRoomSkin::S_SKIN_KEY_PHOTO_HANDCARDNUM);

    translate(-_m_photoLayout->m_normalWidth / 2, -_m_photoLayout->m_normalHeight / 2);
    chain_icon = QPixmap(_m_roomSkin->getPixmap(QSanRoomSkin::S_SKIN_KEY_PHOTO_CHAIN));

    progress_bar = new QSanCommandProgressBar;
    progress_bar->setAutoHide(true);
    progress_bar->hide();
    progress_bar->setOrientation(_m_photoLayout->m_isProgressBarHorizontal ? Qt::Horizontal : Qt::Vertical);
    progress_bar->setMaximumHeight(_m_photoLayout->m_progressBarArea.height());
    progress_bar->setMaximumWidth(_m_photoLayout->m_progressBarArea.width());

    QGraphicsProxyWidget *widget = new QGraphicsProxyWidget(this);
    widget->setWidget(progress_bar);
    widget->setPos(_m_photoLayout->m_progressBarArea.left(), _m_photoLayout->m_progressBarArea.top());

    skill_name_item = new QGraphicsSimpleTextItem(this);
    skill_name_item->setBrush(Qt::white);
    skill_name_item->setFont(Config.SmallFont);
    skill_name_item->moveBy(10, 37);

    QGraphicsDropShadowEffect * drp = new QGraphicsDropShadowEffect;
    drp->setBlurRadius(10);
    drp->setColor(Qt::yellow);
    drp->setOffset(0);
    skill_name_item->setGraphicsEffect(drp);

    emotion_item = new QGraphicsPixmapItem(this);
    emotion_item->moveBy(10, 0);
    emotion_item->setZValue(10086);

    avatar_area = new QGraphicsRectItem(_m_photoLayout->m_avatarArea, this);
    avatar_area->setPen(Qt::NoPen);

    back_icon = QPixmap(_m_roomSkin->getPixmap(QSanRoomSkin::S_SKIN_KEY_PHOTO_FACETURNEDMASK));

    small_avatar_area = new QGraphicsRectItem(_m_photoLayout->m_smallAvatarArea, this);
    small_avatar_area->setPen(Qt::NoPen);

    equips << &weapon << &armor << &defensive_horse << &offensive_horse;

    for(int i = 0; i < 4; i++){
        equip_rects[i] = new QGraphicsRectItem(QRect(1, 93 + 14 * i, 130, 18), this);
        equip_rects[i]->setPen(Qt::NoPen);
    }

    ready_item = new QGraphicsPixmapItem(QPixmap("image/system/ready.png"), this);
    ready_item->setPos(88, 99);
    ready_item->hide();

    lord_frame = new QGraphicsPixmapItem(QPixmap("image/system/lordframe.png"), this);
    lord_frame->setPos(-9, -6);
    lord_frame->setZValue(1.5);
    lord_frame->hide();

    _m_kingdomIcon = new QGraphicsPixmapItem(this);
    _m_kingdomIcon->setPos(-2, -2);
    _m_kingdomIcon->setZValue(2.5);

    phase = new PhasePixmap(this, _m_photoLayout->m_phaseArea, _m_roomSkin);
    phase->hide();

    mark_item = new QGraphicsTextItem(this);
    mark_item->setPos(-71, 73);
    mark_item->setDefaultTextColor(Qt::white);

    role_combobox = NULL;
    pile_button = NULL;
    game_start = false;
}

QRectF Photo::boundingRect() const
{
    return QRectF(0, 0, _m_photoLayout->m_widthIncludeMarkAndControl, _m_photoLayout->m_heightIncludeMarkAndControl);
}

void Photo::setOrder(int order){
    QPixmap pixmap(QString("image/system/number/%1.png").arg(order));
    if(order_item)
    {
        order_item->setPixmap(pixmap);
        order_item->setZValue(3.0);
    }
    else{
        order_item = new QGraphicsPixmapItem(pixmap, this);
        order_item->setVisible(ServerInfo.EnableSame);
        order_item->moveBy(15, 0);
        order_item->setZValue(3.0);
    }
}

void Photo::revivePlayer(){
    updateAvatar();
    updateSmallAvatar();
    this->setOpacity(1.0);
}

void Photo::createRoleCombobox(){
    role_combobox = new RoleCombobox(this);
    role_combobox->setPos(_m_photoLayout->m_normalWidth - RoleCombobox::S_ROLE_COMBO_BOX_WIDTH, 0);
    QString role = player->getRole();
    if(!ServerInfo.EnableHegemony && !role.isEmpty())
            role_combobox->fix(role);

    connect(player, SIGNAL(role_changed(QString)), role_combobox, SLOT(fix(QString)));
}

void Photo::showProgressBar(Countdown countdown){
    progress_bar->setCountdown(countdown);
    if (countdown.m_max != 0 && countdown.m_type != Countdown::S_COUNTDOWN_NO_LIMIT)
        progress_bar->show();
}

void Photo::hideProgressBar(){
    progress_bar->hide();
}

void Photo::setEmotion(const QString &emotion, bool permanent){
    this->permanent = permanent;

    if(emotion == "."){
        emotion_item->hide();
        return;
    }

    QString path = QString("image/system/emotion/%1.png").arg(emotion);
    emotion_item->setPixmap(QPixmap(path));
    emotion_item->show();

    if(emotion == "question" || emotion == "no-question" || emotion.contains("guhuo/"))
        return;

    if(!permanent)
        QTimer::singleShot(2000, this, SLOT(hideEmotion()));

    PixmapAnimation::GetPixmapAnimation(this,emotion);
}

void Photo::tremble(){
    QPropertyAnimation *vibrate = new QPropertyAnimation(this, "x");
    static qreal offset = 20;

    vibrate->setKeyValueAt(0.5, x() - offset);
    vibrate->setEndValue(x());

    vibrate->setEasingCurve(QEasingCurve::OutInBounce);

    vibrate->start(QAbstractAnimation::DeleteWhenStopped);
}

void Photo::showSkillName(const QString &skill_name){
    skill_name_item->setText(Sanguosha->translate(skill_name));
    skill_name_item->show();

    QTimer::singleShot(1500, this, SLOT(hideSkillName()));
}

void Photo::hideSkillName(){
    skill_name_item->hide();
}

void Photo::setDrankState(){
    if(player->hasFlag("drank"))
        avatar_area->setBrush(QColor(0xFF, 0x00, 0x00, 255 * 0.45));
    else
        avatar_area->setBrush(Qt::NoBrush);
}

void Photo::setActionState(){
    if(action_item == NULL){
        action_item = new QGraphicsPixmapItem(this);
        action_item->setPixmap(QPixmap("image/system/3v3/actioned.png"));
        action_item->setPos(75, 40);
    }

    action_item->setVisible(player->hasFlag("actioned"));
}

void Photo::hideEmotion(){
    if(!permanent)
        emotion_item->hide();
}

void Photo::setPlayer(const ClientPlayer *player)
{
    this->player = player;

    if(player){
        connect(player, SIGNAL(general_changed()), this, SLOT(updateAvatar()));
        connect(player, SIGNAL(general2_changed()), this, SLOT(updateSmallAvatar()));
        connect(player, SIGNAL(kingdom_changed()), this, SLOT(updateAvatar()));
        connect(player, SIGNAL(ready_changed(bool)), this, SLOT(updateReadyItem(bool)));
        connect(player, SIGNAL(state_changed()), this, SLOT(refresh()));
        connect(player, SIGNAL(phase_changed()), this, SLOT(updatePhase()));
        connect(player, SIGNAL(drank_changed()), this, SLOT(setDrankState()));
        connect(player, SIGNAL(action_taken()), this, SLOT(setActionState()));
        connect(player, SIGNAL(pile_changed(QString)), this, SLOT(updatePile(QString)));

        mark_item->setDocument(player->getMarkDoc());
    }

    updateAvatar();
}

void Photo::hideAvatar(){
    hide_avatar = true;
    _m_kingdomIcon->hide();
    update();
}

void Photo::showCard(int card_id){
    const Card *card = Sanguosha->getCard(card_id);

    CardItem *card_item = new CardItem(card);
    scene()->addItem(card_item);

    QPointF card_pos(pos() + QPointF(-40, -56));
    card_item->setPos(card_pos);
    card_item->setHomePos(card_pos);
    card_item->setZValue(10001);

    QTimer::singleShot(2000, card_item, SLOT(deleteLater()));
}

void Photo::updateAvatar(){
    if(player){
        const General *general = player->getAvatarGeneral();
        bool success = false;
        if (general)
        {
            avatar_area->setToolTip(general->getSkillDescription());
            success = avatar.load(general->getPixmapPath("small"));
        }
        QPixmap kingdom_icon(player->getKingdomIcon());
        if (!kingdom_icon.isNull())
            game_start = true;
        _m_kingdomIcon->setPixmap(kingdom_icon);
        _m_kindomColorMaskIcon.load(player->getKingdomFrame());

        if(!success){
            QPixmap pixmap(General::SmallIconSize);
            pixmap.fill(Qt::black);
            QPainter painter(&pixmap);

            painter.setPen(Qt::white);
            painter.setFont(Config.SmallFont);
            painter.drawText(0, 0, pixmap.width(), pixmap.height(),
                             Qt::AlignCenter,
                             Sanguosha->translate(player->getGeneralName()));

            avatar = pixmap;
        }

        if(player->getGameMode() == "03_3kingdoms")
            lord_frame->hide();
        else if(player->getGameMode() == "06_3v3"){
            if(player->getRole() == "renegade" || player->isLord())
                lord_frame->show();
            else
                lord_frame->hide();
        }else{
            if(player->isLord())
                lord_frame->show();
            else
                lord_frame->hide();
        }
    }else{
        avatar = QPixmap();
        _m_kindomColorMaskIcon = QPixmap();

        avatar_area->setToolTip(QString());
        small_avatar_area->setToolTip(QString());

        ready_item->hide();
        lord_frame->hide();
    }

    hide_avatar = false;

    update();
}

void Photo::updateSmallAvatar(){
    const General *general2 = player->getGeneral2();
    if(general2){
        bool success = small_avatar.load(general2->getPixmapPath("tiny"));
        small_avatar_area->setToolTip(general2->getSkillDescription());

        if(!success){
            QPixmap pixmap(General::TinyIconSize);
            pixmap.fill(Qt::black);

            QPainter painter(&pixmap);

            painter.setPen(Qt::white);
            painter.drawText(0, 0, pixmap.width(), pixmap.height(),
                             Qt::AlignCenter,
                             Sanguosha->translate(player->getGeneral2Name()));

            small_avatar = pixmap;
        }
    }

    hide_avatar = false;
    update();
}

void Photo::updateReadyItem(bool visible){
    ready_item->setVisible(visible);
}

void Photo::refresh(){
    if(player && player->getHp() <= 0 && player->isAlive() && player->getMaxHp() > 0){
        if(player->getPhase()==Player::NotActive)
            setFrame(SOS);
        else
            setFrame(Playing);

        if(save_me_item == NULL){
            QPixmap save_me("image/system/death/save-me.png");
            save_me_item = new QGraphicsPixmapItem(save_me, this);
            save_me_item->setPos(5, 15);
        }
        save_me_item->show();
    }else{
        if(save_me_item)
            save_me_item->hide();
        updatePhase();
    }

    update();
}

const ClientPlayer *Photo::getPlayer() const{
    return player;
}

void Photo::speak(const QString &content)
{

}

void Photo::installEquip(CardItem *equip){
    const EquipCard *equip_card = qobject_cast<const EquipCard *>(equip->getCard());
    int index = -1;
    switch(equip_card->location()){
    case EquipCard::WeaponLocation: weapon = equip; index = 0; break;
    case EquipCard::ArmorLocation: armor = equip; index = 1; break;
    case EquipCard::DefensiveHorseLocation: defensive_horse = equip; index = 2; break;
    case EquipCard::OffensiveHorseLocation: offensive_horse = equip; index = 3; break;
    }

    if(index >= 0)
        equip_rects[index]->setToolTip(equip_card->getDescription());

    update();
}

void Photo::installDelayedTrick(CardItem *trick){
    QGraphicsPixmapItem *item = new QGraphicsPixmapItem(this);
    item->setPixmap(QPixmap(player->topDelayedTrick()->getIconPath()));
    item->setZValue(2.0);
    QString tooltip;
    if(player->topDelayedTrick()->isVirtualCard())
        tooltip=Sanguosha->getCard((player->topDelayedTrick()->getSubcards()).at(0))->getDescription();
    else
        tooltip=player->topDelayedTrick()->getDescription();
    item->setToolTip(tooltip);

    item->setPos(-10, 16 + judging_area.count() * 19);
    judging_area << trick;
    judging_pixmaps << item;
}

QList<CardItem*> Photo::removeCardItems(const QList<int> &card_ids, Player::Place place)
{
    QList<CardItem*> result;
    if(place == Player::Hand || place == Player::Special){
         result = _createCards(card_ids);
    }else if(place == Player::Equip){
        foreach(CardItem **equip_ptr, equips){
            CardItem *equip = *equip_ptr;
            if(equip && card_ids.contains(equip->getCard()->getId())){
                result.append(equip);
                *equip_ptr = NULL;
                int index = equips.indexOf(equip_ptr);
                equip_rects[index]->setToolTip(QString());
            }
        }
    }else if(place == Player::Judging){
        foreach (int card_id, card_ids)
        {
            CardItem* card_item = CardItem::FindItem(judging_area, card_id);
            if(card_item){
                result.append(card_item);
                int index = judging_area.indexOf(card_item);
                delete judging_pixmaps.takeAt(index);
                judging_area.removeAt(index);
            }
        }
    }else if (place == Player::PlaceTakeoff){
        foreach (int card_id, card_ids)
        {
            CardItem* card_item = CardItem::FindItem(m_takenOffCards, card_id);
             if (card_item == NULL)
                 card_item = CardItem::FindItem(m_takenOffCards, Card::S_UNKNOWN_CARD_ID);
            if (card_item == NULL)
            {
                Q_ASSERT(!m_takenOffCards.isEmpty());
                card_item = m_takenOffCards.first();
            }
            int index = m_takenOffCards.indexOf(card_item);
            m_takenOffCards.removeAt(index);
            if (card_item->getId() == Card::S_UNKNOWN_CARD_ID)
            {
                const Card* card = Sanguosha->getCard(card_id);
                card_item->setCard(card);
            }
            result.append(card_item);
        }
    }
    _disperseCards(result, _m_photoLayout->m_cardMoveRegion, Qt::AlignCenter, true, false);
    update();
    return result;
}

bool Photo::_addCardItems(QList<CardItem*> &card_items, Player::Place place)
{
    _disperseCards(card_items, _m_photoLayout->m_cardMoveRegion, Qt::AlignCenter, true, false);
    double homeOpacity = 0.0;
    bool destroy = true;
    if (place == Player::PlaceTakeoff)
    {
        homeOpacity = 1.0;
        destroy  = false;
        m_takenOffCards.append(card_items);
    }
    foreach (CardItem* card_item, card_items)
        card_item->setHomeOpacity(homeOpacity);
    if (place == Player::Equip)
    {
        foreach (CardItem* card, card_items)
            installEquip(card);
        destroy = false;
    }
    else if (place == Player::Judging)
    {
        foreach (CardItem* card, card_items)
            installDelayedTrick(card);
        destroy = false;
    }
    return destroy;
}

void Photo::drawMagatama(QPainter *painter, int index, const QPixmap &pixmap){
    const int step = pixmap.width();
    painter->drawPixmap(54 + index * step, 74, pixmap);
}

void Photo::drawHp(QPainter *painter){
    int hp = qMax(0, player->getHp());

    int index = 5;
    if(player->isWounded())
        index = qBound(0, hp, 5);

    QPixmap *magatama = MagatamaWidget::GetSmallMagatama(index);
    QPixmap *zero_magatama = MagatamaWidget::GetSmallMagatama(0);

    int max_hp = player->getMaxHp();
    if (max_hp <= 5)
    {
        for(int i = 0; i< hp; i++)
            drawMagatama(painter, i, *magatama);
        for(int i = hp; i< max_hp; i++)
            drawMagatama(painter, i, *zero_magatama);
    }
    else
    {
        const QRectF textArea(72, 74, 40, 20);
        drawMagatama(painter, 0, *magatama);
        QFont hpFont("Arial", 12);
        hpFont.setBold(true);
        painter->setFont(hpFont);
        painter->drawText(textArea, tr("%1 / %2").arg(hp).arg(max_hp));
    }
}

void Photo::setFrame(FrameType type){
    static QPixmap playing_frame("image/system/frame/playing.png");
    static QPixmap responsing_frame("image/system/frame/responsing.png");
    static QPixmap sos_frame("image/system/frame/sos.png");
    static QPixmap sosr_frame("image/system/frame/sosr.png");

    QPixmap *to_draw = NULL;
    switch(type){
    case Playing: to_draw = &playing_frame; break;
    case Responsing: to_draw = &responsing_frame; break;
    case SOS: to_draw = &sos_frame; break;
    case SOSR: to_draw = &sosr_frame; break;
    default:
        break;
    }

    if(to_draw){
        frame_item->setPixmap(*to_draw);
        frame_item->show();
    }else{
        frame_item->hide();
    }
}

void Photo::updatePhase(){
    progress_bar->hide();

    if(player->getPhase() != Player::NotActive){
        int index = static_cast<int>(player->getPhase());
        if(!phase->isVisible())
            phase->show();
        phase->setPhase(index);
        setFrame(Playing);
    }
    else{
        if(phase->isVisible())
            phase->hide();
        setFrame(NoFrame);
    }
}

static bool CompareByNumber(const Card *card1, const Card *card2){
    return card1->getNumber() < card2->getNumber();
}

void Photo::updatePile(const QString &pile_name){
    QPushButton *button = NULL;
    QGraphicsProxyWidget *button_widget = NULL;

    if(pile_button == NULL){
        button = new QPushButton;
        button->setObjectName(pile_name);
        button->setProperty("private_pile","true");

        button_widget = new QGraphicsProxyWidget(this);
        button_widget->setWidget(button);
        //button_widget->setPos(pos());
        button_widget->moveBy(44, 57);
        button_widget->resize(80, 16);
        //scene()->addItem(button_widget);

        QMenu *menu = new QMenu(button);
        button->setMenu(menu);

        pile_button = button_widget;
    }else
    {
        button_widget = pile_button;
        button = qobject_cast<QPushButton *>(pile_button->widget());
    }

    ClientPlayer *who = qobject_cast<ClientPlayer *>(sender());
    if(who == NULL)
        return;

    QStringList names = who->getPileNames();
    button->menu()->clear();

    button_widget->hide();
    int active = 0;
    foreach(QString pile_name,names)
    {
        if(pile_name.startsWith("#"))
            continue;
        const QList<int> &pile = who->getPile(pile_name);
        if(!pile.isEmpty()){
            button_widget->show();
            active++;
            button->setText(QString("%1 (%2)").arg(Sanguosha->translate(pile_name)).arg(pile.length()));
        }
        else
            continue;

        QMenu *menu = button->menu();
        menu->setProperty("private_pile","true");
        //menu->clear();

        QList<const Card *> cards;
        foreach(int card_id, pile){
            const Card *card = Sanguosha->getCard(card_id);
            if (card != NULL) cards << card;
        }
        if (cards.isEmpty())
            menu->addAction(QString("%1 (%2)(%3)").arg(tr("Hide"))
                            .arg(Sanguosha->translate(pile_name))
                            .arg(pile.length()));

        qSort(cards.begin(), cards.end(), CompareByNumber);
        foreach(const Card *card, cards){
            menu->addAction(card->getSuitIcon(),
                            QString("%1 (%2)").arg(card->getFullName())
                            .arg(Sanguosha->translate(pile_name)));
        }
        menu->addSeparator();
    }
    if(active>1)button->setText(QString(tr("Multiple")));
}

void Photo::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    painter->setPen(Qt::white);
    QRect avatarRect = avatar_area->boundingRect().toRect();
    static QPixmap wait_frame("image/system/wait-frame.png");
    if(_m_kindomColorMaskIcon.isNull())
        painter->drawPixmap(avatarRect, wait_frame);
    if (!hide_avatar)
    {
        // avatar related
        painter->drawPixmap(avatarRect, avatar);
        painter->drawPixmap(small_avatar_area->boundingRect().toRect(), small_avatar);
        if(!_m_kindomColorMaskIcon.isNull())
            painter->drawPixmap(avatarRect, _m_kindomColorMaskIcon);
    }

    painter->drawPixmap(_m_photoLayout->m_mainFrameArea, _m_mainFrame);
    if (!hide_avatar)
    {
        _m_kingdomIcon->show();
    }

    if (player == NULL) return;
    painter->drawText(QRect(28, 12, 72, 14), player->screenName(), QTextOption(Qt::AlignHCenter));
    drawHp(painter);

    if(player->isDead()){
        int death_x = 5;

        if(death_pixmap.isNull()){
            QString path = player->getDeathPixmapPath();
            death_pixmap.load(path);

            if(path.contains("unknown"))
                death_x = 23;
            else
                death_pixmap = death_pixmap.scaled(death_pixmap.size() / (1.5));
        }

        painter->drawPixmap(death_x, 50, death_pixmap);
    }

    if (player != NULL && player->isAlive())
    {
        if (!player->faceUp())
        {
            painter->drawPixmap(avatarRect, back_icon);
        }
        if (player->isChained())
            painter->drawPixmap(-8, avatarRect.top() + 5, chain_icon);
    }

    if(game_start)
    {
        int n = player->getHandcardNum();
        painter->drawPixmap(QRect(0, 72, 18, 19), _m_handCardIcon);
        QFont hpFont("Arial");
        hpFont.setBold(true);
        painter->setFont(hpFont);
        QPen pen(Qt::black);
        painter->setPen(pen);
        painter->drawText(QRect(0, 73, 18, 18), QString::number(n), QTextOption(Qt::AlignCenter));
        hpFont.setBold(false);
        painter->setFont(hpFont);
    }

    QString state_str = player->getState();
    if(!state_str.isEmpty() && state_str != "online"){
        QRectF stateArea(0, avatarRect.top() + 1, 30, 15);
        stateArea.moveRight(avatarRect.right());
        painter->fillRect(stateArea, QColor(63, 57, 43));
        QPen pen(QColor(224, 217, 175));
        painter->setPen(pen);
        painter->drawText(stateArea, Sanguosha->translate(state_str), QTextOption(Qt::AlignCenter));
    }

    drawEquip(painter, weapon, 0);
    drawEquip(painter, armor, 1);
    drawEquip(painter, defensive_horse, 2);
    drawEquip(painter, offensive_horse, 3);
}

void Photo::drawEquip(QPainter *painter, CardItem *equip, int order){
    if(!equip)
        return;

    const EquipCard *card = qobject_cast<const EquipCard *>(equip->getCard());

    painter->setPen(Qt::black);
    QFont bold_font;
    bold_font.setBold(true);
    painter->setFont(bold_font);

    // draw image or name
    QString path = QString("image/small-equips/%1.png").arg(card->objectName());
    QPixmap small_equip;
    if(!QPixmapCache::find(path, &small_equip)){
        small_equip.load(path);
        QPixmapCache::insert(path, small_equip);
    }
    QRect equip_rect(1, 93 + order * 14, small_equip.width(), small_equip.height());

    if(small_equip.isNull())
        painter->drawText(20, 102 + order * 14, card->label());
    else
        painter->drawPixmap(equip_rect, small_equip);

    // draw the number of equip
    // painter->setPen(card->isRed() ? Qt::red : Qt::black);
    painter->drawText(108, 103 + order * 14, card->getNumberString());

    // draw the suit of equip
    QRect suit_rect(115, 89 + order * 14, 18, 19);
    painter->drawPixmap(suit_rect, equip->getSmallSuitPixmap());

}

QVariant Photo::itemChange(GraphicsItemChange change, const QVariant &value){
    if(change == ItemFlagsHaveChanged){
        if(!ServerInfo.EnableSame)
            order_item->setVisible(flags() & ItemIsSelectable);
    }

    return Pixmap::itemChange(change, value);
}

void Photo::killPlayer(){
    if(!avatar.isNull())
        MakeGray(avatar);

    if(!small_avatar.isNull())
        MakeGray(small_avatar);

    _m_kindomColorMaskIcon = QPixmap();

    role_combobox->fix(player->getRole());

    if(save_me_item)
        save_me_item->hide();
}

PhasePixmap::PhasePixmap(QGraphicsItem *parent, const QRect &Area, const QSanRoomSkin* roomSkin)
    : _m_Area(Area), _m_roomSkin(roomSkin)
{
    setParentItem(parent);
    setPos(_m_Area.left(), _m_Area.top());
    setZValue(1.8);
}

void PhasePixmap::setPhase(int index){
    load(_m_roomSkin->getPath(QString(QSanRoomSkin::S_SKIN_KEY_PHOTO_PHASE).arg(index)));
}
