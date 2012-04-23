#ifndef BGMPACKAGE_H
#define BGMPACKAGE_H

#include "package.h"
#include "card.h"
#include "skill.h"
#include "standard.h"

class BGMPackage : public Package{
    Q_OBJECT

public:
    BGMPackage();
};

class LihunCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE LihunCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class DaheCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE DaheCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const;
};

#endif // BGMPACKAGE_H
