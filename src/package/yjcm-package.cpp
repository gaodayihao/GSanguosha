#include "yjcm-package.h"
#include "skill.h"
#include "standard.h"
#include "maneuvering.h"
#include "clientplayer.h"
#include "carditem.h"
#include "engine.h"
#include "ai.h"
#include "general.h"
#include "settings.h"

class Yizhong: public TriggerSkill{
public:
    Yizhong():TriggerSkill("yizhong"){
        events << SlashEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getArmor() == NULL;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if(effect.slash->isBlack()){
            player->getRoom()->playSkillEffect(objectName());

            LogMessage log;
            log.type = "#SkillNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();

            player->getRoom()->sendLog(log);

            player->getRoom()->setEmotion(player, QString("skill/%1").arg(objectName()));

            return true;
        }

        return false;
    }
};

class Luoying: public TriggerSkill{
public:
    Luoying():TriggerSkill("luoying"){
        events << CardDiscarded << CardUsed << FinishJudge;
        frequency = Frequent;
        default_choice = "no";
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return ! target->hasSkill(objectName());
    }

    QList<const Card *> getClubs(const Card *card) const{
        QList<const Card *> clubs;

        if(!card->isVirtualCard()){
            if(card->getSuit() == Card::Club)
                clubs << card;

            return clubs;
        }

        foreach(int card_id, card->getSubcards()){
            const Card *c = Sanguosha->getCard(card_id);
            if(c->getSuit() == Card::Club)
                clubs << c;
        }

        return clubs;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        QList<const Card *> clubs;

        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            const SkillCard *skill_card = qobject_cast<const SkillCard *>(use.card);
            if(skill_card &&
                    skill_card->subcardsLength() > 0 &&
                    skill_card->willThrow() &&
                    !skill_card->isOwnerDiscarded()){
                clubs = getClubs(skill_card);
            }
        }else if(event == CardDiscarded){
            const Card *card = data.value<CardStar>();

            clubs = getClubs(card);
        }else if(event == FinishJudge){
            JudgeStar judge = data.value<JudgeStar>();
            if(room->getCardPlace(judge->card->getEffectiveId()) == Player::DiscardPile
               && judge->card->getSuit() == Card::Club)
               clubs << judge->card;
        }

        ServerPlayer *caozhi = room->findPlayerBySkillName(objectName());
        foreach(const Card* card, clubs)
            if(card->objectName() == "shit")
                if(caozhi && room->askForChoice(caozhi, objectName(), "yes+no") == "no")
                    clubs.removeOne(card);

        if(clubs.isEmpty())
            return false;

        if(caozhi && caozhi->askForSkillInvoke(objectName(), data)){
            if(Config.SoundEffectMode == "Qsgs"){
                if(player->getGeneralName() == "zhenji")
                    room->playSkillEffect("luoying", 2);
                else
                    room->playSkillEffect("luoying", 1);
            }
            else
                room->playSkillEffect("luoying");

            room->getThread()->delay(650);

            foreach(const Card *club, clubs)
                caozhi->obtainCard(club);
        }

        return false;
    }
};

class Jiushi: public ZeroCardViewAsSkill{
public:
    Jiushi():ZeroCardViewAsSkill("jiushi"){
        Analeptic *analeptic = new Analeptic(Card::NoSuit, 0);
        analeptic->setSkillName("jiushi");

        this->analeptic = analeptic;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Analeptic::IsAvailable(player) && player->faceUp();
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern.contains("analeptic") && player->faceUp();
    }

    virtual const Card *viewAs() const{
        return analeptic;
    }

    virtual int getEffectIndex(const ServerPlayer *, const Card *) const{
        return qrand() % 2 + 1;
    }

private:
    const Analeptic *analeptic;
};

class JiushiFlip: public TriggerSkill{
public:
    JiushiFlip():TriggerSkill("#jiushi-flip"){
        events << CardUsed << Predamaged << Damaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        if(event == CardUsed){
            CardUseStruct use = data.value<CardUseStruct>();
            if(use.card->getSkillName() == "jiushi")
                player->turnOver();
        }else if(event == Predamaged){
            player->tag["PredamagedFace"] = player->faceUp();
        }else if(event == Damaged){
            bool faceup = player->tag.value("PredamagedFace").toBool();
            if(!faceup && player->askForSkillInvoke("jiushi", data)){
                player->getRoom()->playSkillEffect("jiushi", 3);
                player->turnOver();
            }
        }

        return false;
    }
};

class Wuyan: public TriggerSkill{
public:
    Wuyan():TriggerSkill("wuyan"){
        events << DamagedProceed << Predamage;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        Room *room = player->getRoom();
        if(damage.card && damage.card->getTypeId() == Card::Trick){
            if(event == DamagedProceed){
                LogMessage log;
                log.type = "#WuyanGood";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->playSkillEffect(objectName());
                room->setEmotion(player, "skill/wuyan");
            }

            if(event == Predamage){
                LogMessage log;
                log.type = "#WuyanBad";
                log.from = player;
                log.arg = damage.card->objectName();
                log.arg2 = objectName();
                room->sendLog(log);
                room->playSkillEffect(objectName());
                room->setEmotion(player, "skill/wuyan");
            }

            return true;
        }

        return false;
    }
};

JujianCard::JujianCard(){
    mute = true;
}

bool JujianCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select == Self)
        return false;

    return true;
}

void JujianCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();

    QStringList choices;
    QString choice;
    choices << "draw";
    if(effect.to->isWounded())
        choices << "recover";
    if(effect.to->isChained() || !effect.to->faceUp())
        choices << "reset";

    if(choices.length() == 1)
        choice = "draw";
    else
        choice = room->askForChoice(effect.to, "jujian", choices.join("+"));

    if(choice == "reset"){
        room->setPlayerProperty(effect.to, "chained", false);
        if(!effect.to->faceUp())
            effect.to->turnOver();
        room->playSkillEffect("jujian", 1);
    }
    else if(choice == "recover"){
        RecoverStruct recover;
        recover.who = effect.from;
        room->recover(effect.to, recover);
        room->playSkillEffect("jujian", 1);
    }
    else{
        effect.to->drawCards(2);
        room->playSkillEffect("jujian", 2);
    }
}

class JujianViewAsSkill: public OneCardViewAsSkill{
public:
    JujianViewAsSkill():OneCardViewAsSkill("jujian"){
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        JujianCard *card = new JujianCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->getCard()->inherits("BasicCard");
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@jujian";
    }
};

class Jujian:public PhaseChangeSkill{
public:
    Jujian():PhaseChangeSkill("jujian"){
        view_as_skill = new JujianViewAsSkill;
    }

    virtual QString getDefaultChoice(ServerPlayer *) const{
        return "draw";
    }

    virtual bool onPhaseChange(ServerPlayer *xushu) const{
        Room *room = xushu->getRoom();
        if(xushu->getPhase() == Player::Finish && !xushu->isKongcheng()){
            room->askForUseCard(xushu, "@@jujian", "@jujian-card");
        }
        return false;
    }
};


class Enyuan: public TriggerSkill{
public:
    Enyuan():TriggerSkill("enyuan"){
        events << CardGotOneTime << Damaged;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(event == CardGotOneTime){
            MoveNCardsStruct moveNcards = player->tag["MoveNCards"].value<MoveNCardsStruct>();
            if(moveNcards.from && moveNcards.count >= 2 && !player->hasFlag("moveNcards")
                    && room->askForSkillInvoke(player,objectName(),data))
            {
                room->setPlayerFlag(player, "moveNcards");
                room->drawCards(moveNcards.from,1);
                room->playSkillEffect(objectName(), qrand() % 2 + 1);
                return false;
            }
            CardsMoveStar move = data.value<CardsMoveStar>();
            if (move->from != NULL && move->card_ids.size() >= 2 && !player->hasFlag("moveNcards")
                    && room->askForSkillInvoke(player,objectName(),data)){
                room->drawCards((ServerPlayer*)move->from,1);
                room->playSkillEffect(objectName(), qrand() % 2 + 1);
            }
            room->setPlayerFlag(player, "-moveNcards");
        }else if(event == Damaged){
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if(source && source != player){
                for(int i = 0; i < damage.damage; i++){
                    if(!room->askForSkillInvoke(player,objectName(), data))
                        continue;
                    room->playSkillEffect(objectName(), qrand() % 2 + 3);

                    const Card *card = room->askForCard(source, ".", "@enyuan", QVariant(), NonTrigger);
                    if(card){
                        player->obtainCard(card);
                    }else{
                        room->loseHp(source);
                    }
                }
            }
        }

        return false;
    }
};

XuanhuoCard::XuanhuoCard(){
}

bool XuanhuoCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(to_select == Self)
        return false;

    return true;
}

void XuanhuoCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    QList<ServerPlayer *> targets;
    QString choice;
    foreach(ServerPlayer *victim, room->getOtherPlayers(effect.to)){
        if(effect.to->canSlash(victim))
            targets << victim;
    }
    room->drawCards(effect.to,2);
    if(targets.isEmpty())
        choice = "give";
    else
        choice = room->askForChoice(effect.to, "xuanhuo", "slash+give");
    if(choice == "slash"){
        ServerPlayer *victim = room->askForPlayerChosen(effect.from, targets, "xuanhuo");
        QString prompt = QString("xuanhuo-slash:%1:%2")
                .arg(effect.from->objectName()).arg(victim->objectName());
        const Card *slash = room->askForCard(effect.to, "slash", prompt, QVariant(), NonTrigger);
        if(slash){
            CardUseStruct use;
            use.card = slash;
            use.from = effect.to;
            use.to << victim;
            room->useCard(use);
        }
        else
        {
            if (!effect.to->isNude())
                room->selectSomeCardToMove(effect.to, effect.from, effect.from, qMin(effect.to->getCardCount(true),2), "he", "xuanhuo");
        }
    }
    else
    {
        if (!effect.to->isNude())
            room->selectSomeCardToMove(effect.to, effect.from, effect.from, qMin(effect.to->getCardCount(true),2), "he", "xuanhuo");
    }
}

class XuanhuoViewAsSkill: public ZeroCardViewAsSkill{
public:
    XuanhuoViewAsSkill():ZeroCardViewAsSkill("xuanhuo"){
    }

    virtual const Card *viewAs() const{
        return new XuanhuoCard;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const{
        return  pattern == "@@xuanhuo";
    }
};

class Xuanhuo:public PhaseChangeSkill{
public:
    Xuanhuo():PhaseChangeSkill("xuanhuo"){
        view_as_skill = new XuanhuoViewAsSkill;
    }

    virtual QString getDefaultChoice(ServerPlayer *) const{
        return "give";
    }

    virtual bool onPhaseChange(ServerPlayer *fazheng) const{
        Room *room = fazheng->getRoom();
        if(fazheng->getPhase() == Player::Draw){
            if(room->askForUseCard(fazheng, "@@xuanhuo", "@xuanhuo-card"))
                return true;
        }
        return false;
    }
};

class Huilei: public TriggerSkill{
public:
    Huilei():TriggerSkill("huilei"){
        events << Death;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStar damage = data.value<DamageStar>();
        ServerPlayer *killer = damage ? damage->from : NULL;
        if(killer){
            Room *room = player->getRoom();

            LogMessage log;
            log.type = "#HuileiThrow";
            log.from = player;
            log.to << killer;
            log.arg = objectName();
            room->sendLog(log);

            killer->throwAllHandCards();
            killer->throwAllEquips();

            QString killer_name = killer->getGeneralName();
            if(killer_name == "zhugeliang" || killer_name == "wolong" || killer_name == "shenzhugeliang")
                room->playSkillEffect(objectName(), 1);
            else
                room->playSkillEffect(objectName(), 2);
        }

        return false;
    }
};

class Xuanfeng: public TriggerSkill{
public:
    Xuanfeng():TriggerSkill("xuanfeng"){
        events << CardLostOnePiece << CardLostOneTime << PhaseChange;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *lingtong, QVariant &data) const{
        if(event == CardLostOnePiece){
            CardMoveStar move = data.value<CardMoveStar>();
            if(move->to_place == Player::DiscardPile && lingtong->getPhase() == Player::Discard){
                lingtong->addMark("xuanfeng");
                if(lingtong->getMark("xuanfeng") == 2){
                    lingtong->tag["InvokeXuanfeng"] = true;
                }
            }
        }
        else if(event == PhaseChange){
            lingtong->setMark("xuanfeng", 0);
        }
        else if(event == CardLostOneTime)
        {
            CardsMoveStar move = data.value<CardsMoveStar>();
            if(move->from_place == Player::Equip ||
                    lingtong->tag.value("InvokeXuanfeng", false).toBool())
            {
                lingtong->tag.remove("InvokeXuanfeng");
                Room *room = lingtong->getRoom();

                QList<ServerPlayer *> targets;
                foreach(ServerPlayer *sp,room->getOtherPlayers(lingtong))
                    if(!sp->isNude())
                        targets << sp;
                if(targets.isEmpty())
                    return false;

                QString choice = room->askForChoice(lingtong, objectName(), "discard+nothing");

                if(choice == "discard")
                {
                    room->playSkillEffect(objectName());

                    ServerPlayer *target;
                    int card_id = -1;
                    for(int i = 0;i < 2;i ++){
                        if(i == 1)
                            if(!room->askForSkillInvoke(lingtong, objectName()))
                                break;
                        target = room->askForPlayerChosen(lingtong, targets, "xuanfeng");
                        if(target){
                            card_id = room->askForCardChosen(lingtong, target, "he", "xuanfeng");
                            room->throwCard(card_id, target);
                            if(target->isNude())
                                targets.removeOne(target);
                        }
                    }
                }
            }
        }
        return false;
    }
};

class Pojun: public TriggerSkill{
public:
    Pojun():TriggerSkill("pojun"){
        events << Damage;
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        if(damage.to->isDead())
            return false;

        if(damage.card && damage.card->inherits("Slash") &&
           player->askForSkillInvoke(objectName(), data))
        {
            player->getRoom()->playSkillEffect(objectName());

            int x = qMin(5, damage.to->getHp());
            damage.to->drawCards(x);
            damage.to->turnOver();
        }

        return false;
    }
};

XianzhenCard::XianzhenCard(){
    once = true;
    will_throw = false;
}

bool XianzhenCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    return targets.isEmpty() && to_select != Self && ! to_select->isKongcheng();
}

void XianzhenCard::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();

    const Card *card = Sanguosha->getCard(subcards.first());
    if(effect.from->pindian(effect.to, "xianzhen", card)){
        PlayerStar target = effect.to;
        effect.from->tag["XianzhenTarget"] = QVariant::fromValue(target);
        room->setPlayerFlag(effect.from, "xianzhen_success");
        room->setFixedDistance(effect.from, effect.to, 1);
    }else{
        room->setPlayerFlag(effect.from, "xianzhen_failed");
    }
}

XianzhenSlashCard::XianzhenSlashCard(){
    target_fixed = true;
    can_jilei = true;
}

void XianzhenSlashCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *target = card_use.from->tag["XianzhenTarget"].value<PlayerStar>();
    if(target == NULL || target->isDead())
        return;

    if(!card_use.from->canSlash(target, false))
        return;

    const Card *slash = room->askForCard(card_use.from, "slash", "@xianzhen-slash-Use", QVariant(), NonTrigger);
    if(slash){
        CardUseStruct use;
        use.card = slash;
        use.from = card_use.from;
        use.to << target;
        room->useCard(use);
    }
}

class XianzhenViewAsSkill: public ViewAsSkill{
public:
    XianzhenViewAsSkill():ViewAsSkill(""){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("XianzhenCard") || player->hasFlag("xianzhen_success");
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(!selected.isEmpty())
            return false;

        if(Self->hasUsed("XianzhenCard"))
            return false;

        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(! Self->hasUsed("XianzhenCard")){
            if(cards.length() != 1)
                return NULL;

            XianzhenCard *card = new XianzhenCard;
            card->addSubcards(cards);
            return card;
        }else if(Self->hasFlag("xianzhen_success")){
            if(!cards.isEmpty())
                return NULL;

            return new XianzhenSlashCard;
        }else
            return NULL;
    }
};

class Xianzhen: public TriggerSkill{
public:
    Xianzhen():TriggerSkill("xianzhen"){
        view_as_skill = new XianzhenViewAsSkill;

        events << PhaseChange << CardUsed << CardFinished;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *gaoshun, QVariant &data) const{
        ServerPlayer *target = gaoshun->tag["XianzhenTarget"].value<PlayerStar>();

        if(event == PhaseChange){
            if(gaoshun->getPhase() == Player::Finish && target){
                Room *room = gaoshun->getRoom();
                room->setFixedDistance(gaoshun, target, -1);
                gaoshun->tag.remove("XianzhenTarget");
                target->removeMark("qinggang");
            }
        }else{
            CardUseStruct use = data.value<CardUseStruct>();

            if(target && use.to.contains(target)){
                if(event == CardUsed)
                    target->addMark("qinggang");
                else
                    target->removeMark("qinggang");
            }
        }

        return false;
    }
};

class Jiejiu: public FilterSkill{
public:
    Jiejiu():FilterSkill("jiejiu"){

    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped() && to_select->getCard()->objectName() == "analeptic";
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        const Card *c = card_item->getCard();
        Slash *slash = new Slash(c->getSuit(), c->getNumber());
        slash->setSkillName(objectName());
        slash->addSubcard(card_item->getCard());

        return slash;
    }
};

MingceCard::MingceCard(){
    once = true;
    will_throw = false;
}

void MingceCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);

    Room *room = effect.to->getRoom();
    QString choice = room->askForChoice(effect.to, "mingce", "use+draw");
    if(choice == "use"){
        QList<ServerPlayer *> players = room->getOtherPlayers(effect.to), targets;
        foreach(ServerPlayer *player, players){
            if(effect.to->canSlash(player))
                targets << player;
        }

        if(!targets.isEmpty()){
            ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "mingce");
            room->cardEffect(new Slash(Card::NoSuit, 0), effect.to, target);
        }
    }else if(choice == "draw"){
        effect.to->drawCards(1, true);
    }
}

class Mingce: public OneCardViewAsSkill{
public:
    Mingce():OneCardViewAsSkill("mingce"){
        default_choice = "draw";
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("MingceCard");
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        const Card *c = to_select->getCard();
        return c->getTypeId() == Card::Equip || c->inherits("Slash");
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        MingceCard *card = new MingceCard;
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class ZhichiClear: public TriggerSkill{
public:
    ZhichiClear():TriggerSkill("#zhichi-clear"){
        events << PhaseChange;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent, ServerPlayer *player, QVariant &) const{
        if(player->getPhase() == Player::NotActive)
            player->getRoom()->setTag("Zhichi", QVariant());

        return false;
    }
};

class Zhichi: public TriggerSkill{
public:
    Zhichi():TriggerSkill("zhichi"){
        events << Damaged << CardEffected;

        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();

        if(player->getPhase() != Player::NotActive)
            return false;

        if(event == Damaged){
            room->setTag("Zhichi", player->objectName());

            room->playSkillEffect(objectName());

            LogMessage log;
            log.type = "#ZhichiDamaged";
            log.from = player;
            room->sendLog(log);

        }else if(event == CardEffected){
            if(room->getTag("Zhichi").toString() != player->objectName())
                return false;

            CardEffectStruct effect = data.value<CardEffectStruct>();
            if(effect.card->inherits("Slash") || effect.card->getTypeId() == Card::Trick){
                LogMessage log;
                log.type = "#ZhichiAvoid";
                log.from = player;
                log.arg = objectName();
                room->sendLog(log);

                return true;
            }
        }

        return false;
    }
};

GanluCard::GanluCard(){
    once = true;
}

void GanluCard::swapEquip(ServerPlayer *first, ServerPlayer *second) const{

    Room *room = first->getRoom();

    if(room->getFront(first, second) != first)
        qSwap(first, second);

    QList<int> equips1, equips2;
    foreach(const Card *equip, first->getEquips())
        equips1.append(equip->getId());
    foreach(const Card *equip, second->getEquips())
        equips2.append(equip->getId());

    QList<CardsMoveStruct> exchangeMove;
    CardsMoveStruct move1;
    move1.card_ids = equips1;
    move1.to = second;
    move1.to_place = Player::Equip;
    CardsMoveStruct move2;
    move2.card_ids = equips2;
    move2.to = first;
    move2.to_place = Player::Equip;
    exchangeMove.push_back(move2);
    exchangeMove.push_back(move1);
    room->moveCards(exchangeMove, false);
}

bool GanluCard::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return targets.length() == 2;
}

bool GanluCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    switch(targets.length()){
    case 0: return true;
    case 1: {
            int n1 = targets.first()->getEquips().length();
            int n2 = to_select->getEquips().length();
            return qAbs(n1-n2) <= Self->getLostHp();
        }

    default:
        return false;
    }
}

void GanluCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    ServerPlayer *first = targets.first();
    ServerPlayer *second = targets.at(1);

    swapEquip(first, second);

    LogMessage log;
    log.type = "#GanluSwap";
    log.from = source;
    log.to = targets;
    room->sendLog(log);
}

class Ganlu: public ZeroCardViewAsSkill{
public:
    Ganlu():ZeroCardViewAsSkill("ganlu"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("GanluCard");
    }

    virtual const Card *viewAs() const{
        return new GanluCard;
    }
};

class Buyi: public TriggerSkill{
public:
    Buyi():TriggerSkill("buyi"){
        events << Dying;
    }

    virtual bool triggerable(const ServerPlayer *player) const{
        return !player->isKongcheng();
    }

    virtual bool trigger(TriggerEvent , ServerPlayer *player, QVariant &data) const{
        Room *room = player->getRoom();
        QList<ServerPlayer *> wuguots = room->findPlayersBySkillName(objectName());
        foreach(ServerPlayer *wuguotai, wuguots){
            if(player->getHp() < 1 && wuguotai->askForSkillInvoke(objectName(), data)){
                const Card *card = NULL;
                if(player == wuguotai)
                    card = room->askForCardShow(player, wuguotai, objectName());
                else{
                    int card_id = room->askForCardChosen(wuguotai, player, "h", "buyi");
                    card = Sanguosha->getCard(card_id);
                }

                room->showCard(player, card->getEffectiveId());

                if(card->getTypeId() != Card::Basic){
                    room->throwCard(card, player);

                    room->playSkillEffect(objectName());

                    RecoverStruct recover;
                    recover.who = wuguotai;
                    room->recover(player, recover);
                }
            }
        }
        return false;
    }
};

XinzhanCard::XinzhanCard(){
    target_fixed = true;
    once = true;
}

void XinzhanCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    QList<int> cards = room->getNCards(3, false), left;
    left = cards;

    QList<int> hearts;
    foreach(int card_id, cards){
        const Card *card = Sanguosha->getCard(card_id);
        if(card->getSuit() == Card::Heart)
            hearts << card_id;
    }

    if(!hearts.isEmpty()){
        room->fillAG(cards, source);

        while(!hearts.isEmpty()){
            int card_id = room->askForAG(source, hearts, true, "xinzhan");
            if(card_id == -1)
                break;

            if(!hearts.contains(card_id))
                continue;

            hearts.removeOne(card_id);
            left.removeOne(card_id);

            source->obtainCard(Sanguosha->getCard(card_id));
            room->showCard(source, card_id);
        }

        source->invoke("clearAG");
    }

    if(!left.isEmpty())
        room->askForGuanxing(source, left, true);
 }

class Xinzhan: public ZeroCardViewAsSkill{
public:
    Xinzhan():ZeroCardViewAsSkill("xinzhan"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("XinzhanCard") && player->getHandcardNum() > player->getMaxHp();
    }

    virtual const Card *viewAs() const{
        return new XinzhanCard;
    }
};

class Quanji: public MasochismSkill{
public:
    Quanji():MasochismSkill("quanji"){
        frequency = Frequent;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        Room *room = zhonghui->getRoom();
        int i, x = damage.damage;
        for(i = 0; i < x; i++){
            if(!zhonghui->askForSkillInvoke(objectName()))
                break;

            room->playSkillEffect(objectName());

            zhonghui->drawCards(1, false);
            if(!zhonghui->isKongcheng()){
                int card_id;
                if(zhonghui->handCards().length() == 1){
                    room->getThread()->delay(500);
                    card_id = zhonghui->handCards().first();
                }
                else
                    card_id = room->askForExchange(zhonghui, objectName(), 1, false, "QuanjiPush")->getSubcards().first();
                zhonghui->addToPile("power", card_id);
            }
        }
    }
};

class Zili: public PhaseChangeSkill{
public:
    Zili():PhaseChangeSkill("zili"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
                && target->getMark("zili") == 0
                && target->getPile("power").length() >= 3
                && target->getPhase() == Player::Start;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();

        LogMessage log;
        log.type = "#ZiliWake";
        log.from = zhonghui;
        log.arg = QString::number(zhonghui->getPile("power").length());
        log.arg2 = objectName();
        room->sendLog(log);

        room->playSkillEffect("zili");
//        room->broadcastInvoke("animate", "lightbox:$zili:4000");
//        room->getThread()->delay(4000);

        room->loseMaxHp(zhonghui);

        if(zhonghui->isWounded())
            if(room->askForChoice(zhonghui, objectName(), "recover+draw") == "recover"){
                RecoverStruct recover;
                recover.who = zhonghui;
                room->recover(zhonghui, recover);
            }else
                room->drawCards(zhonghui, 2);
        else
            room->drawCards(zhonghui, 2);

        room->setPlayerMark(zhonghui, "zili", 1);
        room->acquireSkill(zhonghui, "paiyi");

        return false;
    }
};

PaiyiCard::PaiyiCard(){
    once = true;
    mute = true;
}

bool PaiyiCard::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    return true;
}

void PaiyiCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *zhonghui = card_use.from;
    ServerPlayer *target = card_use.to.first();
    QList<int> powers = zhonghui->getPile("power");

    int card_id;
    if(powers.length() == 1)
        card_id = powers.first();
    else{
        room->fillAG(powers, zhonghui);
        card_id = room->askForAG(zhonghui, powers, true, "paiyi");
        zhonghui->invoke("clearAG");

        if(card_id == -1)
            return;
    }

    room->throwCard(card_id);

    LogMessage log;
    log.from = card_use.from;
    log.to = card_use.to;
    log.type = "#UseCard";
    log.card_str = card_use.card->toString();
    room->sendLog(log);

    room->playSkillEffect("paiyi");

    room->drawCards(target, 2);
    if(target->getHandcardNum() > zhonghui->getHandcardNum()){
        DamageStruct damage;
        damage.card = NULL;
        damage.from = zhonghui;
        damage.to = target;

        room->damage(damage);
    }
}

class Paiyi: public ZeroCardViewAsSkill{
public:
    Paiyi():ZeroCardViewAsSkill("paiyi"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("power").isEmpty() && !player->hasUsed("PaiyiCard");
    }

    virtual const Card *viewAs() const{
        return new PaiyiCard;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

YJCMPackage::YJCMPackage():Package("YJCM"){
    General *caozhi = new General(this, "caozhi", "wei", 3);
    caozhi->addSkill(new Luoying);
    caozhi->addSkill(new Jiushi);
    caozhi->addSkill(new JiushiFlip);

    related_skills.insertMulti("jiushi", "#jiushi-flip");

    General *yujin = new General(this, "yujin", "wei");
    yujin->addSkill(new Yizhong);

    General *xushu = new General(this, "xushu", "shu", 3);
    xushu->addSkill(new Wuyan);
    xushu->addSkill(new Jujian);

    General *masu = new General(this, "masu", "shu", 3);
    masu->addSkill(new Xinzhan);
    masu->addSkill(new Huilei);

    General *fazheng = new General(this, "fazheng", "shu", 3);
    fazheng->addSkill(new Enyuan);
    fazheng->addSkill(new Xuanhuo);

    General *lingtong = new General(this, "lingtong", "wu");
    lingtong->addSkill(new Xuanfeng);

    General *xusheng = new General(this, "xusheng", "wu");
    xusheng->addSkill(new Pojun);

    General *wuguotai = new General(this, "wuguotai", "wu", 3, false);
    wuguotai->addSkill(new Ganlu);
    wuguotai->addSkill(new Buyi);

    General *chengong = new General(this, "chengong", "qun", 3);
    chengong->addSkill(new Zhichi);
    chengong->addSkill(new ZhichiClear);
    chengong->addSkill(new Mingce);

    related_skills.insertMulti("zhichi", "#zhichi-clear");

    General *gaoshun = new General(this, "gaoshun", "qun");
    gaoshun->addSkill(new Xianzhen);
    gaoshun->addSkill(new Jiejiu);

    General *zhonghui = new General(this, "zhonghui", "wei");
    zhonghui->addSkill(new Quanji);
    zhonghui->addSkill(new Zili);
    zhonghui->addRelateSkill("paiyi");

    addMetaObject<MingceCard>();
    addMetaObject<GanluCard>();
    addMetaObject<XianzhenCard>();
    addMetaObject<XianzhenSlashCard>();
    addMetaObject<JujianCard>();
    addMetaObject<XuanhuoCard>();
    addMetaObject<XinzhanCard>();
    addMetaObject<PaiyiCard>();

    skills << new Paiyi;
}

ADD_PACKAGE(YJCM)
