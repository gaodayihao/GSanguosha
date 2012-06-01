#include "threekingdoms.h"
#include "skill.h"
#include "standard.h"
#include "server.h"
#include "carditem.h"
#include "engine.h"
#include "settings.h"

#include <QFile>

QString GeneralCard::getType() const{
    return "general_card";
}

Card::CardType GeneralCard::getTypeId() const{
    return Generals;
}

HeroCard::HeroCard(const QString object_name, Card::Suit suit, int number)
    :GeneralCard(suit, number)
{
    setObjectName(object_name);
}

bool HeroCard::isAvailable(const Player *) const{
    return false;
}

QString HeroCard::getSubtype() const{
    return "hero_card";
}

QString HeroCard::getPixmapPath() const{
    QString path = QString("image/generals/card/%1.jpg").arg(objectName());
    return QFile::exists(path) ? path : "image/card/unknown.jpg";
}

QString HeroCard::getDescription() const{
    const General *general = Sanguosha->getGeneral(objectName());
    if(general)
        return general->getSkillDescription();

    return tr("<b>[%1]</b> %2").arg(getName()).arg(":hero");
}

//-----------Skills Part

PrepareCard::PrepareCard(){
    target_fixed = true;
    once = true;
    owner_discarded = true;
}

void PrepareCard::onUse(Room *room, const CardUseStruct &card_use) const{
    const Card *card = Sanguosha->getCard(this->getSubcards().first());

    LogMessage log;
    log.from = card_use.from;
    log.type = "#EquipedHeroCard";
    log.arg = card->objectName();
    room->sendLog(log);
    room->broadcastInvoke("playAudio", "equiphero");

    card_use.from->addToPile("heros", card->getEffectiveId());
    room->setPlayerMark(card_use.from, "hero", card->getEffectiveId());
    room->transfigure(card_use.from, card->objectName(), false);
    room->setPlayerProperty(card_use.from, "kingdom", card_use.from->getGeneral()->getKingdom());
    card_use.from->fillHero();
}

class Prepare:public OneCardViewAsSkill{
public:
    Prepare():OneCardViewAsSkill("prepare"){
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->inherits("HeroCard");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new PrepareCard;
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

RecastHeroCard::RecastHeroCard(){
    target_fixed = true;
    owner_discarded = true;
    mute = true;
}

void RecastHeroCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *source = card_use.from;

    QList<int> heros = source->getPile("heros");
    QList<int> canRecast;
    foreach(int hero, heros)
    {
        if(Sanguosha->getCard(hero)->hasFlag("justdraw")){
            canRecast << hero;
        }
    }

    while(!canRecast.isEmpty())
    {
        room->fillAG(canRecast, source);
        int card_id = room->askForAG(source, canRecast, true, "herorecast");
        source->invoke("clearAG");
        if (card_id == -1)
            return;
        canRecast.removeOne(card_id);
        room->setCardFlag(card_id, "-justdraw");
        source->playCardEffect("@recast");
        room->throwCard(card_id,source);
        room->drawCards(source,1);
    }
}

class HeroRecast:public ZeroCardViewAsSkill{
public:
    HeroRecast():ZeroCardViewAsSkill("herorecast"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        bool canInvoke = false;
        QList<int> heros = player->getPile("heros");
        foreach(int hero, heros)
        {
            if(Sanguosha->getCard(hero)->hasFlag("justdraw")){
                canInvoke = true;
                break;
            }
        }

        return canInvoke;
    }

    virtual const Card *viewAs() const{
        return new RecastHeroCard;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

UseHeroCard::UseHeroCard(){
    target_fixed = true;
    once = true;
    owner_discarded = true;
}

void UseHeroCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *source = card_use.from;
    QList<int> heros = source->getPile("heros");

    int equiped = source->getMark("hero");
    heros.removeOne(equiped);

    room->fillAG(heros, source);
    int hero = room->askForAG(source, heros, false, "usehero");
    room->setCardFlag(hero, "-justdraw");
    source->invoke("clearAG");

    const Card *herocard = Sanguosha->getCard(hero);

    if(equiped >0){
        room->setCardFlag(equiped, "-justdraw");
        room->throwCard(equiped, source);
    }

    LogMessage log;
    log.from = source;
    log.type = "#EquipedHeroCard";
    log.arg = herocard->objectName();
    room->sendLog(log);
    room->broadcastInvoke("playAudio", "equiphero");

    room->setPlayerMark(source, "hero", herocard->getEffectiveId());
    room->transfigure(source, herocard->objectName(), false);
    room->setPlayerProperty(source, "kingdom", source->getGeneral()->getKingdom());
    source->fillHero();
}

class UseHero:public ZeroCardViewAsSkill{
public:
    UseHero():ZeroCardViewAsSkill("usehero"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        bool canInvoke = (player->getPile("heros").length() > 1 && player->getMark("hero") > 0)
                || player->getMark("hero") == 0;
        return !player->getPile("heros").isEmpty() && !player->hasUsed("UseHeroCard") && canInvoke;
    }

    virtual const Card *viewAs() const{
        return new UseHeroCard;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

ThreeKingdomsPackage::ThreeKingdomsPackage():Package("ThreeKingdoms")
{
    type = Package::CardPack;

    QStringList PackageNames;
    PackageNames << "Standard"
                 << "Wind"
                 << "Fire"
                 << "Thicket"
                 << "Mountain"
                 << "YJCM"
                 << "YJCM2012"
                 << "BGM";
    QList<const General*> generals;
    foreach(QString package, PackageNames)
    {
        Package *apackage = PackageAdder::packages()[package];
        generals << apackage->findChildren<const General *>();
    }

    QSet<QString> all, ban_set;

    foreach(const General *general, generals){
        all << general->objectName();
    }
    all << "yuanshu" << "yangxiu" << "gongsunzan";

    ban_set = Config.value("Banlist/ThreeKingdoms").toStringList().toSet();
    if(ban_set.isEmpty())
        ban_set << "yuji" << "zuoci" << "zuocif" << "bgm_pangtong";
    all.subtract(ban_set);

    QList<Card*> cards;
    foreach(const QString name, all.toList())
    {
        cards << new HeroCard(name,  Card::NoSuit, 0);
    }

    cards << new Dismantlement(Card::Spade, 1)
          << new Dismantlement(Card::Club, 13)
          << new Dismantlement(Card::Club, 1)
          << new Dismantlement(Card::Diamond, 13);

    foreach(Card *card, cards)
        card->setParent(this);

    skills << new Prepare << new HeroRecast << new UseHero;

    addMetaObject<PrepareCard>();
    addMetaObject<RecastHeroCard>();
    addMetaObject<UseHeroCard>();
}

ADD_PACKAGE(ThreeKingdoms)

