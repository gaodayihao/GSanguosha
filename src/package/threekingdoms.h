#ifndef THREEKINGDOMS_H
#define THREEKINGDOMS_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class GeneralCard:public Card{
    Q_OBJECT

public:
    GeneralCard(Suit suit, int number):Card(suit, number){}
    virtual QString getType() const;
    virtual CardType getTypeId() const;
};

class HeroCard: public GeneralCard{
    Q_OBJECT

public:
    Q_INVOKABLE HeroCard(const QString object_name, Card::Suit suit, int number);

    virtual QString getSubtype() const;
    virtual bool isAvailable(const Player *player) const;
    virtual QString getPixmapPath() const;
    virtual QString getDescription() const;
};

class ThreeKingdomsPackage : public Package{
    Q_OBJECT

public:
    ThreeKingdomsPackage();
};

class PrepareCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE PrepareCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class UseHeroCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE UseHeroCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class RecastHeroCard:public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE RecastHeroCard();
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

#endif // THREEKINGDOMS_H
