#include "yjcm2012-package.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"
#include "carditem.h"
#include "engine.h"
#include "god.h"
#include "maneuvering.h"


class Zhenlie: public TriggerSkill{
public:
    Zhenlie():TriggerSkill("zhenlie"){
        events << AskForRetrial;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        JudgeStar judge = data.value<JudgeStar>();
        if(!judge->who->hasSkill(objectName()))
            return false;

        if(player->askForSkillInvoke(objectName(), data)){
            room->playSkillEffect(objectName());

            int card_id = room->drawCard();
            room->throwCard(judge->card);

            judge->card = Sanguosha->getCard(card_id);
            room->moveCardTo(judge->card, NULL, Player::Special);

            LogMessage log;
            log.type = "$ChangedJudge";
            log.from = player;
            log.to << judge->who;
            log.card_str = judge->card->getEffectIdString();
            room->sendLog(log);

            room->sendJudgeResult(judge);
            room->getThread()->delay(500);
        }
        return false;
    }
};

class Miji: public PhaseChangeSkill{
public:
    Miji():PhaseChangeSkill("miji"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *wangyi) const{
        if(!wangyi->isWounded())
            return false;
        if(wangyi->getPhase() == Player::Start || wangyi->getPhase() == Player::Finish){
            if(!wangyi->askForSkillInvoke(objectName()))
                return false;
            Room *room = wangyi->getRoom();
            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(club|spade):(.*)");
            judge.good = true;
            judge.reason = objectName();
            judge.who = wangyi;

            room->judge(judge);

            if(judge.isGood()){
                int x = wangyi->getLostHp();

                LogMessage log;
                log.type = "#WatchNCards";
                log.from = wangyi;
                log.arg = QString::number(x);
                room->sendLog(log);

                room->playSkillEffect(objectName());

                QList<int> miji_cards = room->getNCards(x, false);
                room->fillAG(miji_cards, wangyi);
                room->askForAG(wangyi, miji_cards, true, objectName());
                room->broadcastInvoke("clearAG");
                room->addToDrawPile(miji_cards);

                ServerPlayer *target = room->askForPlayerChosen(wangyi, room->getAllPlayers(), objectName());

                log.type = "#Miji";
                log.arg = objectName();
                log.to << target;
                room->sendLog(log);

                room->setPlayerFlag(target, "SpecialDraw");
                target->drawCards(x, false);
                room->setPlayerFlag(target, "-SpecialDraw");
            }
        }
        return false;
    }
};

QiceCard::QiceCard(){
    will_throw = true;
    mute = true;
}

bool QiceCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    CardStar card = Self->tag.value("qice").value<CardStar>();
    return card && card->targetFilter(targets, to_select, Self) && !Self->isProhibited(to_select, card);
}

bool QiceCard::targetFixed() const{
    if(ClientInstance->getStatus() == Client::Responsing)
        return true;

    CardStar card = Self->tag.value("qice").value<CardStar>();
    return card && card->targetFixed();
}

bool QiceCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    CardStar card = Self->tag.value("qice").value<CardStar>();
    return card && card->targetsFeasible(targets, Self);
}

Card::Suit QiceCard::getSuit(QList<int> cardid_list) const{
    QSet<QString> suit_set;
    QSet<Card::Color> color_set;
    foreach(int id, cardid_list){
        const Card *cd = Sanguosha->getCard(id);
        suit_set << cd->getSuitString();
        color_set << cd->getColor();
    }
    if(color_set.size() == 2)
        return Card::NoSuit;
    else{
        if(suit_set.size() == 1)
            return Sanguosha->getCard(cardid_list.first())->getSuit();
        else{
            if(Sanguosha->getCard(cardid_list.first())->isBlack())
                return Card::Spade;
            else
                return Card::Heart;
        }
    }
}

int QiceCard::getNumber(QList<int> cardid_list) const{
    if(cardid_list.length() == 1)
        return Sanguosha->getCard(cardid_list.first())->getNumber();
    else
        return 0;
}

const Card *QiceCard::validate(const CardUseStruct *card_use) const{
    Room *room = card_use->from->getRoom();
    card_use->from->setFlags("QiceUsed");
    room->playSkillEffect("qice");
    Card *use_card = Sanguosha->cloneCard(user_string, getSuit(this->getSubcards()), getNumber(this->getSubcards()));
    use_card->setSkillName("qice");
    foreach(int id, this->getSubcards())
        use_card->addSubcard(id);
    return use_card;
}

const Card *QiceCard::validateInResposing(ServerPlayer *xunyou, bool *continuable) const{
    *continuable = true;

    Room *room = xunyou->getRoom();
    room->playSkillEffect("qice");
    xunyou->setFlags("QiceUsed");

    Card *use_card = Sanguosha->cloneCard(user_string, getSuit(this->getSubcards()), getNumber(this->getSubcards()));
    use_card->setSkillName("qice");
    foreach(int id, this->getSubcards())
        use_card->addSubcard(id);

    return use_card;
}

class Qice: public ViewAsSkill{
public:
    Qice():ViewAsSkill("qice"){
    }

    virtual QDialog *getDialog() const{
        return GuhuoDialog::GetInstance("qice", false);
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() < Self->getHandcardNum())
            return NULL;

        if(ClientInstance->getStatus() == Client::Responsing){
            QiceCard *card = new QiceCard;
            card->setUserString("nullification");
            card->addSubcards(cards);
            return card;
        }

        CardStar c = Self->tag.value("qice").value<CardStar>();
        if(c){
            QiceCard *card = new QiceCard;
            card->setUserString(c->objectName());
            card->addSubcards(cards);
            return card;
        }else
            return NULL;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        if(player->isKongcheng())
            return false;
        else
            return !player->hasUsed("QiceCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "nullification" &&
                !player->hasFlag("QiceUsed") &&
                !player->isKongcheng() &&
                player->getPhase() == Player::Play ;
    }
};

class Zhiyu: public MasochismSkill{
public:
    Zhiyu():MasochismSkill("zhiyu"){

    }

    virtual void onDamaged(ServerPlayer *target, const DamageStruct &damage) const{
        if(target->askForSkillInvoke(objectName(), QVariant::fromValue(damage))){
            target->getRoom()->playSkillEffect(objectName());

            target->drawCards(1);

            Room *room = target->getRoom();
            room->showAllCards(target);

            QList<const Card *> cards = target->getHandcards();
            Card::Color color = cards.first()->getColor();
            bool same_color = true;
            foreach(const Card *card, cards){
                if(card->getColor() != color){
                    same_color = false;
                    break;
                }
            }

            if(same_color && damage.from && !damage.from->isKongcheng())
                room->askForDiscard(damage.from, objectName(), 1);
        }
    }
};

class Jiangchi:public DrawCardsSkill{
public:
    Jiangchi():DrawCardsSkill("jiangchi"){
    }

    virtual int getDrawNum(ServerPlayer *caozhang, int n) const{
        Room *room = caozhang->getRoom();
        QString choice = room->askForChoice(caozhang, objectName(), "jiang+chi+cancel");
        if(choice == "cancel")
            return n;
        LogMessage log;
        log.from = caozhang;
        log.arg = objectName();
        if(choice == "jiang"){
            log.type = "#Jiangchi1";
            room->sendLog(log);
            room->playSkillEffect(objectName(), 1);
            room->setPlayerCardLock(caozhang, "Slash");
            room->setPlayerFlag(caozhang, "slash_lock");
            return n + 1;

            room->playSkillEffect(objectName());
        }else{
            log.type = "#Jiangchi2";
            room->sendLog(log);
            room->playSkillEffect(objectName(), 2);
            room->setPlayerFlag(caozhang, "jiangchi_invoke");
            return n - 1;
        }
        return n;
    }
};

class JiangchiClear: public PhaseChangeSkill{
public:
    JiangchiClear():PhaseChangeSkill("#jiangchi-clear"){
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target);
    }

    virtual bool onPhaseChange(ServerPlayer *zhangzi) const{
        if(zhangzi->getPhase() == Player::NotActive && zhangzi->hasCardLock("Slash"))
            zhangzi->getRoom()->setPlayerCardLock(zhangzi, "-Slash");
        return false;
    }
};

class Qianxi: public TriggerSkill{
public:
    Qianxi():TriggerSkill("qianxi"){
        events << DamageProceed;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        DamageStruct damage = data.value<DamageStruct>();

        if(player->distanceTo(damage.to) == 1 && damage.card && damage.card->inherits("Slash") &&
           player->askForSkillInvoke(objectName(), data)){
            room->playSkillEffect(objectName());
            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart):(.*)");
            judge.good = false;
            judge.who = player;
            judge.reason = objectName();

            room->judge(judge);
            if(judge.isGood()){
                LogMessage log;
                log.type = "#Qianxi";
                log.from = player;
                log.arg = objectName();
                log.to << damage.to;
                room->sendLog(log);
                room->loseMaxHp(damage.to);
                return true;
            }
        }
        return false;
    }
};

class Dangxian: public PhaseChangeSkill{
public:
    Dangxian():PhaseChangeSkill("dangxian"){
        frequency = Compulsory;
    }

    virtual bool onPhaseChange(ServerPlayer *liaohua) const{
        Room *room = liaohua->getRoom();
        if(liaohua->getPhase() == Player::RoundStart){
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = liaohua;
            log.arg = objectName();
            room->sendLog(log);
            room->playSkillEffect(objectName());

            QList<Player::Phase> phases = liaohua->getPhases();
            phases.prepend(Player::Play) ;
            room->setEmotion(liaohua, QString("skill/%1").arg(objectName()));
            liaohua->play(phases);
        }
        return false;
    }
};

class Fuli: public TriggerSkill{
public:
    Fuli():TriggerSkill("fuli"){
        events << Dying;
        frequency = Limited;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("@laoji") > 0;
    }

    int getKingdoms(Room *room) const{
        QSet<QString> kingdom_set;
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            kingdom_set << p->getKingdom();
        }
        return kingdom_set.size();
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *liaohua, QVariant &data) const{
        DyingStruct dying_data = data.value<DyingStruct>();
        if(dying_data.who != liaohua)
            return false;

        Room *room = liaohua->getRoom();
        if(liaohua->askForSkillInvoke(objectName(), data)){
            //room->broadcastInvoke("animate", "lightbox:$fuli");
            room->playSkillEffect(objectName());

            liaohua->loseMark("@laoji");
            int x = getKingdoms(room);
            RecoverStruct rev;
            rev.recover = x - liaohua->getHp();
            room->recover(liaohua, rev);
            liaohua->turnOver();
        }
        return false;
    }
};

class Fuhun: public PhaseChangeSkill{
public:
    Fuhun():PhaseChangeSkill("fuhun"){

    }

    const Card *getCard(ServerPlayer *player) const{
        Room *room = player->getRoom();
        int card_id = room->drawCard();
        const Card *card = Sanguosha->getCard(card_id);
        room->moveCardTo(card, NULL, Player::Special, true);
        room->getThread()->delay();

        player->obtainCard(card);

        return card;
    }

    virtual bool onPhaseChange(ServerPlayer *shuangying) const{
        switch(shuangying->getPhase()){
        case Player::Draw:{
                if(shuangying->askForSkillInvoke(objectName())){
                    Room *room = shuangying->getRoom();
                    room->playSkillEffect(objectName());
                    const Card *first = getCard(shuangying);
                    const Card *second = getCard(shuangying);

                    if(first->getColor() != second->getColor()){
                        room->setEmotion(shuangying, "good");
                        room->acquireSkill(shuangying, "wusheng");
                        room->getThread()->delay();
                        room->acquireSkill(shuangying, "paoxiao");

                        shuangying->setFlags(objectName());
                    }else
                        room->setEmotion(shuangying, "bad");

                    return true;
                }

                break;
            }

        case Player::NotActive:{
                if(shuangying->hasFlag(objectName())){
                    Room *room = shuangying->getRoom();
                    room->detachSkillFromPlayer(shuangying, "wusheng");
                    room->detachSkillFromPlayer(shuangying, "paoxiao");
                }

                break;

            }

        default:
            break;
        }

        return false;
    }
};

class Zishou:public DrawCardsSkill{
public:
    Zishou():DrawCardsSkill("zishou"){
    }

    virtual int getDrawNum(ServerPlayer *liubiao, int n) const{
        Room *room = liubiao->getRoom();
        if(room->askForSkillInvoke(liubiao, objectName())){
            room->playSkillEffect(objectName());
            liubiao->clearHistory();
            liubiao->skip(Player::Play);
            return n + liubiao->getLostHp();
        }else
            return n;
    }
};

class Shiyong: public TriggerSkill{
public:
    Shiyong():TriggerSkill("shiyong"){
        events << SlashHit;
        frequency = Compulsory;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if(!effect.to->hasSkill(objectName()))
            return false;

        if(effect.slash->isRed() || effect.drank){
            Room *room = player->getRoom();
            room->playSkillEffect(objectName());
            LogMessage log;
            log.type = "#TriggerSkill";
            log.from = effect.to;
            log.arg = objectName();
            room->sendLog(log);

            room->loseMaxHp(effect.to);
        }
        return false;
    }


    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getRoom()->findPlayerBySkillName(objectName());
    }
};

class Gongqi : public OneCardViewAsSkill{
public:
    Gongqi():OneCardViewAsSkill("gongqi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "slash";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->getTypeId() == Card::Equip;
    }

    const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();
        WushenSlash *slash = new WushenSlash(card->getSuit(), card->getNumber());
        slash->addSubcard(card);
        slash->setSkillName(objectName());
        return slash;
    }
};

class Jiefan : public TriggerSkill{
public:
    Jiefan():TriggerSkill("jiefan"){
        events << Dying << DamagedProceed << CardFinished;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        ServerPlayer *handang = room->findPlayerBySkillName(objectName());

        if(event == Dying){
            DyingStruct dying = data.value<DyingStruct>();
            if(!handang || !dying.savers.contains(handang) || dying.who->getHp() > 0 ||
               handang->isNude() || !room->askForSkillInvoke(handang, objectName(), data))
                return false;
            room->playSkillEffect(objectName());

            const Card *slash = room->askForCard(handang, "slash", "jiefan-slash:" + dying.who->objectName(), data);

            if(slash){
                room->setTag("JiefanTarget", data);
                room->setTag("JiefanSlash", slash->getEffectiveId());
                slash->setFlags("jiefan-slash");
                CardUseStruct use;
                use.card = slash;
                use.from = handang;
                use.to << room->getCurrent();
                room->useCard(use);
            }
        }
        else if(event == DamagedProceed){
            DamageStruct damage = data.value<DamageStruct>();
            int id = room->getTag("JiefanSlash").toInt();
            if(player->hasSkill(objectName()) && damage.card && damage.card->inherits("Slash")
                && !room->getTag("JiefanTarget").isNull()
                && id == damage.card->getEffectiveId()){

                DyingStruct dying = room->getTag("JiefanTarget").value<DyingStruct>();
                ServerPlayer *target = dying.who;
                room->removeTag("JiefanTarget");
                Peach *peach = new Peach(damage.card->getSuit(), damage.card->getNumber());
                peach->setSkillName(objectName());
                CardUseStruct use;
                use.card = peach;
                use.from = handang;
                use.to << target;
                room->useCard(use);

                return true;
            }
            return false;
        }
        else{
            if(!room->getTag("JiefanTarget").isNull() && !room->getTag("JiefanSlash").isNull()){
                room->removeTag("JiefanTarget");
                room->removeTag("JiefanSlash");
            }
        }

        return false;
    }
};

AnxuCard::AnxuCard(){
    once = true;
}

bool AnxuCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select == Self)
        return false;
    if(targets.isEmpty())
        return true;
    else if(targets.length() == 1)
        return to_select->getHandcardNum() != targets.first()->getHandcardNum();
    else
        return false;
}

bool AnxuCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void AnxuCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHandcardNum() < selecteds.last()->getHandcardNum() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    int id = room->askForCardChosen(from, to, "h", "anxu");
    const Card *cd = Sanguosha->getCard(id);
    room->moveCardTo(cd, from, Player::Hand, true);
    room->showCard(from, id);
    if(cd->getSuit() != Card::Spade)
        source->drawCards(1);
}

class Anxu: public ZeroCardViewAsSkill{
public:
    Anxu():ZeroCardViewAsSkill("anxu"){
    }

    virtual const Card *viewAs() const{
        return new AnxuCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("AnxuCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return false;
    }
};

class Zhuiyi: public TriggerSkill{
public:
    Zhuiyi():TriggerSkill("zhuiyi"){
        events << Death ;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        DamageStar damage = data.value<DamageStar>();
        QList<ServerPlayer *> targets = (damage && damage->from) ?
                                        room->getOtherPlayers(damage->from) : room->getAlivePlayers();

        if(targets.isEmpty() || !player->askForSkillInvoke(objectName(), data))
            return false;

        ServerPlayer *target = room->askForPlayerChosen(player, targets, objectName());

        room->playSkillEffect(objectName());
        LogMessage log;
        log.type = "#ZhuiyiLog";
        log.from = player;
        log.arg = target->getGeneralName();
        log.arg2 = objectName();
        room->sendLog(log);

        target->drawCards(3);
        RecoverStruct recover;
        recover.who = target;
        room->recover(target, recover, true);
        return false;
    }
};

class LihuoViewAsSkill:public OneCardViewAsSkill{
public:
    LihuoViewAsSkill():OneCardViewAsSkill("lihuo"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->objectName() == "slash";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *acard = new FireSlash(card->getSuit(), card->getNumber());
        acard->addSubcard(card->getId());
        acard->setSkillName(objectName());
        return acard;
    }
};

class Lihuo: public TriggerSkill{
public:
    Lihuo():TriggerSkill("lihuo"){
        events << DamagedProceed << CardFinished;
        view_as_skill = new LihuoViewAsSkill;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == DamagedProceed){
            DamageStruct damage = data.value<DamageStruct>();
            if(damage.card && damage.card->inherits("Slash") && damage.card->getSkillName() == objectName())
                player->tag["Invokelihuo"] = true;
        }
        else if(player->tag.value("Invokelihuo", false).toBool()){
            player->tag["Invokelihuo"] = false;
            room->loseHp(player, 1);
        }
        return false;
    }
};

ChunlaoCard::ChunlaoCard(){
    will_throw = false;
    target_fixed = true;
}

void ChunlaoCard::use(Room *, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    foreach(int id, this->subcards){
        source->addToPile("wine", id, true);
    }
}

class ChunlaoViewAsSkill:public ViewAsSkill{
public:
    ChunlaoViewAsSkill():ViewAsSkill("chunlao"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@chunlao";
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return to_select->getFilteredCard()->inherits("Slash");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() == 0)
            return NULL;

        Card *acard = new ChunlaoCard;
        acard->addSubcards(cards);
        acard->setSkillName(objectName());
        return acard;
    }
};

class Chunlao: public TriggerSkill{
public:
    Chunlao():TriggerSkill("chunlao"){
        events << PhaseChange << AskForPeaches ;
        view_as_skill = new ChunlaoViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->getRoom()->findPlayerBySkillName(objectName());
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        if(event == PhaseChange){
            QList<ServerPlayer *>chengpus = room->findPlayersBySkillName(objectName());
            foreach(ServerPlayer *chengpu, chengpus)
                if(chengpu->getPhase() == Player::Finish && !chengpu->isKongcheng() && chengpu->getPile("wine").isEmpty())
                    room->askForUseCard(chengpu, "@@chunlao", "@chunlao");
        }


        if(event == AskForPeaches){
            QList<ServerPlayer *>chengpus = room->findPlayersBySkillName(objectName());
            DyingStruct dying = data.value<DyingStruct>();
            foreach(ServerPlayer *chengpu, chengpus){
                while(!chengpu->getPile("wine").isEmpty() && dying.who->getHp() < 1
                      && player == chengpu && chengpu->askForSkillInvoke(objectName(), data)){
                    QList<int> cards = chengpu->getPile("wine");
                    room->fillAG(cards, chengpu);
                    int card_id = room->askForAG(chengpu, cards, true, objectName());
                    room->broadcastInvoke("clearAG");
                    if(card_id != -1){
                        room->playSkillEffect(objectName());
                        room->throwCard(card_id);
                        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
                        analeptic->setSkillName(objectName());
                        CardUseStruct use;
                        use.card = analeptic;
                        use.from = dying.who;
                        use.to << dying.who;
                        room->useCard(use);
                    }
                }
            }
        }
        return false;
    }
};


YJCM2012Package::YJCM2012Package():Package("YJCM2012"){

    General *wangyi = new General(this, "wangyi", "wei", 3, false);
    wangyi->addSkill(new Zhenlie);
    wangyi->addSkill(new Miji);

    General *xunyou = new General(this, "xunyou", "wei", 3);
    xunyou->addSkill(new Qice);
    xunyou->addSkill(new Zhiyu);

    General *caozhang = new General(this, "caozhang", "wei");
    caozhang->addSkill(new Jiangchi);
    caozhang->addSkill(new JiangchiClear);
    related_skills.insertMulti("jiangchi", "#jiangchi-clear");

    General *madai = new General(this, "madai", "shu");
    madai->addSkill(new Qianxi);
    madai->addSkill("mashu");

    General *liaohua = new General(this, "liaohua", "shu");
    liaohua->addSkill(new Dangxian);
    liaohua->addSkill(new MarkAssignSkill("@laoji", 1));
    liaohua->addSkill(new Fuli);

    General *guanxingzhangbao = new General(this, "guanxingzhangbao", "shu");
    guanxingzhangbao->addSkill(new Fuhun);

    General *chengpu = new General(this, "chengpu", "wu");
    chengpu->addSkill(new Lihuo);
    chengpu->addSkill(new Chunlao);

    General *bulianshi = new General(this, "bulianshi", "wu", 3, false);
    bulianshi->addSkill(new Anxu);
    bulianshi->addSkill(new Zhuiyi);

    General *handang = new General(this, "handang", "wu");
    handang->addSkill(new Gongqi);
    handang->addSkill(new Jiefan);

    General *liubiao = new General(this, "liubiao", "qun", 4);
    liubiao->addSkill(new Zishou);
    liubiao->addSkill(new Skill("zongshi", Skill::Compulsory));

    General *huaxiong = new General(this, "huaxiong", "qun", 6);
    huaxiong->addSkill(new Shiyong);

    addMetaObject<QiceCard>();
    addMetaObject<ChunlaoCard>();
    addMetaObject<AnxuCard>();
}

ADD_PACKAGE(YJCM2012)
