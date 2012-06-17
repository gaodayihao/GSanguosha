#include "special3v3-package.h"
#include "skill.h"
#include "standard.h"
#include "server.h"
#include "carditem.h"
#include "engine.h"
#include "ai.h"
#include "maneuvering.h"

HongyuanCard::HongyuanCard(){

}

bool HongyuanCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select == Self)
        return false;
    return targets.length() < 2;
}

void HongyuanCard::onEffect(const CardEffectStruct &effect) const{
   effect.to->drawCards(1);
}

void HongyuanCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    source->drawCards(1);
    SkillCard::use(room, source, targets);
}

class HongyuanViewAsSkill: public ZeroCardViewAsSkill{
public:
    HongyuanViewAsSkill():ZeroCardViewAsSkill("hongyuan"){
    }

    virtual const Card *viewAs() const{
        return new HongyuanCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@hongyuan";
    }
};

class Hongyuan:public TriggerSkill{
public:
    Hongyuan():TriggerSkill("hongyuan"){
        events << PhaseChange;
        frequency = NotFrequent;
        view_as_skill = new HongyuanViewAsSkill;
    }


    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *zhugejin, QVariant &data) const{
        if(zhugejin->getPhase() == Player::Draw && room->askForSkillInvoke(zhugejin, objectName())){
            room->playSkillEffect(objectName());
            if(ServerInfo.GameMode == "06_3v3"){
                zhugejin->drawCards(1);
                foreach(ServerPlayer *other, room->getOtherPlayers(zhugejin)){
                    if(AI::GetRelation3v3(zhugejin, other) == AI::Friend)
                        other->drawCards(1);
                }

            }else{
                if(!room->askForUseCard(zhugejin, "@@hongyuan!", "@hongyuan"))
                   zhugejin->drawCards(1);
            }
            return true;
        }
        return false;
    }
};

HuanshiCard::HuanshiCard(){
    target_fixed = true;
    will_throw = false;
    can_jilei = true;
}

void HuanshiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{

}

class HuanshiViewAsSkill:public OneCardViewAsSkill{
public:
    HuanshiViewAsSkill():OneCardViewAsSkill("huanshi"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@huanshi";
    }

    virtual bool viewFilter(const CardItem *) const{
        return true;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new HuanshiCard;
        card->setSuit(card_item->getFilteredCard()->getSuit());
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class Huanshi: public TriggerSkill{
public:
    Huanshi():TriggerSkill("huanshi"){
        view_as_skill = new HuanshiViewAsSkill;

        events << AskForRetrial;
    }

    QList<ServerPlayer *> getTeammates(ServerPlayer *zhugejin) const{
        Room *room = zhugejin->getRoom();

        QList<ServerPlayer *> teammates;
        foreach(ServerPlayer *other, room->getOtherPlayers(zhugejin)){
            if(AI::GetRelation3v3(zhugejin, other) == AI::Friend)
                teammates << other;
        }
        return teammates;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->isKongcheng();
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();

        bool can_invoke = false;
        if(ServerInfo.GameMode == "06_3v3"){
            foreach(ServerPlayer *teammate, getTeammates(player)){
                if(teammate->objectName() == judge->who->objectName()){
                    can_invoke = true;
                    break;
                }
            }
        }else if(judge->who->objectName() != player->objectName()){
            if(room->askForSkillInvoke(player,objectName()))
                if(room->askForChoice(judge->who, objectName(), "yes+no") == "yes")
                    can_invoke = true;
        }
        else
            can_invoke = true;
        if(!can_invoke)
            return false;

        QStringList prompt_list;
        prompt_list << "@huanshi-card" << judge->who->objectName()
                << objectName() << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");

        player->tag["Judge"] = data;
        const Card *card = room->askForCard(player, "@huanshi", prompt, data);

        if(card){
            // the only difference for Guicai & Guidao
            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, QString());
            // remove below after TopDrawPile works
            if(room->getCardPlace(judge->card->getEffectiveId()) != Player::DiscardPile
                    || room->getCardPlace(judge->card->getEffectiveId()) != Player::Hand)
                room->throwCard(judge->card, reason, judge->who);

            judge->card = Sanguosha->getCard(card->getEffectiveId());
            /* revive this when TopDrawPile is OK
            room->moveCardTo(judge->card, player, NULL, Player::TopDrawPile,
                CardMoveReason(CardMoveReason::S_REASON_RETRIAL, player->objectName(), "huanshi", QString()), true); */
            room->moveCardTo(judge->card, player, judge->who, Player::Special,
                             CardMoveReason(CardMoveReason::S_REASON_JUDGEDONE, player->objectName(), "huanshi", QString()), true);
            LogMessage log;
            log.type = "$ChangedJudge";
            log.from = player;
            log.to << judge->who;
            log.card_str = card->getEffectIdString();
            room->sendLog(log);

            room->sendJudgeResult(judge);
        }

        return false;
    }
};

class Mingzhe: public TriggerSkill{
public:
    Mingzhe():TriggerSkill("mingzhe"){
        events << CardUsed << CardResponsed << CardDiscarded;
        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::NotActive)
            return false;

        const Card *card = NULL;
        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }
        else
            card = data.value<CardStar>();

        int n = 0;
        if(event == CardDiscarded){
            if(card->isVirtualCard()){
                foreach(int card_id, card->getSubcards()){
                    const Card *subcard = Sanguosha->getCard(card_id);
                    if(subcard->isRed())
                        n++;
                }
            }
            else if(card->isRed())
                n++;
        }
        else
            n = card->isRed() ? 1 : 0;

        if(n>0 && player->askForSkillInvoke(objectName(), data))
            player->drawCards(n);

        return false;
    }
};

New3v3CardPackage::New3v3CardPackage()
    :Package("New3v3Card")
{
    QList<Card *> cards;
    cards << new SupplyShortage(Card::Spade, 1)
          << new SupplyShortage(Card::Club, 12)
          << new Nullification(Card::Heart, 12);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(New3v3Card)

Special3v3Package::Special3v3Package():Package("Special3v3")
{
    General *zhugejin = new General(this, "zhugejin", "wu", 3, true);
    zhugejin->addSkill(new Huanshi);
    zhugejin->addSkill(new Hongyuan);
    zhugejin->addSkill(new Mingzhe);

    addMetaObject<HuanshiCard>();
    addMetaObject<HongyuanCard>();
}

ADD_PACKAGE(Special3v3)
