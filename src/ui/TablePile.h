#ifndef _DISCARD_PILE_H
#define _DISCARD_PILE_H

#include "QSanSelectableItem.h"
#include "player.h"
#include "carditem.h"
#include "protocol.h"
#include "GeneralCardContainerUI.h"
#include <QGraphicsObject>
#include <QPixmap>

class TablePile: public GeneralCardContainer
{
    Q_OBJECT
public:  
    inline TablePile() : GeneralCardContainer() {}
    virtual QList<CardItem*> removeCardItems(const QList<int> &card_ids, Player::Place place);
    inline void setSize(QSize newSize) 
    {
        setSize(newSize.width(), newSize.height());
    }
    void setSize(double width, double height);
    inline void setNumCardsVisible(int num) { m_numCardsVisible = num; }
    inline int getNumCardsVisible() { return m_numCardsVisible; }
    inline virtual void paint(QPainter *,const QStyleOptionGraphicsItem *,QWidget *) {}
    void clear(bool playAnimation = true);
    void adjustCards();
    virtual QRectF boundingRect() const;
protected:
    virtual bool _addCardItems(QList<CardItem*> &card_items, Player::Place place);
    QList<CardItem*> m_visibleCards;
    QMutex _m_mutex_pileCards;
    int m_numCardsVisible;
    QRect m_cardsDisplayRegion;
};

#endif