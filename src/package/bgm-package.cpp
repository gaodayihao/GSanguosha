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
        events << CardResponsed  << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        if(event == CardResponsed){
            ResponsedStar resp = data.value<ResponsedStar>();
            const Card *card = resp->card;
            ServerPlayer *who = resp->who;
            if(card->getSkillName() == "longdan" && who)
            {
                QVariant to = QVariant::fromValue((PlayerStar)who);
                if(!who->isKongcheng() && player->askForSkillInvoke(objectName(), to)){
                    int card_id = room->askForCardChosen(player, who, "h", objectName());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                    room->playSkillEffect("chongzhen");
                    room->obtainCard(player, Sanguosha->getCard(card_id), reason, false);
                }
            }
        }
        else{
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.from == player && use.card->getSkillName() == "longdan"){
                foreach(ServerPlayer *p, use.to){
                    if(p->isKongcheng())
                        continue;

                    QVariant to = QVariant::fromValue((PlayerStar)p);
                    if(!player->askForSkillInvoke(objectName(), to))
                        continue;

                    int card_id = room->askForCardChosen(player, p, "h", objectName());
                    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, player->objectName());
                    room->playSkillEffect("chongzhen");
                    room->obtainCard(player, Sanguosha->getCard(card_id), reason, false);
                }
            }
        }
        return false;
    }
};

LihunCard::LihunCard(){
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
    {
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, effect.from->objectName(),
                              effect.to->objectName(), "lihun", QString());
        room->moveCardTo(cd, effect.to, effect.from, Player::PlaceHand, reason, false);
    }
    cd->deleteLater();
    effect.to->setFlags("LihunTarget");
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

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *diaochan, QVariant &data) const{
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

            CardMoveReason reason(CardMoveReason::S_REASON_GIVE, diaochan->objectName(),
                                  target->objectName(), "lihun", QString());
            reason.m_playerId = target->objectName();
            room->moveCardTo(to_goback, diaochan, target, Player::PlaceHand, reason);
            target->setFlags("-LihunTarget");
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

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *caoren, QVariant &) const{
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
            room->moveCardTo(Sanguosha->getCard(id), sp_pangtong, Player::PlaceHand, true);

        sp_pangtong->invoke("clearAG");
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *sp_pangtong, QVariant &data) const{
        if(sp_pangtong->hasFlag("ManjuanInvoke")){
            sp_pangtong->setFlags("-ManjuanInvoke");
            return false;
        }

        int card_id = -1;
        CardMoveReason reason(CardMoveReason::S_REASON_PUT, sp_pangtong->objectName(), "manjuan", QString());
        if(event == CardGotOnePiece){
            CardMoveStar move = data.value<CardMoveStar>();
            card_id = move->card_id;
            if(move->to_place == Player::PlaceHand){
                const Card* card = Sanguosha->getCard(card_id);
                room->moveCardTo(card, NULL, NULL, Player::DiscardPile, reason);
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
            const Card* card = Sanguosha->getCard(card_id);
            room->moveCardTo(card, NULL, NULL, Player::DiscardPile, reason);
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
            CardMoveReason reason(CardMoveReason::S_REASON_PUT, player->objectName(), QString(), "zuixiang", "");
            CardsMoveStruct move(zuixiang, player, Player::PlaceHand, reason);
            room->moveCards(move, true);
        }
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *sp_pangtong, QVariant &data) const{
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

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *zhangfei, QVariant &data) const{
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
    as_pindian = true;
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

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *zhangfei, QVariant &data) const{
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
                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, zhangfei->objectName());
                    reason.m_playerId = target->objectName();
                    room->obtainCard(target, pindian->to_card, reason);
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
    as_pindian = true;
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
        return target != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
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
                room->playSkillEffect(objectName());
            }
        }
        else{
            foreach(ServerPlayer *lvmeng, lvmengs)
                if((lvmeng && lvmeng->getMark("@wen") > 0) && !lvmeng->isNude()
                        && lvmeng->askForSkillInvoke(objectName())){

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
                    room->playSkillEffect(objectName());
                    room->askForDiscard(lvmeng, "mouduan", 1, 1, false, true);
                }
        }
        return false;
    }
};

class Zhaolie: public DrawCardsSkill{
public:
    Zhaolie():DrawCardsSkill("zhaolie"){
    }

    virtual int getPriority() const{
        return 5;
    }

    virtual int getDrawNum(ServerPlayer *liubei, int n) const{
        Room *room = liubei->getRoom();
        QList<ServerPlayer *> victims;
        foreach(ServerPlayer *p, room->getOtherPlayers(liubei)){
            if(liubei->inMyAttackRange(p)){
                victims << p;
            }
        }
        if(victims.isEmpty())
            return n;
        if(liubei->askForSkillInvoke(objectName()))
        {
            Room *room = liubei->getRoom();
            room->setPlayerFlag(liubei, "ZhaolieInvoke");
            room->playSkillEffect(objectName());
            return --n;
        }
        return n;
    }
};

class ZhaolieDo:public TriggerSkill{
public:
    ZhaolieDo():TriggerSkill("#zhaolie"){
        events << CardDrawnDone;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->hasFlag("ZhaolieInvoke");
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *liubei, QVariant &) const{
        room->setPlayerFlag(liubei, "-ZhaolieInvoke");
        QList<ServerPlayer *> victims;
        QList<int> card_ids;
        int no_basic = 0;

        foreach(ServerPlayer *p, room->getOtherPlayers(liubei)){
            if(liubei->inMyAttackRange(p)){
                victims << p;
            }
        }
        if(victims.empty())
            return false;

        ServerPlayer *victim = room->askForPlayerChosen(liubei, victims, "zhaolie");
        for(int i = 0; i < 3; i++){
            int card_id = room->drawCard();
            room->moveCardTo(Sanguosha->getCard(card_id), NULL, NULL, Player::TopDrawPile,
                             CardMoveReason(CardMoveReason::S_REASON_TURNOVER, QString(), QString(), "zhaolie", QString()), true);
            room->getThread()->delay();

            const Card *card = Sanguosha->getCard(card_id);
            if(card->getTypeId() != Card::Basic || card->inherits("Peach")){
                if(!card->inherits("BasicCard")){
                    no_basic++;
                }
                CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString(), "zhaolie", QString());
                room->throwCard(Sanguosha->getCard(card_id), reason, NULL);
            }else{
                card_ids << card_id;
            }
        }
        QStringList choicelist;
        choicelist << "damage";
        if (victim->getCardCount(true) >= no_basic){
            choicelist << "throw";
        }
        QString choice;
        if (choicelist.length() == 1){
            choice = choicelist.first();
        }
        else{
            QVariant data = QVariant::fromValue(no_basic);
            choice = room->askForChoice(victim, "zhaolie", choicelist.join("+"), data);
        }
        if(choice == "damage"){
            if(no_basic > 0){
                DamageStruct damage;
                damage.card = NULL;
                damage.from = liubei;
                damage.to = victim;
                damage.damage = no_basic;

                room->damage(damage);
            }
            if(!card_ids.isEmpty()){
                QList<CardsMoveStruct> moves;
                CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, victim->objectName());
                CardsMoveStruct move(card_ids, victim, Player::PlaceHand, reason);
                moves.append(move);
                room->moveCardsAtomic(moves, true);
            }
        }
        else{
			for(int i = 0; i < no_basic; i++){
				room->askForDiscard(victim, "zhaolie", 1, 1, false, true);
            }
            if(!card_ids.isEmpty()){
                QList<CardsMoveStruct> moves;
                CardMoveReason reason(CardMoveReason::S_REASON_GOTBACK, liubei->objectName());
                CardsMoveStruct move(card_ids, liubei, Player::PlaceHand, reason);
                moves.append(move);
                room->moveCardsAtomic(moves, true);
            }
        }
        return false;
    }
};

class Shichou: public TriggerSkill{
public:
    Shichou(): TriggerSkill("shichou$"){
        events << GameStart << PhaseChange << DamageInflicted << Dying << DamageComplete;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return player != NULL;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
        switch(event){
        case GameStart:{
            foreach(ServerPlayer *lord, room->getAlivePlayers())
                if(lord->hasLordSkill(objectName()))
                    room->setPlayerMark(lord, "@hate", 1);
            break;
        }
        case PhaseChange:{
            if(player->hasLordSkill(objectName()) && player->getPhase() == Player::Start &&
                    player->getMark("shichouInvoke") == 0 && player->getCards("he").length() > 1)
            {
                if(player->getMark("@hate") == 0)
                    room->setPlayerMark(player, "@hate", 1);

                QList<ServerPlayer *> victims;

                foreach(ServerPlayer *p, room->getAlivePlayers()){
                    if(p->getKingdom() == "shu"){
                        if(!p->tag.value("ShichouTarget").isNull()
                                && p->tag.value("ShichouTarget").value<PlayerStar>() == player)
                            continue;
                        else
                            victims << p;
                    }
                }
                if(victims.empty())
                    return false;
                if(player->askForSkillInvoke(objectName())){
                    player->loseMark("@hate", 1);
                    room->setPlayerMark(player, "shichouInvoke", 1);
                    room->playSkillEffect(objectName());
                    ServerPlayer *victim = room->askForPlayerChosen(player, victims, objectName());
                    room->setPlayerMark(victim, "@chou", 1);
                    player->tag["ShichouTarget"] = QVariant::fromValue((PlayerStar)victim);

                    const Card *card = room->askForExchange(player, objectName(), 2, true, "ShichouGive");
                    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, player->objectName());
                    reason.m_playerId = victim->objectName();
                    room->obtainCard(victim, card, reason, false);
                }
            }
            break;
        }
        case DamageInflicted:{
            if(player->hasLordSkill("shichou", true) && !player->tag.value("ShichouTarget").isNull())
            {
                ServerPlayer *target = player->tag.value("ShichouTarget").value<PlayerStar>();

                LogMessage log;
				log.type = "#ShichouProtect";
				log.arg = objectName();
				log.from = player;
				log.to << target;
				room->sendLog(log);
				room->setPlayerFlag(target, "Shichou");

				if(target != player)
				{
					DamageStruct damage = data.value<DamageStruct>();
					damage.to = target;
					damage.chain = true;
                    room->damage(damage);

                    damage.transfer = true;
                    data = QVariant::fromValue(damage);
					return true;
				}
            }
            break;
        }
        case DamageComplete:{
            if(player->hasFlag("Shichou")){
                DamageStruct damage = data.value<DamageStruct>();
                player->drawCards(damage.damage);
                room->setPlayerFlag(player, "-Shichou");
            }
            break;
        }
        case Dying:{
            if(player->getMark("@chou") > 0)
            {
                player->loseMark("@chou");
                foreach(ServerPlayer *lord, room->getAlivePlayers())
                {
                    if(lord->hasLordSkill(objectName())
                            && lord->tag.value("ShichouTarget").value<PlayerStar>() == player)
                        lord->tag.remove("ShichouTarget");
                }
            }
            break;
        }
        default:
            break;
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

    General *bgm_liubei = new General(this, "bgm_liubei$", "shu", 4, true, true);
    bgm_liubei->addSkill(new Zhaolie);
    bgm_liubei->addSkill(new ZhaolieDo);
    bgm_liubei->addSkill(new Shichou);

    related_skills.insertMulti("zhaolie", "#zhaolie");

    addMetaObject<LihunCard>();
    addMetaObject<DaheCard>();
    addMetaObject<TanhuCard>();
}

ADD_PACKAGE(BGM)
