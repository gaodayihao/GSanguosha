#ifndef PHOTO_H
#define PHOTO_H

#include "pixmap.h"
#include "player.h"
#include "carditem.h"
#include "protocol.h"
#include "TimedProgressBar.h"
#include "GeneralCardContainerUI.h"
#include "SkinBank.h"

#include <QGraphicsObject>
#include <QPixmap>
#include <QComboBox>
#include <QProgressBar>

class ClientPlayer;
class RoleCombobox;
class QPushButton;

class PhasePixmap : public Pixmap
{
    Q_OBJECT
public:
    explicit PhasePixmap(QGraphicsItem *parent, const QRect &Area);
    void setPhase(const char* typeofPhase, int index);

private:
    QRect _m_Area;
    const QSanRoomSkin* _m_roomSkin;
};

class Photo : public PlayerCardContainer
{
    Q_OBJECT

public:
    explicit Photo();
    void setPlayer(const ClientPlayer *player);
    const ClientPlayer *getPlayer() const;
    void speak(const QString &content);
    QList<CardItem*> removeCardItems(const QList<int> &card_id, Player::Place place);
    void installEquip(CardItem *equip);
    void installDelayedTrick(CardItem *trick);
    void hideAvatar();
    void showCard(int card_id);
    void showProgressBar(QSanProtocol::Countdown countdown);
    void hideProgressBar();
    void setEmotion(const QString &emotion, bool permanent = false);
    void tremble();
    void showSkillName(const QString &skill_name);
    void createRoleCombobox();
    void setOrder(int order);
    void revivePlayer();

    enum FrameType{
        Playing,
        Responsing,
        SOS,
        NoFrame,
        SOSR,
        Selected
    };

    void setFrame(FrameType type);
    virtual QRectF boundingRect() const;
public slots:
    void updateAvatar();    
    void updateSmallAvatar();
    void updateReadyItem(bool visible);
    void updatePhase();
    void updatePile(const QString &pile_name);
    void refresh();
    void hideEmotion();
    void hideSkillName();
    void setDrankState();
    void setActionState();
    void killPlayer();

protected:
    bool _addCardItems(QList<CardItem*> &card_items, Player::Place place);
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    virtual QVariant itemChange(GraphicsItemChange change, const QVariant &value);

    QList<CardItem*> m_takenOffCards;
private:
    const QSanRoomSkin* _m_roomSkin;
    const QSanRoomSkin::CommonLayout* _m_commonLayout;
    const QSanRoomSkin::PhotoLayout* _m_photoLayout;
    const ClientPlayer *player;
    QPixmap avatar, small_avatar;
    QGraphicsPixmapItem *ready_item, *lord_frame;
    QPixmap _m_mainFrame;
    QPixmap _m_handCardIcon;
    QGraphicsPixmapItem *_m_kingdomIcon;
    QPixmap _m_kindomColorMaskIcon;
    RoleCombobox *role_combobox;
    QGraphicsProxyWidget  *pile_button;
    QGraphicsPixmapItem *action_item, *save_me_item;
    bool permanent;

    QGraphicsTextItem *mark_item;

    CardItem *weapon, *armor, *defensive_horse, *offensive_horse;
    QList<CardItem **> equips;
    QGraphicsRectItem *equip_rects[4];

    QList<QGraphicsPixmapItem *> judging_pixmaps;    
    QList<CardItem *> judging_area;

    QMap<QString, QGraphicsPixmapItem *> mark_items;
    QMap<QString, QGraphicsSimpleTextItem *> mark_texts;

    QGraphicsPixmapItem *order_item;
    bool hide_avatar, game_start;
    QPixmap death_pixmap;
    QPixmap back_icon, chain_icon;
    PhasePixmap *phase;
    QSanCommandProgressBar *progress_bar;
    QGraphicsPixmapItem *emotion_item, *frame_item;
    QGraphicsSimpleTextItem *skill_name_item;
    QGraphicsRectItem *avatar_area, *small_avatar_area;
    FrameType _m_old_frame;

    void drawEquip(QPainter *painter, CardItem *equip, int order);
    void drawHp(QPainter *painter);
    void drawMagatama(QPainter *painter, int index, const QPixmap &pixmap);
};

#endif // PHOTOBACK_H
