#include "general.h"
#include "standard.h"
#include "skill.h"
#include "engine.h"
#include "client.h"
#include "carditem.h"
#include "serverplayer.h"
#include "room.h"
#include "standard-skillcards.h"
#include "ai.h"
#include "settings.h"

class Jianxiong:public MasochismSkill{
public:
    Jianxiong():MasochismSkill("jianxiong"){
    }

    virtual void onDamaged(ServerPlayer *caocao, const DamageStruct &damage) const{
        Room *room = caocao->getRoom();
        const Card *card = damage.card;
        if(!room->obtainable(card, caocao))
            return;

        QVariant data = QVariant::fromValue(card);
        if(room->askForSkillInvoke(caocao, "jianxiong", data)){
            room->playSkillEffect(objectName());
            caocao->obtainCard(card);
        }
    }
};

class Hujia:public TriggerSkill{
public:
    Hujia():TriggerSkill("hujia$"){
        events << CardAsked;
        default_choice = "ignore";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasLordSkill("hujia");
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *caocao, QVariant &data) const{
        QString pattern = data.toString();
        if(pattern != "jink")
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("wei", caocao);
        if(lieges.isEmpty())
            return false;

        if(!room->askForSkillInvoke(caocao, objectName()))
            return false;

        room->playSkillEffect(objectName());
        QVariant tohelp = QVariant::fromValue((PlayerStar)caocao);
        foreach(ServerPlayer *liege, lieges){
            const Card *jink = room->askForCard(liege, "jink", "@hujia-jink:" + caocao->objectName(), tohelp, NonTrigger);
            if(jink){
                LogMessage log;
                log.type = "#Hujia";
                log.from = liege;
                log.to << caocao;
                log.arg = objectName();
                room->sendLog(log);
                room->provide(jink);
                return true;
            }
        }

        return false;
    }
};

class TuxiViewAsSkill: public ZeroCardViewAsSkill{
public:
    TuxiViewAsSkill():ZeroCardViewAsSkill("tuxi"){
    }

    virtual const Card *viewAs() const{
        return new TuxiCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@tuxi";
    }
};

class Tuxi:public PhaseChangeSkill{
public:
    Tuxi():PhaseChangeSkill("tuxi"){
        view_as_skill = new TuxiViewAsSkill;
    }

    virtual bool onPhaseChange(ServerPlayer *zhangliao) const{
        if(zhangliao->getPhase() == Player::Draw){
            Room *room = zhangliao->getRoom();
            bool can_invoke = false;
            QList<ServerPlayer *> other_players = room->getOtherPlayers(zhangliao);
            foreach(ServerPlayer *player, other_players){
                if(!player->isKongcheng()){
                    can_invoke = true;
                    break;
                }
            }

            if(can_invoke && room->askForUseCard(zhangliao, "@@tuxi", "@tuxi-card"))
                return true;
        }

        return false;
    }
};

class Tiandu:public TriggerSkill{
public:
    Tiandu():TriggerSkill("tiandu"){
        frequency = Frequent;
        default_choice = "no";
        events << FinishJudge;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *guojia, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();
        CardStar card = judge->card;

        QVariant data_card = QVariant::fromValue(card);
        if(guojia->askForSkillInvoke(objectName(), data_card)){
            if(card->objectName() == "shit"){
                QString result = room->askForChoice(guojia, objectName(), "yes+no");
                if(result == "no")
                    return false;
            }

            guojia->obtainCard(judge->card);
            room->playSkillEffect(objectName());

            return true;
        }

        return false;
    }
};

class Yiji:public MasochismSkill{
public:
    Yiji():MasochismSkill("yiji"){
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *guojia, const DamageStruct &damage) const{
        Room *room = guojia->getRoom();

        if(!room->askForSkillInvoke(guojia, objectName()))
            return;

        room->playSkillEffect(objectName());

        int x = damage.damage, i;
        for(i=0; i<x; i++){
            guojia->drawCards(2, true, objectName());
            QList<int> yiji_cards;
            foreach(const Card *card, guojia->getHandcards()){
                if(card->hasFlag(objectName())){
                    room->setCardFlag(card, "-" + objectName());
                    yiji_cards << card->getEffectiveId();
                }
            }

            if(yiji_cards.isEmpty())
                continue;

            while(room->askForYiji(guojia, yiji_cards))
                ; // empty loop
        }

    }
};

class Ganglie:public MasochismSkill{
public:
    Ganglie():MasochismSkill("ganglie"){

    }

    virtual void onDamaged(ServerPlayer *xiahou, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = xiahou->getRoom();
        QVariant source = QVariant::fromValue(from);

        if(from && from->isAlive() && room->askForSkillInvoke(xiahou, "ganglie", source)){
            room->playSkillEffect(objectName());

            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart):(.*)");
            judge.good = false;
            judge.reason = objectName();
            judge.who = xiahou;

            room->judge(judge);
            if(judge.isGood()){
                if(!room->askForDiscard(from, objectName(), 2, 2, true)){
                    DamageStruct damage;
                    damage.from = xiahou;
                    damage.to = from;

                    room->setEmotion(xiahou, "good");
                    room->damage(damage);
                }
            }else
                room->setEmotion(xiahou, "bad");
        }
    }
};

class Fankui:public MasochismSkill{
public:
    Fankui():MasochismSkill("fankui"){

    }

    virtual void onDamaged(ServerPlayer *simayi, const DamageStruct &damage) const{
        ServerPlayer *from = damage.from;
        Room *room = simayi->getRoom();
        QVariant data = QVariant::fromValue(from);
        if(from && !from->isNude() && room->askForSkillInvoke(simayi, "fankui", data)){
            int card_id = room->askForCardChosen(simayi, from, "he", "fankui");
            room->obtainCard(simayi, card_id, room->getCardPlace(card_id) != Player::Hand);
            room->playSkillEffect(objectName());
        }
    }
};

class GuicaiViewAsSkill:public OneCardViewAsSkill{
public:
    GuicaiViewAsSkill():OneCardViewAsSkill(""){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@guicai";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new GuicaiCard;
        card->setSuit(card_item->getFilteredCard()->getSuit());
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class Guicai: public TriggerSkill{
public:
    Guicai():TriggerSkill("guicai"){
        view_as_skill = new GuicaiViewAsSkill;

        events << AskForRetrial;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && !target->isKongcheng();
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        JudgeStar judge = data.value<JudgeStar>();

        QStringList prompt_list;
        prompt_list << "@guicai-card" << judge->who->objectName()
                << objectName() << judge->reason << judge->card->getEffectIdString();
        QString prompt = prompt_list.join(":");
        const Card *card = room->askForCard(player, "@guicai", prompt, data);

        if(card){
            // the only difference for Guicai & Guidao
            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, player->objectName(), "guicai", QString());
            reason.m_targetId = player->objectName();
            room->throwCard(judge->card, reason, judge->who);

            judge->card = Sanguosha->getCard(card->getEffectiveId());
            room->moveCardTo(judge->card, NULL, Player::PlaceTakeoff,
                             CardMoveReason(CardMoveReason::S_REASON_JUDGE, player->objectName(), "guicai", QString()), true);

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

class LuoyiBuff: public TriggerSkill{
public:
    LuoyiBuff():TriggerSkill("#luoyi"){
        events << DamageBegin;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasFlag("luoyi") && target->isAlive();
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *xuchu, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        const Card *reason = damage.card;

        if(reason && (reason->inherits("Slash") || reason->inherits("Duel"))){
            LogMessage log;
            log.type = "#LuoyiBuff";
            log.from = xuchu;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);
            log.arg2 = QString::number(damage.damage + 1);
            room->sendLog(log);

            damage.damage ++;
            data = QVariant::fromValue(damage);
        }

        return false;
    }
};

class Luoyi: public DrawCardsSkill{
public:
    Luoyi():DrawCardsSkill("luoyi"){

    }

    virtual int getDrawNum(ServerPlayer *xuchu, int n) const{
        Room *room = xuchu->getRoom();
        if(room->askForSkillInvoke(xuchu, objectName())){
            room->playSkillEffect(objectName());

            xuchu->setFlags(objectName());
            return n - 1;
        }else
            return n;
    }
};

class Luoshen:public TriggerSkill{
public:
    Luoshen():TriggerSkill("luoshen"){
        events << PhaseChange << FinishJudge;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *zhenji, QVariant &data) const{
        if(event == PhaseChange && zhenji->getPhase() == Player::Start){
            int i = 0;
            while(zhenji->askForSkillInvoke("luoshen")){
                if(i == 0)
                    room->playSkillEffect(objectName());
                i++;

                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(spade|club):(.*)");
                judge.good = true;
                judge.reason = objectName();
                judge.who = zhenji;
                judge.time_consuming = true;

                room->judge(judge);
                if(judge.isBad())
                    break;
            }

        }else if(event == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if(judge->reason == objectName()){
                if(judge->card->isBlack()){
                    zhenji->obtainCard(judge->card);
                    return true;
                }
            }
        }

        return false;
    }
};

class Qingguo:public OneCardViewAsSkill{
public:
    Qingguo():OneCardViewAsSkill("qingguo"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->isBlack() && !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Jink *jink = new Jink(card->getSuit(), card->getNumber());
        jink->setSkillName(objectName());
        jink->addSubcard(card->getId());
        return jink;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "jink";
    }
};

class RendeViewAsSkill:public ViewAsSkill{
public:
    RendeViewAsSkill():ViewAsSkill("rende"){
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(ServerInfo.GameMode == "04_1v3"
           && selected.length() + Self->getMark("rende") >= 2)
           return false;
        else
            return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        RendeCard *rende_card = new RendeCard;
        rende_card->addSubcards(cards);
        return rende_card;
    }
};

class Rende: public PhaseChangeSkill{
public:
    Rende():PhaseChangeSkill("rende"){
        view_as_skill = new RendeViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
                && target->getPhase() == Player::NotActive
                && target->hasUsed("RendeCard");
    }

    virtual bool onPhaseChange(ServerPlayer *target) const{
        target->getRoom()->setPlayerMark(target, "rende", 0);

        return false;
    }
};

class JijiangViewAsSkill:public ZeroCardViewAsSkill{
public:
    JijiangViewAsSkill():ZeroCardViewAsSkill("jijiang$"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->hasLordSkill("jijiang") && Slash::IsAvailable(player);
    }

    virtual const Card *viewAs() const{
        return new JijiangCard;
    }
};

class Jijiang: public TriggerSkill{
public:
    Jijiang():TriggerSkill("jijiang$"){
        events << CardAsked;
        default_choice = "ignore";

        view_as_skill = new JijiangViewAsSkill;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target != NULL && target->hasLordSkill("jijiang");
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *liubei, QVariant &data) const{
        if (liubei == NULL) return false;
        QString pattern = data.toString();
        if(pattern != "slash")
            return false;

        QList<ServerPlayer *> lieges = room->getLieges("shu", liubei);
        if(lieges.isEmpty())
            return false;

        if(!room->askForSkillInvoke(liubei, objectName()))
            return false;

        room->playSkillEffect(objectName(), getEffectIndex(liubei, NULL));

        QVariant tohelp = QVariant::fromValue((PlayerStar)liubei);
        foreach(ServerPlayer *liege, lieges){
            const Card *slash = room->askForCard(liege, "slash", "@jijiang-slash:" + liubei->objectName(), tohelp, NonTrigger);
            if(slash){
                LogMessage log;
                log.type = "#Jijiang";
                log.from = liege;
                log.to << liubei;
                log.arg = "jijiang";
                room->sendLog(log);
                room->provide(slash);
                return true;
            }
        }

        return false;
    }

    virtual int getEffectIndex(const ServerPlayer *player, const Card *) const{
        int r = 1 + qrand() % 2;
        if(player->getGeneralName() == "liushan" || player->getGeneral2Name() == "liushan")
            r += 2;

        return r;
    }
};

class Wusheng:public OneCardViewAsSkill{
public:
    Wusheng():OneCardViewAsSkill("wusheng"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();

        if(!card->isRed())
            return false;

        return EquipCard::CanUseInViewAsSkill(card);
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getCard();
        Card *slash = new Slash(card->getSuit(), card->getNumber());
        slash->addSubcard(card->getId());
        slash->setSkillName(objectName());
        return slash;
    }
};

class Longdan:public OneCardViewAsSkill{
public:
    Longdan():OneCardViewAsSkill("longdan"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *card = to_select->getFilteredCard();

        switch(ClientInstance->getStatus()){
        case Client::Playing:{
                // jink as slash
                return card->inherits("Jink");
            }

        case Client::Responsing:{
                QString pattern = ClientInstance->getPattern();
                if(pattern == "slash")
                    return card->inherits("Jink");
                else if(pattern == "jink")
                    return card->inherits("Slash");
            }

        default:
            return false;
        }
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "jink" || pattern == "slash";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *card = card_item->getFilteredCard();
        if(card->inherits("Slash")){
            Jink *jink = new Jink(card->getSuit(), card->getNumber());
            jink->addSubcard(card);
            jink->setSkillName(objectName());
            return jink;
        }else if(card->inherits("Jink")){
            Slash *slash = new Slash(card->getSuit(), card->getNumber());
            slash->addSubcard(card);
            slash->setSkillName(objectName());
            return slash;
        }else
            return NULL;
    }
};

class Tieji:public SlashBuffSkill{
public:
    Tieji():SlashBuffSkill("tieji"){

    }

    virtual bool buff(const SlashEffectStruct &effect) const{
        ServerPlayer *machao = effect.from;

        Room *room = machao->getRoom();
        if(effect.from->askForSkillInvoke("tieji", QVariant::fromValue(effect))){
            room->playSkillEffect(objectName());

            JudgeStruct judge;
            judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
            judge.good = true;
            judge.reason = objectName();
            judge.who = machao;

            room->judge(judge);
            if(judge.isGood()){
                room->slashResult(effect, NULL);
                return true;
            }
        }

        return false;
    }
};

class Guanxing:public PhaseChangeSkill{
public:
    Guanxing():PhaseChangeSkill("guanxing"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge) const{
        if(zhuge->getPhase() == Player::Start &&
           zhuge->askForSkillInvoke(objectName()))
        {
            Room *room = zhuge->getRoom();
            room->playSkillEffect(objectName());

            int n = qMin(5, room->alivePlayerCount());
            room->askForGuanxing(zhuge, room->getNCards(n, false), false);
        }

        return false;
    }
};

class Kongcheng: public ProhibitSkill{
public:
    Kongcheng():ProhibitSkill("kongcheng"){

    }

    virtual bool isProhibited(const Player *from, const Player *to, const Card *card) const{
        if(card->inherits("Slash") || card->inherits("Duel"))
            return to->isKongcheng();
        else
            return false;
    }
};

class KongchengEffect: public TriggerSkill{
public:
    KongchengEffect():TriggerSkill("#kongcheng-effect"){
        frequency = Compulsory;

        events << CardLostOneTime;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        if(player->isKongcheng()){
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if(move->from_places.contains(Player::Hand))
                room->playSkillEffect("kongcheng");
        }

        return false;
    }
};

class Jizhi:public TriggerSkill{
public:
    Jizhi():TriggerSkill("jizhi"){
        frequency = Frequent;
        events << CardonUse << CardResponsed;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *yueying, QVariant &data) const{
        CardStar card = NULL;
        if(event == CardonUse){
            CardUseStruct use = data.value<CardUseStruct>();
            card = use.card;
        }else if(event == CardResponsed)
            card = data.value<CardStar>();

        if(card->isNDTrick()){
            if(room->askForSkillInvoke(yueying, objectName(), data)){
                room->playSkillEffect(objectName());
                yueying->drawCards(1);
            }
        }

        return false;
    }
};

class Zhiheng:public ViewAsSkill{
public:
    Zhiheng():ViewAsSkill("zhiheng"){

    }

    virtual bool viewFilter(const QList<CardItem *> &, const CardItem *) const{
        return true;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        ZhihengCard *zhiheng_card = new ZhihengCard;
        zhiheng_card->addSubcards(cards);

        return zhiheng_card;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("ZhihengCard");
    }
};

class Jiuyuan: public TriggerSkill{
public:
    Jiuyuan():TriggerSkill("jiuyuan$"){
        events << Dying << AskForPeachesDone << CardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasLordSkill("jiuyuan");
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *sunquan, QVariant &data) const{
        switch(event){
        case Dying: {
                if(Config.SoundEffectMode != "Qsgs")
                    break;
                foreach(ServerPlayer *wu, room->getOtherPlayers(sunquan)){
                    if(wu->getKingdom() == "wu"){
                         room->playSkillEffect("jiuyuan", 1);
                        break;
                    }
                }
                break;
            }

        case CardEffected: {
                CardEffectStruct effect = data.value<CardEffectStruct>();
                if(effect.card->inherits("Peach") && effect.from->getKingdom() == "wu"
                   && sunquan != effect.from && sunquan->hasFlag("dying"))
                {
                    int index;
                    if(Config.SoundEffectMode == "Qsgs")
                        index = effect.from->getGeneral()->isMale() ? 2 : 3;
                    else
                        index = 1;
                    room->playSkillEffect("jiuyuan", index);
                    sunquan->setFlags("jiuyuan");

                    LogMessage log;
                    log.type = "#JiuyuanExtraRecover";
                    log.from = sunquan;
                    log.to << effect.from;
                    log.arg = objectName();
                    room->sendLog(log);

                    RecoverStruct recover;
                    recover.who = effect.from;
                    room->recover(sunquan, recover);

                    room->getThread()->delay(2000);
                }

                break;
            }

        case AskForPeachesDone:{
                if(sunquan->getHp() > 0 && sunquan->hasFlag("jiuyuan")){
                    if(Config.SoundEffectMode == "Qsgs")
                        room->playSkillEffect("jiuyuan", 4);
                    else
                        room->playSkillEffect("jiuyuan", 2);
                }
                sunquan->setFlags("-jiuyuan");

                break;
            }

        default:
            break;
        }

        return false;
    }
};

class Yingzi:public DrawCardsSkill{
public:
    Yingzi():DrawCardsSkill("yingzi"){
        frequency = Frequent;
    }

    virtual int getDrawNum(ServerPlayer *zhouyu, int n) const{
        Room *room = zhouyu->getRoom();
        if(room->askForSkillInvoke(zhouyu, objectName())){
            room->playSkillEffect(objectName());
            return n + 1;
        }else
            return n;
    }
};

class Fanjian:public ZeroCardViewAsSkill{
public:
    Fanjian():ZeroCardViewAsSkill("fanjian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->isKongcheng() && ! player->hasUsed("FanjianCard");
    }

    virtual const Card *viewAs() const{
        return new FanjianCard;
    }
};

class Keji: public TriggerSkill{
public:
    Keji():TriggerSkill("keji"){
        events << CardResponsed;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent , Room*, ServerPlayer *lvmeng, QVariant &data) const{
        CardStar card_star = data.value<CardStar>();
        if(card_star->inherits("Slash"))
            lvmeng->setFlags("keji_use_slash");

        return false;
    }
};

class KejiSkip: public PhaseChangeSkill{
public:
    KejiSkip():PhaseChangeSkill("#keji-skip"){
    }

    virtual int getPriority() const{
        return 3;
    }

    virtual bool onPhaseChange(ServerPlayer *lvmeng) const{
        if(lvmeng->getPhase() == Player::Start){
            lvmeng->setFlags("-keji_use_slash");
        }else if(lvmeng->getPhase() == Player::Discard){
            if(!lvmeng->hasFlag("keji_use_slash") &&
               lvmeng->getSlashCount() == 0 &&
               lvmeng->askForSkillInvoke("keji"))
            {
                lvmeng->getRoom()->playSkillEffect("keji");

                return true;
            }
        }

        return false;
    }
};

class Lianying: public TriggerSkill{
public:
    Lianying():TriggerSkill("lianying"){
        events << CardLostOneTime;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *luxun, QVariant &data) const{
        CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
        if(luxun->isKongcheng() && move->from_places.contains(Player::Hand)){
            if(room->askForSkillInvoke(luxun, objectName(), data)){
                room->playSkillEffect(objectName());
                luxun->drawCards(1);
            }
        }
        return false;
    }
};

class Qixi: public OneCardViewAsSkill{
public:
    Qixi():OneCardViewAsSkill("qixi"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->isBlack();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Dismantlement *dismantlement = new Dismantlement(first->getSuit(), first->getNumber());
        dismantlement->addSubcard(first->getId());
        dismantlement->setSkillName(objectName());
        return dismantlement;
    }
};

class Kurou: public ZeroCardViewAsSkill{
public:
    Kurou():ZeroCardViewAsSkill("kurou"){

    }

    virtual const Card *viewAs() const{
        return new KurouCard;
    }
};

class Guose: public OneCardViewAsSkill{
public:
    Guose():OneCardViewAsSkill("guose"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getCard()->getSuit() == Card::Diamond;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Indulgence *indulgence = new Indulgence(first->getSuit(), first->getNumber());
        indulgence->addSubcard(first->getId());
        indulgence->setSkillName(objectName());
        return indulgence;
    }
};

class LiuliViewAsSkill: public OneCardViewAsSkill{
public:
    LiuliViewAsSkill():OneCardViewAsSkill("liuli"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return pattern == "@@liuli";
    }

    virtual bool viewFilter(const CardItem *) const{
        return true;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        LiuliCard *liuli_card = new LiuliCard;
        liuli_card->addSubcard(card_item->getFilteredCard());

        return liuli_card;
    }
};

class Liuli: public TriggerSkill{
public:
    Liuli():TriggerSkill("liuli"){
        view_as_skill = new LiuliViewAsSkill;

        events << TargetConfirm;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *daqiao, QVariant &data) const{

        CardUseStruct use = data.value<CardUseStruct>();

        if(use.card && use.card->inherits("Slash") && use.to.contains(daqiao) && !daqiao->isNude() && room->alivePlayerCount() > 2){
            QList<ServerPlayer *> players = room->getOtherPlayers(daqiao);
            players.removeOne(use.from);

            bool can_invoke = false;
            foreach(ServerPlayer *p, players){
                if(daqiao->inMyAttackRange(p)){
                    can_invoke = true;
                    break;
                }
            }

            if(can_invoke){
                QString prompt = "@liuli:" + use.from->objectName();
                room->setPlayerFlag(use.from, "slash_source");
                if(room->askForUseCard(daqiao, "@@liuli", prompt)){
                    foreach(ServerPlayer *p, players){
                        if(p->hasFlag("liuli_target")){
                            use.to.insert(use.to.indexOf(daqiao), p);
                            use.to.removeOne(daqiao);

                            data = QVariant::fromValue(use);

                            room->setPlayerFlag(use.from, "-slash_source");
                            room->setPlayerFlag(p, "-liuli_target");
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }
};

class Jieyin: public ViewAsSkill{
public:
    Jieyin():ViewAsSkill("jieyin"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("JieyinCard");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(selected.length() > 2)
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 2)
            return NULL;

        JieyinCard *jieyin_card = new JieyinCard();
        jieyin_card->addSubcards(cards);

        return jieyin_card;
    }
};

class Xiaoji: public TriggerSkill{
public:
    Xiaoji():TriggerSkill("xiaoji"){
        events << CardLostOnePiece;

        frequency = Frequent;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *sunshangxiang, QVariant &data) const{
        if (sunshangxiang == NULL) return false;
        CardMoveStar move = data.value<CardMoveStar>();
        if(move->from_place == Player::Equip){
            if(room->askForSkillInvoke(sunshangxiang, objectName())){
                room->playSkillEffect(objectName());
                sunshangxiang->drawCards(2);
            }
        }

        return false;
    }
};

class Wushuang: public TriggerSkill{
public:
    Wushuang():TriggerSkill("wushuang"){
        events << SlashProceed;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *lvbu, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        room->playSkillEffect(objectName());

        QString slasher = lvbu->objectName();

        const Card *first_jink = NULL, *second_jink = NULL;
        first_jink = room->askForCard(effect.to, "jink", "@wushuang-jink-1:" + slasher, QVariant(), JinkUsed);
        if(first_jink)
            second_jink = room->askForCard(effect.to, "jink", "@wushuang-jink-2:" + slasher, QVariant(), JinkUsed);

        Card *jink = NULL;
        if(first_jink && second_jink){
            jink = new DummyCard;
            jink->addSubcard(first_jink);
            jink->addSubcard(second_jink);
        }

        room->slashResult(effect, jink);

        return true;
    }
};

class Lijian: public OneCardViewAsSkill{
public:
    Lijian():OneCardViewAsSkill("lijian"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("LijianCard");
    }

    virtual bool viewFilter(const CardItem *) const{
        return true;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        LijianCard *lijian_card = new LijianCard;
        lijian_card->addSubcard(card_item->getCard()->getId());

        return lijian_card;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return 0;
    }

};

class Biyue: public PhaseChangeSkill{
public:
    Biyue():PhaseChangeSkill("biyue"){
        frequency = Frequent;
    }

    virtual bool onPhaseChange(ServerPlayer *diaochan) const{
        if(diaochan->getPhase() == Player::Finish){
            Room *room = diaochan->getRoom();
            if(room->askForSkillInvoke(diaochan, objectName())){
                room->playSkillEffect(objectName());
                diaochan->drawCards(1);
            }
        }

        return false;
    }
};

class Tuoqiao: public GameStartSkill{
public:
    Tuoqiao():GameStartSkill("tuoqiao"){
        frequency = Limited;
        default_choice = "SP-Diaochan";
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        if (target == NULL) return false;
        if(Sanguosha->getBanPackages().contains("BGM") && Sanguosha->getBanPackages().contains("sp")) return false;
        bool canInvoke = ServerInfo.GameMode.endsWith("p") || ServerInfo.GameMode.endsWith("pd") ||
                ServerInfo.GameMode.endsWith("pz");
        return GameStartSkill::triggerable(target) && target->getGeneralName() == "diaochan" && canInvoke;
    }

    virtual void onGameStart(ServerPlayer *player) const{
        if(player->askForSkillInvoke(objectName())){
            Room *room = player->getRoom();
            QString choice;
            QStringList ConvertList;
            ConvertList << "SP-Diaochan" << "BGM-Diaochan";
            if(ServerInfo.Enable2ndGeneral)
            {
                QStringList ban_list = Config.value("Banlist/Pairs").toStringList();
                QString ban1(QString("%1+%2").arg("bgm_diaochan").arg(player->getGeneral2Name()));
                QString ban2(QString("%1+%2").arg(player->getGeneral2Name()).arg("bgm_diaochan"));
                if(ban_list.contains(ban1) || ban_list.contains(ban2))
                    ConvertList.removeOne("BGM-Diaochan");
            }
            if(Sanguosha->getBanPackages().contains("sp")) ConvertList.removeOne("SP-Diaochan");
            else if(Sanguosha->getBanPackages().contains("BGM")) ConvertList.removeOne("BGM-Diaochan");

            if(ConvertList.length() == 1)
                choice = ConvertList.first();
            else if(ConvertList.isEmpty())
                return;
            else
                choice = room->askForChoice(player, objectName(), ConvertList.join("+"));

            if(choice == "SP-Diaochan") choice = "sp_diaochan"; else choice = "bgm_diaochan";
            room->transfigure(player, choice, true, false, "diaochan");
        }
    }
};


class Qingnang: public OneCardViewAsSkill{
public:
    Qingnang():OneCardViewAsSkill("qingnang"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("QingnangCard");
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        QingnangCard *qingnang_card = new QingnangCard;
        qingnang_card->addSubcard(card_item->getCard()->getId());

        return qingnang_card;
    }
};

class Jijiu: public OneCardViewAsSkill{
public:
    Jijiu():OneCardViewAsSkill("jijiu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("peach") && player->getPhase() == Player::NotActive;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return to_select->getFilteredCard()->isRed();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *first = card_item->getCard();
        Peach *peach = new Peach(first->getSuit(), first->getNumber());
        peach->addSubcard(first->getId());
        peach->setSkillName(objectName());
        return peach;
    }
};

class Qianxun: public ProhibitSkill{
public:
    Qianxun():ProhibitSkill("qianxun"){

    }

    virtual bool isProhibited(const Player *, const Player *, const Card *card) const{
        return card->inherits("Snatch") || card->inherits("Indulgence");
    }
};

class Mashu: public DistanceSkill{
public:
    Mashu():DistanceSkill("mashu")
    {
    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        if(from->hasSkill(objectName()))
            return -1;
        else
            return 0;
    }
};

void StandardPackage::addGenerals(){
    General *caocao, *zhangliao, *guojia, *xiahoudun, *simayi, *xuchu, *zhenji;

    caocao = new General(this, "caocao$", "wei");
    caocao->addSkill(new Jianxiong);
    caocao->addSkill(new Hujia);

    simayi = new General(this, "simayi", "wei", 3);
    simayi->addSkill(new Fankui);
    simayi->addSkill(new Guicai);

    xiahoudun = new General(this, "xiahoudun", "wei");
    xiahoudun->addSkill(new Ganglie);

    zhangliao = new General(this, "zhangliao", "wei");
    zhangliao->addSkill(new Tuxi);

    xuchu = new General(this, "xuchu", "wei");
    xuchu->addSkill(new Luoyi);
    xuchu->addSkill(new LuoyiBuff);
    related_skills.insertMulti("luoyi", "#luoyi");

    guojia = new General(this, "guojia", "wei", 3);
    guojia->addSkill(new Tiandu);
    guojia->addSkill(new Yiji);

    zhenji = new General(this, "zhenji", "wei", 3, false);
    zhenji->addSkill(new Luoshen);
    zhenji->addSkill(new Qingguo);

    General *liubei, *guanyu, *zhangfei, *zhaoyun, *machao, *zhugeliang, *huangyueying;
    liubei = new General(this, "liubei$", "shu");
    liubei->addSkill(new Rende);
    liubei->addSkill(new Jijiang);

    guanyu = new General(this, "guanyu", "shu");
    guanyu->addSkill(new Wusheng);
    guanyu->addSkill(new TransfigureSkill("guayin", "guanyu", "sp_guanyu"));

    zhangfei = new General(this, "zhangfei", "shu");
    zhangfei->addSkill(new Skill("paoxiao"));
    zhangfei->addSkill(new TransfigureSkill("hengmao", "zhangfei", "bgm_zhangfei"));

    zhugeliang = new General(this, "zhugeliang", "shu", 3);
    zhugeliang->addSkill(new Guanxing);
    zhugeliang->addSkill(new Kongcheng);
    zhugeliang->addSkill(new KongchengEffect);
    related_skills.insertMulti("kongcheng", "#kongcheng-effect");

    zhaoyun = new General(this, "zhaoyun", "shu");
    zhaoyun->addSkill(new Longdan);
    zhaoyun->addSkill(new TransfigureSkill("huantong", "zhaoyun", "bgm_zhaoyun"));

    machao = new General(this, "machao", "shu");
    machao->addSkill(new Tieji);
    machao->addSkill(new Mashu);
    machao->addSkill(new SPConvertSkill("fanqun", "machao", "sp_machao"));

    huangyueying = new General(this, "huangyueying", "shu", 3, false);
    huangyueying->addSkill(new Jizhi);
    huangyueying->addSkill(new Skill("qicai", Skill::Compulsory));

    General *sunquan, *zhouyu, *lvmeng, *luxun, *ganning, *huanggai, *daqiao, *sunshangxiang;
    sunquan = new General(this, "sunquan$", "wu");
    sunquan->addSkill(new Zhiheng);
    sunquan->addSkill(new Jiuyuan);

    ganning = new General(this, "ganning", "wu");
    ganning->addSkill(new Qixi);

    lvmeng = new General(this, "lvmeng", "wu");
    lvmeng->addSkill(new Keji);
    lvmeng->addSkill(new KejiSkip);
    lvmeng->addSkill(new TransfigureSkill("weixue", "lvmeng", "bgm_lvmeng"));
    related_skills.insertMulti("keji", "#keji-skip");

    huanggai = new General(this, "huanggai", "wu");
    huanggai->addSkill(new Kurou);

    zhouyu = new General(this, "zhouyu", "wu", 3);
    zhouyu->addSkill(new Yingzi);
    zhouyu->addSkill(new Fanjian);

    daqiao = new General(this, "daqiao", "wu", 3, false);
    daqiao->addSkill(new Guose);
    daqiao->addSkill(new Liuli);

    luxun = new General(this, "luxun", "wu", 3);
    luxun->addSkill(new Qianxun);
    luxun->addSkill(new Lianying);

    sunshangxiang = new General(this, "sunshangxiang", "wu", 3, false);
    sunshangxiang->addSkill(new SPConvertSkill("chujia", "sunshangxiang", "sp_sunshangxiang"));
    sunshangxiang->addSkill(new Jieyin);
    sunshangxiang->addSkill(new Xiaoji);

    General *lvbu, *huatuo, *diaochan;

    huatuo = new General(this, "huatuo", "qun", 3);
    huatuo->addSkill(new Qingnang);
    huatuo->addSkill(new Jijiu);

    lvbu = new General(this, "lvbu", "qun");
    lvbu->addSkill(new Wushuang);

    diaochan = new General(this, "diaochan", "qun", 3, false);
    diaochan->addSkill(new Lijian);
    diaochan->addSkill(new Biyue);
    diaochan->addSkill(new Tuoqiao);

    // for skill cards
    addMetaObject<ZhihengCard>();
    addMetaObject<RendeCard>();
    addMetaObject<TuxiCard>();
    addMetaObject<JieyinCard>();
    addMetaObject<KurouCard>();
    addMetaObject<LijianCard>();
    addMetaObject<FanjianCard>();
    addMetaObject<GuicaiCard>();
    addMetaObject<QingnangCard>();
    addMetaObject<LiuliCard>();
    addMetaObject<JijiangCard>();
}

class Zhiba: public Zhiheng{
public:
    Zhiba(){
        setObjectName("zhiba");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return player->usedTimes("ZhihengCard") < (player->getLostHp() + 1);
    }
};

class SuperGuanxing: public Guanxing{
public:
    SuperGuanxing():Guanxing(){
        setObjectName("super_guanxing");
    }

    virtual bool onPhaseChange(ServerPlayer *zhuge) const{
        if(zhuge->getPhase() == Player::Start &&
           zhuge->askForSkillInvoke(objectName()))
        {
            Room *room = zhuge->getRoom();
            room->playSkillEffect("guanxing");

            room->askForGuanxing(zhuge, room->getNCards(5, false), false);
        }

        return false;
    }
};

YihunCard::YihunCard(){
    once = true;
}

bool YihunCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select == Self)
        return false;
    if(targets.isEmpty())
        return true;
    else if(targets.length() == 1)
        return to_select->getHp() != targets.first()->getHp();
    else
        return false;
}

bool YihunCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

void YihunCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    QList<ServerPlayer *> selecteds = targets;
    ServerPlayer *from = selecteds.first()->getHp() < selecteds.last()->getHp() ? selecteds.takeFirst() : selecteds.takeLast();
    ServerPlayer *to = selecteds.takeFirst();
    int maxhp1 = from->getMaxHp();
    int maxhp2 = to->getMaxHp();
    int hp1 = from->getHp();
    int hp2 = to->getHp();
    room->setPlayerProperty(from, "maxhp", maxhp2);
    room->setPlayerProperty(to, "maxhp", maxhp1);
    room->setPlayerProperty(from, "hp", hp2);
    room->setPlayerProperty(to, "hp", hp1);
    RecoverStruct rev;
    rev.recover = 1;
    if ((hp2-hp1) > 1)
        room->recover(source, rev);
}

class Yihun: public ZeroCardViewAsSkill{
public:
    Yihun():ZeroCardViewAsSkill("yihun"){
    }

    virtual const Card *viewAs() const{
        return new YihunCard;
    }

protected:
    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->hasUsed("YihunCard");
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return false;
    }
};

class Shenjia: public TriggerSkill{
public:
    Shenjia():TriggerSkill("shenjia"){
        events << Predamaged;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target);
    }

    virtual bool trigger(TriggerEvent , Room *room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        LogMessage log;
        log.type = "#Shenjia";
        log.from = player;
        log.arg = QString::number(damage.damage);
        log.arg2 = objectName();
        room->sendLog(log);
        room->loseHp(player);
        return true;
    }
};

class Bumie: public TriggerSkill{
public:
    Bumie():TriggerSkill("bumie"){
        events << Dying;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->faceUp();
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        ServerPlayer *yoka = room->findPlayerBySkillName(objectName());
        DyingStruct dying_data = data.value<DyingStruct>();
        if(dying_data.who != yoka)
            return false;
        if(!yoka || !room->askForSkillInvoke(yoka, objectName(), data))
            return false;
        ServerPlayer *target = room->askForPlayerChosen(player, room->getOtherPlayers(player), objectName());
        int hp = target->getHp();
        RecoverStruct rev;
        rev.recover = qMin(hp,player->getMaxHp()-player->getHp());
        room->recover(player, rev);
        player->turnOver();
        room->loseHp(target,rev.recover);
        return false;
    }
};

TestPackage::TestPackage()
    :Package("test")
{
    // for test only
    General *zhiba_sunquan = new General(this, "zhibasunquan$", "wu", 4, true, true);
    zhiba_sunquan->addSkill(new Zhiba);
    zhiba_sunquan->addSkill("jiuyuan");

    General *wuxing_zhuge = new General(this, "wuxingzhuge", "shu", 3, true, true);
    wuxing_zhuge->addSkill(new SuperGuanxing);
    wuxing_zhuge->addSkill("kongcheng");
    wuxing_zhuge->addSkill("#kongcheng-effect");

    General *yoka = new General(this, "yoka", "god", 3, true, true);
    yoka->addSkill(new Yihun);
    yoka->addSkill(new Shenjia);
    yoka->addSkill(new Bumie);

    new General(this, "sujiang", "god", 5, true, true);
    new General(this, "sujiangf", "god", 5, false, true);

    new General(this, "anjiang", "god", 4,true, true, true);
    addMetaObject<YihunCard>();
}

ADD_PACKAGE(Test)
