#include "standard.h"
#include "skill.h"
#include "wind.h"
#include "client.h"
#include "carditem.h"
#include "engine.h"
#include "nostalgia.h"

class MoonSpearSkill: public WeaponSkill{
public:
    MoonSpearSkill():WeaponSkill("moon_spear"){
        events << CardFinished << CardResponsed;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(player->getPhase() != Player::NotActive)
            return false;

        CardStar card = NULL;
        if(event == CardFinished){
            CardUseStruct card_use = data.value<CardUseStruct>();
            card = card_use.card;

            if(card == player->tag["MoonSpearSlash"].value<CardStar>()){
                card = NULL;
            }
        }else if(event == CardResponsed){
            card = data.value<CardStar>();
            player->tag["MoonSpearSlash"] = data;
        }

        if(card == NULL || !card->isBlack())
            return false;

        Room *room = player->getRoom();

        room->setEmotion(player, QString("weapon/%1").arg("sp_moonspear"));

        room->askForUseCard(player, "slash", "@moon-spear-slash");

        return false;
    }
};

class MoonSpear: public Weapon{
public:
    MoonSpear(Suit suit = Card::Diamond, int number = 12)
        :Weapon(suit, number, 3){
        setObjectName("moon_spear");
        skill = new MoonSpearSkill;
    }
};

NostalgiaPackage::NostalgiaPackage()
    :Package("nostalgia")
{
    type = CardPack;

    Card *moon_spear = new MoonSpear;
    moon_spear->setParent(this);
}

NosFanjianCard::NosFanjianCard(){
    once = true;
    mute = true;
}

void NosFanjianCard::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *zhouyu = effect.from;
    ServerPlayer *target = effect.to;
    Room *room = zhouyu->getRoom();

    room->playSkillEffect("fanjian");

    int card_id = zhouyu->getRandomHandCardId();
    const Card *card = Sanguosha->getCard(card_id);
    Card::Suit suit = room->askForSuit(target, "nos_fanjian");

    LogMessage log;
    log.type = "#ChooseSuit";
    log.from = target;
    log.arg = Card::Suit2String(suit);
    room->sendLog(log);

    room->showCard(zhouyu, card_id);
    room->getThread()->delay();

    if(card->getSuit() != suit){
        DamageStruct damage;
        damage.card = NULL;
        damage.from = zhouyu;
        damage.to = target;

        room->damage(damage);
    }

    target->obtainCard(card);
}

class NosFanjian:public ZeroCardViewAsSkill{
public:
    NosFanjian():ZeroCardViewAsSkill("nos_fanjian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && ! player->hasUsed("NosFanjianCard");
    }

    virtual const Card *viewAs() const{
        return new NosFanjianCard;
    }
};

class NosLieren: public TriggerSkill{
public:
    NosLieren():TriggerSkill("nos_lieren"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *zhurong, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if(damage.card && damage.card->inherits("Slash") && !zhurong->isKongcheng()
            && !target->isKongcheng() && target != zhurong){
            Room *room = zhurong->getRoom();
            if(room->askForSkillInvoke(zhurong, objectName(), data)){
                room->playSkillEffect("lieren", 1);

                bool success = zhurong->pindian(target, "lieren", NULL);
                if(success)
                    room->playSkillEffect("lieren", 2);
                else{
                    room->playSkillEffect("lieren", 3);
                    return false;
                }

                if(!target->isNude()){
                    int card_id = room->askForCardChosen(zhurong, target, "he", objectName());
                    if(room->getCardPlace(card_id) == Player::Hand)
                        room->moveCardTo(Sanguosha->getCard(card_id), zhurong, Player::Hand, false);
                    else
                        room->obtainCard(zhurong, card_id);
                }
            }
        }

        return false;
    }
};

NostalGeneralPackage::NostalGeneralPackage()
    :Package("nostal_general")
{
    General *nos_zhouyu = new General(this, "nos_zhouyu", "wu", 3);
    nos_zhouyu->addSkill(new NosFanjian);
    nos_zhouyu->addSkill("yingzi");

    General *nos_zhurong = new General(this, "nos_zhurong", "shu", 4, false);
    nos_zhurong->addSkill("juxiang");
    nos_zhurong->addSkill(new NosLieren);

    addMetaObject<NosFanjianCard>();
}

ADD_PACKAGE(NostalGeneral)
ADD_PACKAGE(Nostalgia)
