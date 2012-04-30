#ifndef NOSTALGIA_H
#define NOSTALGIA_H

#include "package.h"
#include "card.h"

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

#endif // NOSTALGIA_H
