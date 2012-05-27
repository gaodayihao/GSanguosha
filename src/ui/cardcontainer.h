#ifndef CARDCONTAINER_H
#define CARDCONTAINER_H

class CardItem;
class ClientPlayer;

#include "pixmap.h"
#include "carditem.h"
#include "GeneralCardContainerUI.h"

#include <QStack>

class CloseButton: public Pixmap{
    Q_OBJECT

public:
    CloseButton();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

signals:
    void clicked();
};

class CardContainer : public PlayerCardContainer
{
    Q_OBJECT

public:
    explicit CardContainer();
    virtual QList<CardItem*> removeCardItems(const QList<int> &card_ids, Player::Place place);
    int getFirstEnabled() const;
    void startChoose();
    void startGongxin();
    void startGongxinwithHongyan();
    void addCloseButton(bool dispose = false);
    void view(const ClientPlayer *player);

    ClientPlayer* m_currentPlayer;
public slots:
    void fillCards(const QList<int> &card_ids = QList<int>());
    void clear();
    void freezeCards(bool is_disable);

protected:

    virtual bool _addCardItems(QList<CardItem*> &card_items, Player::Place place);

private:
    QList<CardItem *> items;
    CloseButton* close_button;

    QStack<QList<CardItem *> > items_stack;

    void _addCardItem(int card_id, const QPointF &pos);

private slots:
    void grabItem();
    void chooseItem();
    void gongxinItem();

signals:
    void item_chosen(int card_id);
    void item_gongxined(int card_id);
};

class HeroCardContainer : public CardContainer
{
    Q_OBJECT

public:
    HeroCardContainer();

public slots:
    void fillCards(const QList<int> &card_ids = QList<int>());
    void clear();

private:
    QList<CardItem *> items;
};

class GuanxingBox: public Pixmap{
    Q_OBJECT

public:
    GuanxingBox();
    void reply();
    void clear();

public slots:
    void doGuanxing(const QList<int> &card_ids, bool up_only);
    void adjust();

private:
    QList<CardItem *> up_items, down_items;
    bool up_only;

    static const int start_x = 30;
    static const int start_y1 = 40;
    static const int start_y2 = 184;
    static const int middle_y = 157;
    static const int skip = 102;
    static const int card_width = 93;
};

#endif // CARDCONTAINER_H
