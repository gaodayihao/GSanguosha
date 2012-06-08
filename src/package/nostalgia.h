#ifndef NOSTALGIA_H
#define NOSTALGIA_H

#include "package.h"
#include "card.h"
#include "player.h"

class NostalgiaPackage: public Package{
    Q_OBJECT

public:
    NostalgiaPackage();
};

class NostalGeneralPackage: public Package{
    Q_OBJECT

public:
    NostalGeneralPackage();
};

class NosFanjianCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE NosFanjianCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosJujianCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE NosJujianCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosXuanhuoCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE NosXuanhuoCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosQuanjiCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE NosQuanjiCard();

    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class NosYexinCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE NosYexinCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

#endif // NOSTALGIA_H
