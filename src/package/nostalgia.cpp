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

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
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
    Card::Suit suit = room->askForSuit(target, "fanjian");

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
    NosFanjian():ZeroCardViewAsSkill("nosfanjian"){

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
    NosLieren():TriggerSkill("noslieren"){
        events << PostDamageCaused;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *zhurong, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();
        ServerPlayer *target = damage.to;
        if(damage.card && damage.card->inherits("Slash") && !zhurong->isKongcheng()
            && !target->isKongcheng() && target != zhurong){
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

// old yjcm's generals

class NosWuyan: public TriggerSkill{
public:
    NosWuyan():TriggerSkill("noswuyan"){
        events << CardEffect << CardEffected;
        frequency = Compulsory;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return true;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *, QVariant &data) const{
        CardEffectStruct effect = data.value<CardEffectStruct>();

        if(effect.to == effect.from)
            return false;

        if(effect.card->getTypeId() == Card::Trick){

            if((effect.from && effect.from->hasSkill(objectName()))){
                LogMessage log;
                log.type = "#WuyanBaD";
                log.from = effect.from;
                log.to << effect.to;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();

                room->sendLog(log);

                room->playSkillEffect("wuyan");
                room->setEmotion(effect.from, "skill/wuyan");

                return true;
            }

            if(effect.to->hasSkill(objectName()) && effect.from){
                LogMessage log;
                log.type = "#WuyanGooD";
                log.from = effect.to;
                log.to << effect.from;
                log.arg = effect.card->objectName();
                log.arg2 = objectName();

                room->sendLog(log);

                room->playSkillEffect("wuyan");
                room->setEmotion(effect.to, "skill/wuyan");

                return true;
            }
        }

        return false;
    }
};

NosJujianCard::NosJujianCard(){
    once = true;
    owner_discarded = true;
    mute = true;
}

void NosJujianCard::onEffect(const CardEffectStruct &effect) const{
    int n = subcardsLength();
    effect.to->drawCards(n);
    Room *room = effect.from->getRoom();
    room->playSkillEffect("jujian");

    if(n == 3){
        QSet<Card::CardType> types;

        foreach(int card_id, effect.card->getSubcards()){
            const Card *card = Sanguosha->getCard(card_id);
            types << card->getTypeId();
        }

        if(types.size() == 1){

            LogMessage log;
            log.type = "#JujianRecover";
            log.from = effect.from;
            const Card *card = Sanguosha->getCard(subcards.first());
            log.arg = card->getType();
            room->sendLog(log);

            RecoverStruct recover;
            recover.card = this;
            recover.who = effect.from;
            room->recover(effect.from, recover);
        }
    }
}

class NosJujian: public ViewAsSkill{
public:
    NosJujian():ViewAsSkill("nosjujian"){

    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 3;
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("NosJujianCard");
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.isEmpty())
            return NULL;

        NosJujianCard *card = new NosJujianCard;
        card->addSubcards(cards);
        return card;
    }
};

class EnyuanPattern: public CardPattern{
public:
    virtual bool match(const Player *player, const Card *card) const{
        return !player->hasEquip(card) && card->getSuit() == Card::Heart;
    }

    virtual bool willThrow() const{
        return false;
    }
};

class NosEnyuan: public TriggerSkill{
public:
    NosEnyuan():TriggerSkill("nosenyuan"){
        events << HpRecover << PostDamageInflicted;
        frequency = Compulsory;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{

        if(event == HpRecover){
            RecoverStruct recover = data.value<RecoverStruct>();
            if(recover.who && recover.who != player){
                recover.who->drawCards(recover.recover);

                LogMessage log;
                log.type = "#EnyuanRecover";
                log.from = player;
                log.to << recover.who;
                log.arg = QString::number(recover.recover);
                log.arg2 = objectName();

                room->sendLog(log);

                room->playSkillEffect("enyuan", qrand() % 2 + 1);

            }
        }else if(event == PostDamageInflicted){
            DamageStruct damage = data.value<DamageStruct>();
            ServerPlayer *source = damage.from;
            if(source && source != player){
                room->playSkillEffect("enyuan", qrand() % 2 + 3);

                const Card *card = room->askForCard(source, ".enyuan", "@enyuanheart", QVariant(), NonTrigger);
                if(card){
                    room->showCard(source, card->getEffectiveId());
                    player->obtainCard(card);
                }else{
                    room->loseHp(source);
                }
            }
        }

        return false;
    }
};

NosXuanhuoCard::NosXuanhuoCard(){
    once = true;
    will_throw = false;
    mute = true;
}

void NosXuanhuoCard::onEffect(const CardEffectStruct &effect) const{
    effect.to->obtainCard(this);

    Room *room = effect.from->getRoom();
    room->playSkillEffect("xuanhuo");
    if(effect.to->isNude())
        return;

    int card_id = room->askForCardChosen(effect.from, effect.to, "he", objectName());
    room->obtainCard(effect.from, card_id, room->getCardPlace(card_id) != Player::Hand);

    QList<ServerPlayer *> targets = room->getOtherPlayers(effect.to);
    ServerPlayer *target = room->askForPlayerChosen(effect.from, targets, "nosxuanhuo");
    if(target != effect.from)
        room->obtainCard(target, card_id, false);
}

class NosXuanhuo: public OneCardViewAsSkill{
public:
    NosXuanhuo():OneCardViewAsSkill("nosxuanhuo"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return ! player->hasUsed("NosXuanhuoCard");
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return ! to_select->isEquipped() && to_select->getFilteredCard()->getSuit() == Card::Heart;
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        NosXuanhuoCard *card = new NosXuanhuoCard;
        card->addSubcard(card_item->getFilteredCard());
        return card;
    }
};

class NosXuanfeng: public TriggerSkill{
public:
    NosXuanfeng():TriggerSkill("nosxuanfeng"){
        events << CardLostOneTime;
    }

    virtual QString getDefaultChoice(ServerPlayer *) const{
        return "nothing";
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *lingtong, QVariant &data) const{
        if(event == CardLostOneTime){
            CardsMoveOneTimeStar move = data.value<CardsMoveOneTimeStar>();
            if (move->from_places.contains(Player::Equip))
            {

                QString choice = room->askForChoice(lingtong, objectName(), "slash+damage+nothing");


                if(choice == "slash"){
                    QList<ServerPlayer *> targets;
                    foreach(ServerPlayer *target, room->getAlivePlayers()){
                        if(lingtong->canSlash(target, false))
                            targets << target;
                    }

                    ServerPlayer *target = room->askForPlayerChosen(lingtong, targets, "xuanfeng-slash");

                    Slash *slash = new Slash(Card::NoSuit, 0);
                    slash->setSkillName("xuanfeng");

                    CardUseStruct card_use;
                    card_use.card = slash;
                    card_use.from = lingtong;
                    card_use.to << target;
                    room->useCard(card_use, false);
                }else if(choice == "damage"){
                    room->playSkillEffect("xuanfeng");

                    QList<ServerPlayer *> players = room->getOtherPlayers(lingtong), targets;
                    foreach(ServerPlayer *p, players){
                        if(lingtong->distanceTo(p) <= 1)
                            targets << p;
                    }

                    ServerPlayer *target = room->askForPlayerChosen(lingtong, targets, "xuanfeng-damage");

                    DamageStruct damage;
                    damage.from = lingtong;
                    damage.to = target;
                    room->damage(damage);
                }
            }
        }

        return false;
    }
};

class NosZhenggong: public MasochismSkill{
public:
    NosZhenggong():MasochismSkill("noszhenggong"){
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return TriggerSkill::triggerable(target) && target->getMark("nosbaijiang") == 0;
    }

    virtual void onDamaged(ServerPlayer *zhonghui, const DamageStruct &damage) const{
        if(damage.from && damage.from->hasEquip())
        {
            QVariant data = QVariant::fromValue((PlayerStar)damage.from);
            if(!zhonghui->askForSkillInvoke(objectName(), data))
                return;

            Room *room = zhonghui->getRoom();

            int equip = room->askForCardChosen(zhonghui, damage.from, "e", objectName());

            const Card *card = Sanguosha->getCard(equip);
            room->moveCardTo(card, zhonghui, Player::Hand);

            CardUseStruct card_use;
            card_use.from = zhonghui;
            card_use.card = card;
            room->useCard(card_use);
        }
    }
};

NosQuanjiCard::NosQuanjiCard(){
    target_fixed = true;
    will_throw = false;
}

void NosQuanjiCard::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    ServerPlayer *target = room->getTag("QuanjiTarget").value<PlayerStar>();
    room->cardEffect(this, source, target);
}

void NosQuanjiCard::onEffect(const CardEffectStruct &effect) const{
    if(effect.from->pindian(effect.to, "nosquanji",
                            Sanguosha->getCard(this->getSubcards().first())))
        effect.from->setFlags("quanji_win");
}


class NosQuanjiViewAsSkill:public OneCardViewAsSkill{
public:
    NosQuanjiViewAsSkill():OneCardViewAsSkill("nosquanji"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@@nosquanji";
    }

    virtual bool viewFilter(const CardItem *to_select) const{
        return !to_select->isEquipped();
    }

    virtual const Card *viewAs(CardItem *card_item) const{
        Card *card = new NosQuanjiCard;
        card->addSubcard(card_item->getFilteredCard());

        return card;
    }
};

class NosQuanji: public TriggerSkill{
public:
    NosQuanji():TriggerSkill("nosquanji"){
        events << PhaseChange;
        view_as_skill = new NosQuanjiViewAsSkill;
    }

    virtual int getPriority() const{
        return 5;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return target && !target->hasSkill(objectName());
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &) const{
        if(player->getPhase() != Player::RoundStart || player->isKongcheng())
            return false;

        bool skip = false;
        room->setTag("QuanjiTarget", QVariant::fromValue((PlayerStar)player));
        foreach(ServerPlayer *zhonghui, room->findPlayersBySkillName(objectName()))
        {
            if(zhonghui->isKongcheng() || zhonghui->getMark("nosbaijiang") > 0 || player->isKongcheng())
                continue;

            QString prompt = "@quanji-pindian:" + player->objectName() + ":" + objectName();
            bool used = room->askForUseCard(zhonghui, "@@nosquanji", prompt);

            if(!used)
                continue;
            else if(zhonghui->hasFlag("quanji_win"))
            {
                zhonghui->setFlags("-quanji_win");
                if(!skip)
                {
                    player->skip(Player::Start);
                    player->skip(Player::Judge);
                    skip = true;
                }
            }
        }
        room->removeTag("QuanjiTarget");
        return skip;
    }
};

class NosBaijiang: public PhaseChangeSkill{
public:
    NosBaijiang():PhaseChangeSkill("nosbaijiang"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
                && target->getMark("nosbaijiang") == 0
                && target->getPhase() == Player::Start
                && target->getEquips().length() >= 3;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();

        /*LogMessage log;
        log.type = "#ZhijiWake";
        log.from = jiangwei;
        log.arg = objectName();
        room->sendLog(log);

        room->playSkillEffect("zhiji");
        room->broadcastInvoke("animate", "lightbox:$Zhiji:5000");
        room->getThread()->delay(5000);*/

        room->setPlayerMark(zhonghui, "nosbaijiang", 1);
        zhonghui->gainMark("@wake");
        room->setPlayerProperty(zhonghui, "maxhp", zhonghui->getMaxHp() + 1);

        room->acquireSkill(zhonghui, "nosyexin");
        room->detachSkillFromPlayer(zhonghui, "noszhenggong");
        room->detachSkillFromPlayer(zhonghui, "nosquanji");

        return false;
    }
};

NosYexinCard::NosYexinCard(){
    target_fixed = true;
    once = true;
}

void NosYexinCard::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *zhonghui = card_use.from;

    QList<int> powers = zhonghui->getPile("nospower");
    QList<int> willput;
    if(powers.isEmpty())
        return;

    int ai_delay = Config.AIDelay;
    Config.AIDelay = 0;

    int n = 0;
    while(!powers.isEmpty())
    {
        room->fillAG(powers, zhonghui);

        int card_id = room->askForAG(zhonghui, powers, true, "nosyexin");
        if(card_id == -1)
        {
            zhonghui->invoke("clearAG");
            break;
        }
        powers.removeOne(card_id);
        ++n;

        room->obtainCard(zhonghui, card_id);
        zhonghui->invoke("clearAG");
    }

    Config.AIDelay = ai_delay;

    if(n == 0)
        return;

    const Card *exchange_card = room->askForExchange(zhonghui, "nosyexin", n);

    foreach(int card_id, exchange_card->getSubcards())
        willput.push_back(card_id);

    zhonghui->addToPile("nospower", willput);

    LogMessage log;
    log.type = "#QixingExchange";
    log.from = zhonghui;
    log.arg = QString::number(qMin(n, zhonghui->getHandcardNum()));
    log.arg2 = "nosyexin";
    room->sendLog(log);

    delete exchange_card;
}

class NosYexinViewAsSkill: public ZeroCardViewAsSkill{
public:
    NosYexinViewAsSkill():ZeroCardViewAsSkill("nosyexin"){
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return !player->getPile("nospower").isEmpty() && !player->hasUsed("NosYexinCard");
    }

    virtual const Card *viewAs() const{
        return new NosYexinCard;
    }

    virtual Location getLocation() const{
        return Right;
    }
};

class NosYexin: public TriggerSkill{
public:
    NosYexin():TriggerSkill("nosyexin"){
        events << PostDamageCaused << PostDamageInflicted;
        view_as_skill = new NosYexinViewAsSkill;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *zhonghui, QVariant &) const{
        if(!zhonghui->askForSkillInvoke(objectName()))
            return false;

        int card_id = room->drawCard();
        zhonghui->addToPile("nospower", card_id);

        return false;
    }
};

class NosPaiyi: public PhaseChangeSkill{
public:
    NosPaiyi():PhaseChangeSkill("nospaiyi"){
        _m_place["Judging"] = Player::Judging;
        _m_place["Equip"] = Player::Equip;
        _m_place["Hand"] = Player::Hand;
    }

    QString getPlace(Room *room, ServerPlayer *player, QStringList places) const{
        if(places.length() == 1)
        {
            return places.first();
        }
        else
        {
            QString place = room->askForChoice(player, "nospaiyi", places.join("+"));
            return place;
        }

        return QString();
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        if(zhonghui->getPhase() != Player::Finish || zhonghui->getPile("nospower").isEmpty())
            return false;

        Room *room = zhonghui->getRoom();

        QList<int> powers = zhonghui->getPile("nospower");
        QStringList places;

        places << "Hand";

        LogMessage log;
        log.type = "#InvokeSkill";
        log.from = zhonghui;
        log.arg = objectName();
        room->sendLog(log);

        room->fillAG(powers, zhonghui);
        int power = room->askForAG(zhonghui, powers, true, "nospaiyi");
        zhonghui->invoke("clearAG");

        if(power == -1)
            power = powers.first();

        const Card *card = Sanguosha->getCard(power);

        ServerPlayer *target = room->askForPlayerChosen(zhonghui, room->getAlivePlayers(), "nospaiyi");
        CardMoveReason reason(CardMoveReason::S_REASON_TRANSFER, zhonghui->objectName(), "nospaiyi", QString());

        if(card->inherits("DelayedTrick"))
        {
            if(!zhonghui->isProhibited(target, card) && !target->containsTrick(card->objectName()))
                places << "Judging";

            room->moveCardTo(card, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        }
        else if(card->inherits("EquipCard"))
        {
            const EquipCard *equip = qobject_cast<const EquipCard *>(card);
            if(!target->getEquip(equip->location()))
                places << "Equip";

            room->moveCardTo(card, target, _m_place[getPlace(room, zhonghui, places)], reason, true);
        }
        else
            room->moveCardTo(card, target, _m_place[getPlace(room, zhonghui, places)], reason, true);

        if(target != zhonghui)
            room->drawCards(zhonghui, 1);

        return false;
    }

private:
    QMap<QString, Player::Place> _m_place;
};

class NosZili: public PhaseChangeSkill{
public:
    NosZili():PhaseChangeSkill("noszili"){
        frequency = Wake;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return PhaseChangeSkill::triggerable(target)
                && target->getMark("noszili") == 0
                && target->getPhase() == Player::Start
                && target->getPile("nospower").length() >= 4;
    }

    virtual bool onPhaseChange(ServerPlayer *zhonghui) const{
        Room *room = zhonghui->getRoom();

        /*LogMessage log;
        log.type = "#ZhijiWake";
        log.from = jiangwei;
        log.arg = objectName();
        room->sendLog(log);

        room->playSkillEffect("zhiji");
        room->broadcastInvoke("animate", "lightbox:$Zhiji:5000");
        room->getThread()->delay(5000);*/

        room->setPlayerMark(zhonghui, "noszili", 1);
        zhonghui->gainMark("@wake");
        room->loseMaxHp(zhonghui);

        room->acquireSkill(zhonghui, "nospaiyi");

        return false;
    }
};

NostalGeneralPackage::NostalGeneralPackage()
    :Package("nostal_general")
{
    General *noszhouyu = new General(this, "noszhouyu", "wu", 3);
    noszhouyu->addSkill(new NosFanjian);
    noszhouyu->addSkill("yingzi");

    General *noszhurong = new General(this, "noszhurong", "shu", 4, false);
    noszhurong->addSkill("juxiang");
    noszhurong->addSkill("#sa_avoid_juxiang");
    noszhurong->addSkill(new NosLieren);


    General *nosxushu = new General(this, "nosxushu", "shu", 3);
    nosxushu->addSkill(new NosWuyan);
    nosxushu->addSkill(new NosJujian);

    General *nosfazheng = new General(this, "nosfazheng", "shu", 3);
    nosfazheng->addSkill(new NosEnyuan);
    patterns.insert(".enyuan", new EnyuanPattern);
    nosfazheng->addSkill(new NosXuanhuo);

    General *noslingtong = new General(this, "noslingtong", "wu");
    noslingtong->addSkill(new NosXuanfeng);

    General *noszhonghui = new General(this, "noszhonghui", "wei", 3);
    noszhonghui->addSkill(new NosZhenggong);
    noszhonghui->addSkill(new NosQuanji);
    noszhonghui->addSkill(new NosBaijiang);
    noszhonghui->addSkill(new NosZili);

    noszhonghui->addRelateSkill("nosyexin");
    noszhonghui->addRelateSkill("nospaiyi");

    addMetaObject<NosFanjianCard>();
    addMetaObject<NosXuanhuoCard>();
    addMetaObject<NosJujianCard>();
    addMetaObject<NosQuanjiCard>();
    addMetaObject<NosYexinCard>();

    skills << new NosYexin << new NosPaiyi;
}

ADD_PACKAGE(NostalGeneral)
ADD_PACKAGE(Nostalgia)
