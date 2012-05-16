#include "carditem.h"
#include "engine.h"
#include "skill.h"
#include "clientplayer.h"
#include "settings.h"

#include <cmath>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>
#include <QFocusEvent>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

CardItem::CardItem(const Card *card)
{
    setCard(card);
    m_isSelected = false;
    auto_back = true;
    frozen = false;

    setTransformOriginPoint(pixmap.width()/2, pixmap.height()/2);
    setAcceptHoverEvents(true);

    QPixmap frame_pixmap("image/system/frame/good.png");
    frame = new QGraphicsPixmapItem(frame_pixmap, this);
    frame->setPos(-12, -12);
    frame->hide();

    avatar = NULL;
    owner_pixmap = NULL;

    m_opacityAtHome = 1.0;
    m_currentAnimation = NULL;
}

void CardItem::setCard(const Card* card)
{
    if (card != NULL)
    {
        Pixmap::load(card->getPixmapPath(), false);
        icon_pixmap.load(card->getIconPath());
        suit_pixmap.load(QString("image/system/suit/%1.png").arg(card->getSuitString()));
        small_suit_pixmap.load(QString("image/system/log/%1.png").arg(card->getSuitString()));
        cardsuit_pixmap.load(QString("image/system/cardsuit/%1.png").arg(card->getSuitString()));
        number_pixmap.load(QString("image/system/%1/%2.png").arg(card->isBlack()?"black":"red").arg(card->getNumberString()));
        setToolTip(card->getDescription());
    }
    else
    {
        Pixmap::load("image/system/card-back.png");
    }
    m_card = card;
    filtered_card = card;
}

void CardItem::setEnabled(bool enabled)
{
    if (enabled)
    {
        setHomeOpacity(1.0);
        setOpacity(1.0);
    }
    else
    {
        setHomeOpacity(0.7);
        setOpacity(0.7);
    }
    Pixmap::setEnabled(enabled);
    goBack(true);
}

CardItem::CardItem(const QString &general_name)
    :m_card(NULL), filtered_card(NULL), auto_back(true), frozen(false)
{
    changeGeneral(general_name);
    m_currentAnimation = NULL;
    m_opacityAtHome = 1.0;
}

CardItem::~CardItem()
{
    if (m_currentAnimation != NULL)
    {
        m_currentAnimation->stop();
        delete m_currentAnimation;
        m_currentAnimation = NULL;
    }
}

void CardItem::changeGeneral(const QString &general_name){
    setObjectName(general_name);

    const General *general = Sanguosha->getGeneral(general_name);
    if(general){
        changePixmap(general->getPixmapPath("card"));
        setToolTip(general->getSkillDescription());
    }else{
        changePixmap("image/system/unknown.png");
        setToolTip(QString());
    }
}

const Card *CardItem::getCard() const{
    return m_card;
}

void CardItem::filter(const FilterSkill *filter_skill){
    if(filter_skill){
        if(filter_skill->viewFilter(this))
            filtered_card = filter_skill->viewAs(this);
    }else
        filtered_card = m_card;
}

const Card *CardItem::getFilteredCard() const{
    return filtered_card;
}

void CardItem::setHomePos(QPointF home_pos){
    this->home_pos = home_pos;
}

QPointF CardItem::homePos() const{
    return home_pos;
}

void CardItem::goBack(bool playAnimation, bool doFade){
    if (playAnimation)
    {
        getGoBackAnimation(doFade);
        if (m_currentAnimation != NULL)
        {
            m_currentAnimation->start();
        }
    }
    else
    {
        m_animationMutex.lock();
        if (m_currentAnimation != NULL)
        {
            m_currentAnimation->stop();
            delete m_currentAnimation;
            m_currentAnimation = NULL;
        }
        setPos(homePos());
        m_animationMutex.unlock();
    }
}

QAbstractAnimation* CardItem::getGoBackAnimation(bool doFade)
{
    m_animationMutex.lock();
    if (m_currentAnimation != NULL)
    {
        m_currentAnimation->stop();
        delete m_currentAnimation;
        m_currentAnimation = NULL;
    }
    QPropertyAnimation *goback = new QPropertyAnimation(this, "pos");
    goback->setEndValue(home_pos);
    goback->setEasingCurve(QEasingCurve::OutQuad);
    goback->setDuration(Config.S_MOVE_CARD_ANIMATION_DURAION);

    if(doFade){
        QParallelAnimationGroup *group = new QParallelAnimationGroup;

        QPropertyAnimation *disappear = new QPropertyAnimation(this, "opacity");
        double middleOpacity = (m_opacityAtHome == 0.0) ? 1.0 : m_opacityAtHome;
        disappear->setEndValue(m_opacityAtHome);
        disappear->setKeyValueAt(0.2, middleOpacity);
        disappear->setKeyValueAt(0.8, middleOpacity);
        disappear->setDuration(Config.S_MOVE_CARD_ANIMATION_DURAION);

        group->addAnimation(goback);
        group->addAnimation(disappear);

        // card is disabled while moving
        // setEnabled(false);
        m_currentAnimation = group;
    }
    else
    {
        setOpacity(this->isEnabled() ? 1.0 : 0.7);
        m_currentAnimation = goback;
    }
    m_animationMutex.unlock();
    return m_currentAnimation;
}

const QPixmap &CardItem::getSuitPixmap() const{
    return suit_pixmap;
}

const QPixmap &CardItem::getSmallSuitPixmap() const{
    return small_suit_pixmap;
}

const QPixmap &CardItem::getNumberPixmap() const{
    return number_pixmap;
}

const QPixmap &CardItem::getIconPixmap() const{
    return icon_pixmap;
}

void CardItem::setFrame(const QString &result){
    QString path = QString("image/system/frame/%1.png").arg(result);
    QPixmap frame_pixmap(path);
    if(!frame_pixmap.isNull()){
        frame->setPixmap(frame_pixmap);
        frame->show();
    }
}

void CardItem::showAvatar(const General *general){
    if(general){
        if(avatar == NULL){
            avatar = new QGraphicsPixmapItem(this);
            avatar->setPos(44, 87);
        }

        avatar->setPixmap(QPixmap(general->getPixmapPath("tiny")));
        avatar->show();
    }else{
        if(avatar)
            avatar->hide();
    }
}

void CardItem::hideFrame(){
    frame->hide();
}

void CardItem::setAutoBack(bool auto_back){
    this->auto_back = auto_back;
}

bool CardItem::isEquipped() const{
    return Self->hasEquip(m_card);
}


void CardItem::setFrozen(bool is_frozen){
    frozen = is_frozen;
}

bool CardItem::isFrozen() const{
    return frozen;
}

CardItem *CardItem::FindItem(const QList<CardItem *> &items, int card_id){
    foreach(CardItem *item, items){
        if (item->getCard() == NULL)
        {
            if (card_id == Card::S_UNKNOWN_CARD_ID) return item;
            else continue;
        }
        if(item->getCard()->getId() == card_id)
            return item;
    }

    return NULL;
}

void CardItem::reduceZ()
{
    if (this->zValue()>0) this->setZValue(this->zValue()-0.8);
}

void CardItem::promoteZ()
{
    if (this->zValue()<0) this->setZValue(this->zValue()+0.8);
}

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent *){
    if(isFrozen())
        return;

    if(hasFocus())
        emit clicked();
}

void CardItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *){
    if (isFrozen()) return;

    if (auto_back){
        goBack(true, false);
    }else{
        emit released();
    }
    emit leave_hover();
}

void CardItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event){
    if(hasFocus()){
        QPointF down_pos = event->buttonDownPos(Qt::LeftButton);
        setPos(this->mapToParent(event->pos() - down_pos));
    }
}

void CardItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event){
    if(isFrozen())
        return;

    if(hasFocus()){
        event->accept();
        emit double_clicked();
    }
    else emit toggle_discards();
}

void CardItem::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    emit enter_hover();
}

void CardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    emit leave_hover();
}

void CardItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    Pixmap::paint(painter, option, widget);

    if (m_card) {
        painter->drawPixmap(0, 14, cardsuit_pixmap);
        painter->drawPixmap(0, 2, number_pixmap);
        if(owner_pixmap)painter->drawPixmap(0,0,*owner_pixmap);
    }
}


void CardItem::writeCardDesc(QString card_owner)
{
     if(m_card){
         int x, y;
         x=(93-card_owner.toLocal8Bit().length()*6)/2;
         y=120;
         owner_pixmap = new QPixmap(pixmap.size());
         owner_pixmap->fill(QColor(0,0,0,0));
         QPainter painter(owner_pixmap);

#ifdef Q_OS_WIN32
         static QFont card_desc_font("SimSun", 9, QFont::Normal);
         painter.setFont(card_desc_font);
         painter.setPen(Qt::black);
#endif

         painter.drawText(x, y-1, card_owner);
         painter.drawText(x, y+1, card_owner);
         painter.drawText(x-1, y, card_owner);
         painter.drawText(x+1, y, card_owner);

         painter.setPen(QColor(228,213,160));
         painter.drawText(x, y, card_owner);
         update();
     }
}

void CardItem::deleteCardDescription(){
    delete owner_pixmap;
    owner_pixmap = NULL;
}
