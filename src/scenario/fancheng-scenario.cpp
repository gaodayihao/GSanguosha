#include "fancheng-scenario.h"
#include "scenario.h"
#include "skill.h"
#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "carditem.h"
#include "standard.h"

class Guagu: public TriggerSkill{
public:
    Guagu():TriggerSkill("guagu"){
        events << Damage;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.to->isLord()){
            int x = damage.damage;

            RecoverStruct recover;
            recover.card = damage.card;
            recover.who = damage.from;
            recover.recover = x*2;
            room->recover(damage.to, recover);
            player->drawCards(x);
        }

        return false;
    }
};

DujiangCard::DujiangCard(){
    target_fixed = true;
}

void DujiangCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    room->throwCard(this, NULL);

    LogMessage log;
    log.type = "#InvokeSkill";
    log.from = source;
    log.arg = "dujiang";
    room->sendLog(log);

    room->transfigure(source, "shenlvmeng", false);

    room->setTag("Dujiang", true);
}

class DujiangViewAsSkill: public ViewAsSkill{
public:
    DujiangViewAsSkill():ViewAsSkill("dujiang"){
        frequency = Limited;
    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@dujiang-card";
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 2 && to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 2)
            return false;

        DujiangCard *card = new DujiangCard;
        card->addSubcards(cards);

        return card;
    }
};

class Dujiang: public PhaseChangeSkill{
public:
    Dujiang():PhaseChangeSkill("dujiang"){
        view_as_skill = new DujiangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        if(!PhaseChangeSkill::triggerable(target))
            return false;

        return target->getGeneralName() != "shenlvmeng";
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if(target->getPhase() == Player::Start){
            if(target->getEquips().length() < 2)
                return false;

            Room *room = target->getRoom();
            room->askForUseCard(target, "@dujiang-card", "@@dujiang");
        }

        return false;
    }
};

FloodCard::FloodCard(){
    target_fixed = true;
}

void FloodCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    room->throwCard(this, NULL);
    room->setTag("Flood", true);

    room->setPlayerFlag(source, "flood");

    QList<ServerPlayer *> players = room->getOtherPlayers(source);
    foreach(ServerPlayer *player, players){
        if(player->getRoleEnum() == Player::Rebel){
            room->cardEffect(this, source, player);
        }
    }
}

void FloodCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->throwAllEquips();

    Room *room = effect.to->getRoom();
    if(!room->askForDiscard(effect.to, "flood", 2, 2, true)){
        DamageStruct damage;
        damage.from = effect.from;
        damage.to = effect.to;

        room->damage(damage);
    }
}

class Flood: public ViewAsSkill{
public:
    Flood():ViewAsSkill("flood"){
        frequency = Limited;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasFlag("flood");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 3 && !to_select->isEquipped() && to_select->getCard()->isBlack();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 3)
            return NULL;

        FloodCard *card = new FloodCard;
        card->addSubcards(cards);

        return card;
    }
};

TaichenFightCard::TaichenFightCard(){
    target_fixed = true;
    once = true;
}

void TaichenFightCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    room->loseHp(source);

    if(source->isAlive()){
        Duel *duel = new Duel(Card::NoSuit, 0);
        duel->setCancelable(false);

        CardEffectStruct effect;
        effect.card = duel;
        effect.from = source;
        effect.to = room->getLord();

        room->acquireSkill(source, "wushuang", false);
        room->cardEffect(effect);
        source->loseSkill("wushuang");
    }
}

class TaichenFight: public ZeroCardViewAsSkill{
public:
    TaichenFight():ZeroCardViewAsSkill("taichen_fight"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("TaichenFightCard");
    }

    virtual const Card *viewAs() const{
        return new TaichenFightCard;
    }
};

class Xiansheng: public PhaseChangeSkill{
public:
    Xiansheng():PhaseChangeSkill("xiansheng"){
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
                && target->getGeneralName() == "guanyu" && target->getHp() <= 2;
    }

    virtual bool onPhaseChange(ServerPlayer *guanyu) const{
        if(guanyu->getPhase() == Player::Start){
            Room *room = guanyu->getRoom();

            if(guanyu->askForSkillInvoke("xiansheng")){
                guanyu->throwAllEquips();
                guanyu->throwAllHandCards();

                room->transfigure(guanyu, "shenguanyu", true);

                room->drawCards(guanyu, 3);
            }
        }

        return false;
    }
};

ZhiyuanCard::ZhiyuanCard(){
    will_throw = false;
}

bool ZhiyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && to_select->getRoleEnum() == Player::Rebel;
}

void ZhiyuanCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    targets.first()->obtainCard(this);
    room->setPlayerMark(source, "zhiyuan", source->getMark("zhiyuan") - 1);
}

class ZhiyuanViewAsSkill: public OneCardViewAsSkill{
public:
    ZhiyuanViewAsSkill():OneCardViewAsSkill(""){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->getMark("zhiyuan") > 0;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->inherits("BasicCard");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        ZhiyuanCard *card = new ZhiyuanCard;
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class Zhiyuan: public PhaseChangeSkill{
public:
    Zhiyuan():PhaseChangeSkill("zhiyuan"){
        view_as_skill = new ZhiyuanViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if(target->getPhase() == Player::Start){
            Room *room = target->getRoom();
            room->setPlayerMark(target, "zhiyuan", 2);
        }

        return false;
    }
};

class CaorenMaxCards: public MaxCardsSkill{
public:
    CaorenMaxCards():MaxCardsSkill("#caorenmaxcards"){
    }

    virtual int getExtra(const Player *target) const{
        if(!target->hasSkill(objectName()))
            return 0;
        return -target->getMark("flood");
    }
};

class FanchengRule: public ScenarioRule{
public:
    FanchengRule(Scenario *scenario)
        :ScenarioRule(scenario)
    {
        events << GameStart << Death;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        switch(event){
        case GameStart:{
            player = room->getLord();
            room->installEquip(player, "chitu");
            room->installEquip(player, "blade");
            room->acquireSkill(player, "flood", true, false);
            room->acquireSkill(player, "xiansheng", true, false);

            ServerPlayer *panglingming = room->findPlayer("sp_pangde");
            room->acquireSkill(panglingming, "taichen_fight", true, false);

            ServerPlayer *huatuo = room->findPlayer("huatuo");
            room->installEquip(huatuo, "hualiu");
            room->acquireSkill(huatuo, "guagu", true, false);

            ServerPlayer *lvmeng = room->findPlayer("lvmeng");
            room->acquireSkill(lvmeng, "dujiang", true, false);

            ServerPlayer *caoren = room->findPlayer("caoren");
            room->installEquip(caoren, "renwang_shield");
            room->acquireSkill(caoren, "zhiyuan", true, false);
            room->acquireSkill(caoren, "#caorenmaxcards");


            break;
        }

        case Death:{
            DamageStar damage = data.value<DamageStar>();
            if(player->getGeneralName() == "sp_pangde" &&
                    damage && damage->from && damage->from->isLord())
            {
                damage = NULL;
                data = QVariant::fromValue(damage);
            }

            break;
        }

        default:
            break;
        }

        return false;
    }
};

FanchengScenario::FanchengScenario()
    :Scenario("fancheng")
{
    lord = "guanyu";
    loyalists << "huatuo";
    rebels << "caoren" << "sp_pangde" << "xuhuang";
    renegades << "lvmeng";

    rule = new FanchengRule(this);

    skills << new Guagu
           << new Dujiang
           << new Flood
           << new TaichenFight
           << new Xiansheng
           << new Zhiyuan
           << new CaorenMaxCards;

    addMetaObject<DujiangCard>();
    addMetaObject<FloodCard>();
    addMetaObject<TaichenFightCard>();
    addMetaObject<ZhiyuanCard>();
}

void FanchengScenario::onTagSet(Room *room, const QString &key) const{
    if(key == "Flood"){
        ServerPlayer *xuhuang = room->findPlayer("xuhuang");
        if(xuhuang){
            ServerPlayer *lord = room->getLord();
            room->setFixedDistance(xuhuang, lord, 1);
        }

        ServerPlayer *caoren = room->findPlayer("caoren");
        if(caoren)
            room->setPlayerMark(caoren, "flood", 1);
    }else if(key == "Dujiang"){
        ServerPlayer *caoren = room->findPlayer("caoren");
        if(caoren)
            room->setPlayerMark(caoren, "flood", 0);
    }
}

ADD_SCENARIO(Fancheng)
