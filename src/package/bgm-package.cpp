#include "bgm-package.h"
#include "skill.h"
#include "standard.h"
#include "clientplayer.h"
#include "carditem.h"
#include "engine.h"
#include "client.h"

class ChongZhen: public TriggerSkill{
public:
    ChongZhen(): TriggerSkill("chongzhen"){
        events << CardResponsed << SlashEffect << CardEffect << CardEffected << CardFinished;
    }

    virtual int getPriority() const{
        return 3;
    }

    void doChongZhen(ServerPlayer *player, const Card *card) const{
        if(card->getSkillName() != "longdan")
            return;

        Room *room = player->getRoom();

        ServerPlayer *target = player->tag["ChongZhenTarget"].value<PlayerStar>();
        if(!target || target->isKongcheng() || !room->askForSkillInvoke(player, objectName()))
            return;

        int card_id = room->askForCardChosen(player, target, "h", objectName());

        room->playSkillEffect(objectName());
        room->obtainCard(player, card_id, false);
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == CardFinished){
            player->tag["ChongZhenTarget"] = QVariant::fromValue(NULL);
        }
        else if(event == CardResponsed){
            CardStar card = data.value<CardStar>();
            doChongZhen(player, card);
        }
        else if(event == SlashEffect){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            player->tag["ChongZhenTarget"] = QVariant::fromValue(effect.to);
            doChongZhen(player, effect.slash);
        }
        else{
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(effect.card->inherits("Duel")
                    || effect.card->inherits("ArcheryAttack")
                    || effect.card->inherits("SavageAssault")
                    || effect.card->inherits("Slash"))
                player->tag["ChongZhenTarget"] = QVariant::fromValue(event == CardEffect ? effect.to : effect.from);
        }

        return false;
    }
};

LihunCard::LihunCard(){
    owner_discarded = true;
}

bool LihunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!to_select->getGeneral()->isMale())
        return false;

    if(!targets.isEmpty())
        return false;

    return true;
}

void LihunCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    effect.from->turnOver();
    effect.to->setFlags("LihunTarget");
    DummyCard *cd = effect.to->wholeHandCards();
    if(cd)
        room->obtainCard(effect.from, cd, false);
    cd->deleteLater();
}

class LihunSelect: public OneCardViewAsSkill{
public:
    LihunSelect():OneCardViewAsSkill("lihun"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return true;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("LihunCard");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new LihunCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class Lihun: public TriggerSkill{
public:
    Lihun():TriggerSkill("lihun"){
        events << PhaseChange;
        view_as_skill = new LihunSelect;
    }

    virtual int getPriority() const{
        return 4;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasUsed("LihunCard");
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *diaochan, QVariant &data) const{
        Room *room = diaochan->getRoom();
        PhaseChangeStruct phase_change = data.value<PhaseChangeStruct>();

        if(phase_change.from == Player::Play){
            ServerPlayer *target = NULL;
            foreach(ServerPlayer *other, room->getOtherPlayers(diaochan)){
                if(other->hasFlag("LihunTarget")){
                    other->setFlags("-LihunTarget");
                    target = other;
                    break;
                }
            }

            if(!target || target->getHp() < 1 || diaochan->isNude())
                return false;

            DummyCard *to_goback;
            if(diaochan->getCardCount(true) <= target->getHp())
            {
                to_goback = diaochan->wholeHandCards();
                if (!to_goback)
                    to_goback = new DummyCard;
                for (int i = 0;i < 4; i++)
                    if(diaochan->getEquip(i))
                        to_goback->addSubcard(diaochan->getEquip(i)->getEffectiveId());
            }
            else
                to_goback = (DummyCard*)room->askForExchange(diaochan, "lihun", target->getHp(), true, "LihunGoBack");

            room->moveCardTo(to_goback, target, Player::Hand);
            delete to_goback;
        }

        return false;
    }
};

class Kuiwei: public TriggerSkill{
public:
    Kuiwei(): TriggerSkill("kuiwei"){
        events << PhaseChange;
    }

    virtual int getPriority() const{
        return 3;
    }

    int getWeaponCount(ServerPlayer *caoren) const{
        int n = 0;
        foreach(ServerPlayer *p, caoren->getRoom()->getAlivePlayers()){
            if(p->getWeapon())
                n ++;
        }

        return n;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *caoren, QVariant &) const{
        Room *room = caoren->getRoom();

        if(caoren->getPhase() == Player::Finish){
            if(!caoren->askForSkillInvoke(objectName()))
                return false;

            int n = getWeaponCount(caoren);
            room->playSkillEffect(objectName(), 1);
            caoren->drawCards(n+2);
            caoren->turnOver();

            if(caoren->getMark("@kuiwei") == 0)
                caoren->gainMark("@kuiwei");
        }
        else if(caoren->getPhase() == Player::Draw){
            if(caoren->getMark("@kuiwei") == 0)
                return false;

            int n = getWeaponCount(caoren);
            if(n > 0){
                LogMessage log;
                log.type = "#KuiweiDiscard";
                log.from = caoren;
                log.arg = QString::number(n);
                log.arg2 = objectName();
                room->sendLog(log);

                room->playSkillEffect(objectName(), 2);
                if(caoren->getCards("he").length() <= n){
                    caoren->throwAllEquips();
                    caoren->throwAllHandCards();
                }
                else{
                    room->askForDiscard(caoren, objectName(), n, n, false, true);
                }
            }

            caoren->loseMark("@kuiwei");
        }
        return false;
    }
};

class Yanzheng: public OneCardViewAsSkill{
public:
    Yanzheng():OneCardViewAsSkill("yanzheng"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification" && player->getHandcardNum() > player->getHp();
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getFilteredCard();
        Card *ncard = new Nullification(first->getSuit(), first->getNumber());
        ncard->addSubcard(first);
        ncard->setSkillName(objectName());

        return ncard;
    }
};

class Manjuan: public TriggerSkill{
public:
    Manjuan(): TriggerSkill("manjuan"){
        events << CardGotOnePiece << CardDrawing;
        frequency = Frequent;
    }

    virtual int getPriority() const{
            return 2;
    }

    void doManjuan(ServerPlayer *sp_pangtong, int card_id) const{
        Room *room = sp_pangtong->getRoom();
        sp_pangtong->setFlags("ManjuanInvoke");
        QList<int> DiscardPile = room->getDiscardPile(), toGainList;
        const Card *card = Sanguosha->getCard(card_id);
        foreach(int id, DiscardPile){
            const Card *cd = Sanguosha->getCard(id);
            if(cd->getNumber() == card->getNumber())
                toGainList << id;
        }

        room->fillAG(toGainList, sp_pangtong);
        int id = room->askForAG(sp_pangtong, toGainList, false, objectName());
        if(id != -1)
            room->moveCardTo(Sanguosha->getCard(id), sp_pangtong, Player::Hand, true);

        sp_pangtong->invoke("clearAG");
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *sp_pangtong, QVariant &data) const{
        Room *room = sp_pangtong->getRoom();

        if(sp_pangtong->hasFlag("ManjuanInvoke")){
            sp_pangtong->setFlags("-ManjuanInvoke");
            return false;
        }

        int card_id = -1;
        if(event == CardGotOnePiece){
            CardMoveStar move = data.value<CardMoveStar>();
            card_id = move->card_id;
            if(move->to_place == Player::Hand){
                room->throwCard(move->card_id);
                LogMessage log;
                log.type = "$ManjuanGot";
                log.from = sp_pangtong;
                log.card_str = Sanguosha->getCard(move->card_id)->toString();
                room->sendLog(log);
            }else
                return false;
        }
        else if(event == CardDrawing){
            if(sp_pangtong->tag.value("FirstDraw", false).toBool())
                return false;

            card_id = data.toInt();
            room->throwCard(card_id);
        }

        if(sp_pangtong->getPhase() == Player::NotActive || !sp_pangtong->askForSkillInvoke(objectName(), data))
            return event == CardGotOnePiece ? false : true;

        room->playSkillEffect(objectName());
        doManjuan(sp_pangtong, card_id);
        return event == CardGotOnePiece ? false : true;
    }
};

class Zuixiang: public TriggerSkill{
public:
    Zuixiang(): TriggerSkill("zuixiang"){
        events << PhaseChange << CardEffected ;
        frequency = Limited;

        type[Card::Basic] = "BasicCard";
        type[Card::Trick] = "TrickCard";
        type[Card::Equip] = "EquipCard";
    }

    void doZuixiang(ServerPlayer *player) const{
        Room *room = player->getRoom();

        QList<int> ids = room->getNCards(3);
        player->addToPile("dream", ids, true);
        QSet<QString> lockedCategories;
        foreach(int id, ids){
            const Card *cd = Sanguosha->getCard(id);
            lockedCategories.insert(type[cd->getTypeId()]);
        }
        foreach (QString s, lockedCategories)
            room->setPlayerCardLock(player, s);
        room->getThread()->delay();

        QList<int> zuixiang = player->getPile("dream");
        QSet<int> numbers;
        bool zuixiangDone = false;
        foreach(int id, zuixiang){
            const Card *card = Sanguosha->getCard(id);
            if(numbers.contains(card->getNumber())){
                zuixiangDone = true;
                break;
            }
            numbers.insert(card->getNumber());
        }
        if (zuixiangDone)
        {
            player->addMark("zuixiangHasTrigger");
            room->setPlayerCardLock(player, ".");
            CardsMoveStruct move(zuixiang, player, Player::Hand);
            room->moveCards(move, true);
        }
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *sp_pangtong, QVariant &data) const{
        Room *room = sp_pangtong->getRoom();
        QList<int> zuixiang = sp_pangtong->getPile("dream");

        if(event == PhaseChange && sp_pangtong->getMark("zuixiangHasTrigger") == 0){
            if(sp_pangtong->getPhase() == Player::Start){
                if(sp_pangtong->getMark("@sleep") == 1){
                    if(!sp_pangtong->askForSkillInvoke(objectName()))
                        return false;
                    room->playSkillEffect(objectName());
                    sp_pangtong->loseMark("@sleep", 1);
                    doZuixiang(sp_pangtong);
                }else
                    doZuixiang(sp_pangtong);
            }
        }
        else if(event == CardEffected){
            if(zuixiang.isEmpty())
                return false;

            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(sp_pangtong->hasCardLock(type[effect.card->getTypeId()])){
                LogMessage log;
                log.type = effect.from ? "#ZuiXiang1" : "#ZuiXiang2";
                log.from = effect.to;
                if(effect.from)
                    log.to << effect.from;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();

                room->sendLog(log);

                room->playSkillEffect(objectName());
                return true;
            }
        }
        return false;
    }

private:
    QMap<Card::CardType, QString> type;
};

class Jie: public TriggerSkill{
public:
    Jie():TriggerSkill("jie"){
        events << Predamage;
        frequency = Compulsory;
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *zhangfei, QVariant &data) const{
        Room *room = zhangfei->getRoom();
        DamageStruct damage = data.value<DamageStruct>();
        const Card *reason = damage.card;

        if(reason && reason->inherits("Slash") && reason->isRed()){
            room->playSkillEffect(objectName());
            LogMessage log;
            log.type = "#Jie";
            log.from = zhangfei;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(++damage.damage);
            room->sendLog(log);
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

DaheCard::DaheCard(){
    once = true;
    mute = true;
    will_throw = false;
}

bool DaheCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return to_select != Self && targets.isEmpty() && !to_select->isKongcheng();
}

void DaheCard::use(Room *room, ServerPlayer *bgm_zhangfei, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();
    room->playSkillEffect("dahe", 1);
    bgm_zhangfei->pindian(target, "dahe", this);
}

class DaheViewAsSkill: public OneCardViewAsSkill{
public:
    DaheViewAsSkill():OneCardViewAsSkill("dahe"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("DaheCard") && !player->isKongcheng();
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        DaheCard *card = new DaheCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class Dahe: public TriggerSkill{
public:
    Dahe():TriggerSkill("dahe"){
        events << PindianFinished << PhaseChange;
        view_as_skill = new DaheViewAsSkill;
        default_choice = "yes";
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *zhangfei, QVariant &data) const{
        Room *room = zhangfei->getRoom();
        if(event == PindianFinished){
            PindianStar pindian = data.value<PindianStar>();
            if(pindian->reason != objectName())
                return false;

            if(pindian->isSuccess()){
                room->playSkillEffect(objectName(), 2);
                QList<ServerPlayer *> targets = room->getAlivePlayers();
                foreach(ServerPlayer *p, targets){
                    if(p->getHp() > pindian->from->getHp())
                        targets.removeOne(p);
                }
                QString choice = room->askForChoice(zhangfei, objectName(), "yes+no");
                if(choice == "yes"){
                    ServerPlayer *target = room->askForPlayerChosen(zhangfei, targets, objectName());
                    target->obtainCard(pindian->to_card);
                }
                pindian->to->setFlags(objectName());
            }
            else{
                room->playSkillEffect(objectName(), 3);
                if(!pindian->from->isKongcheng()){
                    room->showAllCards(pindian->from);
                    room->askForDiscard(pindian->from, objectName(), 1, 1);
                }
            }
        }
        else{
            if(zhangfei->getPhase() != Player::NotActive)
                return false;
            foreach(ServerPlayer *sp, room->getAlivePlayers())
                if(sp->hasFlag(objectName()))
                    room->setPlayerFlag(sp, "-" + objectName());
        }

        return false;
    }
};

TanhuCard::TanhuCard(){
    once = true;
    will_throw = false;
}

bool TanhuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && !to_select->isKongcheng() && to_select != Self;
}

void TanhuCard::use(Room *room, ServerPlayer *lvmeng, const QList<ServerPlayer *> &targets) const{
    bool success = lvmeng->pindian(targets.first(), "tanhu", this);
    if(success){
        room->setPlayerFlag(targets.first(), "TanhuTarget");
        room->setFixedDistance(lvmeng, targets.first(), 1);
    }
}

class TanhuViewAsSkill: public OneCardViewAsSkill{
public:
    TanhuViewAsSkill():OneCardViewAsSkill("tanhu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("TanhuCard") && !player->isKongcheng();
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new TanhuCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class Tanhu: public PhaseChangeSkill{
public:
    Tanhu():PhaseChangeSkill("tanhu"){
        view_as_skill = new TanhuViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        if(target->getPhase() == Player::Finish){
            Room *room = target->getRoom();
            QList<ServerPlayer *> players = room->getAlivePlayers();

            foreach(ServerPlayer *player, players){
                if(player->hasFlag("TanhuTarget"))
                {
                    room->setPlayerFlag(player, "-TanhuTarget");
                    room->setFixedDistance(target, player, -1);
                }
            }
        }

        return false;
    }
};

class MouduanStart: public GameStartSkill{
public:
    MouduanStart():GameStartSkill("#mouduan"){

    }

    virtual int getPriority() const{
        return -1;
    }

    virtual void onGameStart(ServerPlayer *lvmeng) const{
        Room *room = lvmeng->getRoom();
        room->setPlayerMark(lvmeng, "@wu", 1);
        room->acquireSkill(lvmeng, "jiang", true);
        room->getThread()->delay();
        room->acquireSkill(lvmeng, "qianxun", true);
    }
};

class Mouduan: public TriggerSkill{
public:
    Mouduan():TriggerSkill("mouduan"){
        events << TurnStart << CardLostOneTime;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *>lvmengs = room->findPlayersBySkillName(objectName());

        if(event == CardLostOneTime){
            if((player->getMark("@wu") > 0) && player->getHandcardNum() <= 2){

                room->setPlayerMark(player, "@wu", 0);
                room->setPlayerMark(player, "@wen", 1);

                LogMessage log;
                log.type = "#MarkTurnOver";
                log.from = player;
                log.arg = "@wen";
                room->sendLog(log);

                room->detachSkillFromPlayer(player, "jiang", false);
                room->detachSkillFromPlayer(player, "qianxun", false);
                room->acquireSkill(player, "yingzi", true, false);
                room->acquireSkill(player, "keji", true, false);

                log.type = "#SkillConversion";
                log.arg = "jiang";
                log.arg2 = "yingzi";
                room->sendLog(log);
                log.arg = "qianxun";
                log.arg2 = "keji";
                room->sendLog(log);
            }
        }
        else{
            foreach(ServerPlayer *lvmeng, lvmengs)
                if((lvmeng && lvmeng->getMark("@wen") > 0) && !lvmeng->isKongcheng()
                        && lvmeng->askForSkillInvoke(objectName())){

                    room->askForDiscard(lvmeng, "mouduan", 1, 1, false, true);
                    room->setPlayerMark(lvmeng, "@wen", 0);
                    room->setPlayerMark(lvmeng, "@wu", 1);

                    LogMessage log;
                    log.type = "#MarkTurnOver";
                    log.from = lvmeng;
                    log.arg = "@wu";
                    room->sendLog(log);

                    room->detachSkillFromPlayer(lvmeng, "yingzi", false);
                    room->detachSkillFromPlayer(lvmeng, "keji", false);
                    room->acquireSkill(lvmeng, "jiang", true, false);
                    room->acquireSkill(lvmeng, "qianxun", true, false);

                    log.type = "#SkillConversion";
                    log.arg = "yingzi";
                    log.arg2 = "jiang";
                    room->sendLog(log);
                    log.arg = "keji";
                    log.arg2 = "qianxun";
                    room->sendLog(log);
                }
        }
        return false;
    }
};

BGMPackage::BGMPackage():Package("BGM"){
    General *bgm_zhaoyun = new General(this, "bgm_zhaoyun", "qun", 3, true, true);
    bgm_zhaoyun->addSkill("longdan");
    bgm_zhaoyun->addSkill(new ChongZhen);

    General *bgm_diaochan = new General(this, "bgm_diaochan", "qun", 3, false, true);
    bgm_diaochan->addSkill(new Lihun);
    bgm_diaochan->addSkill("biyue");

    General *bgm_caoren = new General(this, "bgm_caoren", "wei", 4, true, true);
    bgm_caoren->addSkill(new Kuiwei);
    bgm_caoren->addSkill(new Yanzheng);

    General *bgm_pangtong = new General(this, "bgm_pangtong", "qun", 3, true, true);
    bgm_pangtong->addSkill(new Manjuan);
    bgm_pangtong->addSkill(new Zuixiang);
    bgm_pangtong->addSkill(new MarkAssignSkill("@sleep", 1));

    General *bgm_zhangfei = new General(this, "bgm_zhangfei", "shu", 4, true, true);
    bgm_zhangfei->addSkill(new Jie);
    bgm_zhangfei->addSkill(new Dahe);

    General *bgm_lvmeng = new General(this, "bgm_lvmeng", "wu", 3, true, true);
    bgm_lvmeng->addSkill(new Tanhu);
    bgm_lvmeng->addSkill(new MouduanStart);
    bgm_lvmeng->addSkill(new Mouduan);
    related_skills.insertMulti("mouduan", "#mouduan");

    addMetaObject<LihunCard>();
    addMetaObject<DaheCard>();
    addMetaObject<TanhuCard>();
}

ADD_PACKAGE(BGM)
