#include "standard.h"
#include "standard-equips.h"
#include "general.h"
#include "engine.h"
#include "client.h"
#include "room.h"
#include "carditem.h"

Slash::Slash(Suit suit, int number): BasicCard(suit, number)
{
    setObjectName("slash");
    nature = DamageStruct::Normal;
}

DamageStruct::Nature Slash::getNature() const{
    return nature;
}

void Slash::setNature(DamageStruct::Nature nature){
    this->nature = nature;
}

bool Slash::IsAvailable(const Player *player){
    if(player->hasFlag("tianyi_failed") || player->hasFlag("xianzhen_failed"))
        return false;

    Card *tc = Sanguosha->cloneCard("slash", Card::NoSuit, 0);
    return (player->hasWeapon("crossbow") || player->canSlashWithoutCrossbow()) && !player->isLocked(tc);
}

bool Slash::isAvailable(const Player *player) const{
    return IsAvailable(player) && BasicCard::isAvailable(player);
}

QString Slash::getSubtype() const{
    return "attack_card";
}

void Slash::onUse(Room *room, const CardUseStruct &card_use) const{
    CardUseStruct new_use = card_use;

    if(new_use.from->hasFlag("SlashUsing"))
    {
        room->setPlayerFlag(new_use.from, "-SlashUsing");
        foreach(ServerPlayer *target, room->getAlivePlayers())
            if(target->hasFlag("SlashAssignee"))
            {
                room->setPlayerFlag(target, "-SlashAssignee");
                new_use.to << target;
            }
    }

    if(card_use.to.length() > 1 && card_use.from->hasWeapon("halberd")
            && card_use.from->isLastHandCard(card_use.card)
            && !subcards.contains(card_use.from->getWeapon()->getEffectiveId()))
    {
        room->setEmotion(card_use.from, QString("weapon/%1").arg("halberd"));
        room->getThread()->delay(1200);
    }

    if(new_use.to.length() > 1)
    {
        qStableSort(new_use.to.begin(), new_use.to.end(), CompareByActionOrder);
    }

    BasicCard::onUse(room, new_use);
}

void Slash::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    BasicCard::use(room, source, targets);

    if(this->hasFlag("drank")){
        LogMessage log;
        log.type = "#UnsetDrank";
        log.from = source;
        room->sendLog(log);
    }

}

void Slash::onEffect(const CardEffectStruct &card_effect) const{
    Room *room = card_effect.from->getRoom();
    if(card_effect.from->hasFlag("drank")){
        room->setCardFlag(this, "drank");
        room->setPlayerFlag(card_effect.from, "-drank");
    }

    SlashEffectStruct effect;
    effect.from = card_effect.from;
    effect.nature = nature;
    effect.slash = this;

    effect.to = card_effect.to;
    effect.drank = this->hasFlag("drank");

    room->slashEffect(effect);
}

bool Slash::targetsFeasible(const QList<const Player *> &targets, const Player *Self) const{
    return Self->hasFlag("SlashUsing") || !targets.isEmpty();
}

bool Slash::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select->hasFlag("SlashAssignee"))
        return false;

    int slash_targets = 0;

    if(!Self->hasFlag("SlashUsing"))
        slash_targets ++;

    if(Self->hasWeapon("halberd") && Self->isLastHandCard(this)
            && !subcards.contains(Self->getWeapon()->getEffectiveId()))
        slash_targets += 2;

    if(Self->hasSkill("shenji") && Self->getWeapon() == NULL)
        slash_targets += 2;

    bool distance_limit = true;

    if(Self->hasFlag("tianyi_success")){
        distance_limit = false;
        slash_targets ++;
    }

    if(Self->hasSkill("lihuo") && inherits("FireSlash"))
        slash_targets ++;

    if(targets.length() >= slash_targets)
        return false;

    if(inherits("WushenSlash")){
        distance_limit = false;
    }

    int distance_fix = 0;
    if(Self->getWeapon() && subcards.contains(Self->getWeapon()->getEffectiveId()))
        distance_fix += Self->getWeapon()->getRange() - 1;

    if(Self->getOffensiveHorse() && subcards.contains(Self->getOffensiveHorse()->getEffectiveId()))
        distance_fix += 1;


    if(Self->hasFlag("Luanwu") && targets.isEmpty())
    {
        return Self->distanceTo(to_select) == Self->getMark("Luanwu") && Self->canSlash(to_select, distance_limit, distance_fix);
    }

    return Self->canSlash(to_select, distance_limit, distance_fix);
}

Jink::Jink(Suit suit, int number):BasicCard(suit, number){
    setObjectName("jink");

    target_fixed = true;
}

QString Jink::getSubtype() const{
    return "defense_card";
}

bool Jink::isAvailable(const Player *) const{
    return false;
}

Peach::Peach(Suit suit, int number):BasicCard(suit, number){
    setObjectName("peach");
    target_fixed = true;
}

QString Peach::getSubtype() const{
    return "recover_card";
}

void Peach::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    BasicCard::use(room, source, targets);
    if(targets.isEmpty())
        room->cardEffect(this, source, source);
}

void Peach::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    // do animation
    room->broadcastInvoke("animate", QString("peach:%1:%2")
                          .arg(effect.from->objectName())
                          .arg(effect.to->objectName()));

    // recover hp
    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;

    room->recover(effect.to, recover);
}

bool Peach::isAvailable(const Player *player) const{
    Card *tc = Sanguosha->cloneCard("peach", Card::NoSuit, 0);
    return player->isWounded() && !player->isLocked(tc);
}

Crossbow::Crossbow(Suit suit, int number)
    :Weapon(suit, number, 1)
{
    setObjectName("crossbow");
}

class DoubleSwordSkill: public WeaponSkill{
public:
    DoubleSwordSkill():WeaponSkill("double_sword"){
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.from->objectName() != player->objectName())
            return false;
        foreach(ServerPlayer *to, use.to){
            if(use.from->getGeneral()->isMale() != to->getGeneral()->isMale()
                    && use.card->inherits("Slash")){
                if(use.from->askForSkillInvoke(objectName())){
                    bool draw_card = false;

                    room->setEmotion(player, QString("weapon/%1").arg(objectName()));

                    if(to->isKongcheng())
                        draw_card = true;
                    else{
                        QString prompt = "double-sword-card:" + use.from->getGeneralName();
                        const Card *card = room->askForCard(to, ".", prompt, QVariant(), NonTrigger);
                        if(card){
                            room->throwCard(card, to);
                        }else
                            draw_card = true;
                    }

                    if(draw_card)
                        use.from->drawCards(1);
                }
            }
        }

        return false;
    }
};

DoubleSword::DoubleSword(Suit suit, int number)
    :Weapon(suit, number, 2)
{
    setObjectName("double_sword");
    skill = new DoubleSwordSkill;
}

class QinggangSwordSkill: public WeaponSkill{
public:
    QinggangSwordSkill():WeaponSkill("qinggang_sword"){
        events << TargetConfirmed << Death;
    }

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *, QVariant &data) const{
        if(event == TargetConfirmed)
        {
            CardUseStruct use = data.value<CardUseStruct>();
            bool doAnimate = false;
            if(use.from && use.from->hasWeapon(objectName()) && use.card->inherits("Slash"))
            {
                foreach(ServerPlayer *target, use.to)
                {
                    target->addMark("qinggang");
                    if(target->getArmor() || target->hasSkill("bazhen"))
                        doAnimate = true;
                }
            }

            if(doAnimate)
            {
                room->getThread()->delay(1200);
                room->setEmotion(use.from, QString("weapon/%1").arg(objectName()));
            }
        }
        else
        {
            foreach(ServerPlayer *player,room->getAlivePlayers())
                player->removeMark("qinggang");
        }

        return false;
    }
};

QinggangSword::QinggangSword(Suit suit, int number)
    :Weapon(suit, number, 2)
{
    setObjectName("qinggang_sword");

    skill = new QinggangSwordSkill;
}

class BladeSkill : public WeaponSkill{
public:
    BladeSkill():WeaponSkill("blade"){
        events << SlashMissed;
    }

    virtual int getPriority() const{
        return -1;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        if(room->isProhibited(player, effect.to, effect.slash))
            return false;

        int blade = player->getWeapon()->getEffectiveId();
        room->setCardFlag(blade, "isUsing");
        const Card *card = room->askForCard(player, "slash", "blade-slash:" + effect.to->objectName(), QVariant(), NonTrigger);
        if(card){
            // if player is drank, unset his flag
            if(player->hasFlag("drank"))
                room->setPlayerFlag(player, "-drank");

            CardUseStruct use;
            use.card = card;
            use.from = player;
            use.to << effect.to;

            room->setPlayerFlag(player, "QingLongblade");

            room->useCard(use, false);

        }
        room->setCardFlag(blade, "-isUsing");
        return false;
    }
};

Blade::Blade(Suit suit, int number)
    :Weapon(suit, number, 3)
{
    setObjectName("blade");
    skill = new BladeSkill;
}

class SpearSkill: public ViewAsSkill{
public:
    SpearSkill():ViewAsSkill("spear"){

    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        return Slash::IsAvailable(player);
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return  pattern == "slash";
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        return selected.length() < 2 && !to_select->isEquipped();
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 2)
            return NULL;

        const Card *first = cards.at(0)->getFilteredCard();
        const Card *second = cards.at(1)->getFilteredCard();

        Card::Suit suit = Card::NoSuit;
        if(first->isBlack() && second->isBlack())
            suit = Card::Spade;
        else if(first->isRed() && second->isRed())
            suit = Card::Heart;

        Slash *slash = new Slash(suit, 0);
        slash->setSkillName(objectName());
        slash->addSubcard(first);
        slash->addSubcard(second);

        return slash;
    }
};

Spear::Spear(Suit suit, int number)
    :Weapon(suit, number, 3)
{
    setObjectName("spear");
    attach_skill = true;
}

class AxeViewAsSkill: public ViewAsSkill{
public:
    AxeViewAsSkill():ViewAsSkill("axe"){

    }

    virtual bool isEnabledAtPlay(const Player *) const{
        return false;
    }

    virtual bool isEnabledAtResponse(const Player *, const QString &pattern) const{
        return pattern == "@axe";
    }

    virtual bool viewFilter(const QList<CardItem *> &selected, const CardItem *to_select) const{
        if(selected.length() >= 2)
            return false;

        if(to_select->getCard() == Self->getWeapon())
            return false;

        return true;
    }

    virtual const Card *viewAs(const QList<CardItem *> &cards) const{
        if(cards.length() != 2)
            return NULL;

        DummyCard *card = new DummyCard;
        card->setSkillName(objectName());
        card->addSubcards(cards);
        return card;
    }
};

class AxeSkill: public WeaponSkill{
public:
    AxeSkill():WeaponSkill("axe"){
        events << SlashMissed;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();

        CardStar card = room->askForCard(player, "@axe", "@axe:" + effect.to->objectName(), data, CardDiscarded);
        if(card){
            LogMessage log;
            log.type = "#AxeSkill";
            log.from = player;
            log.to << effect.to;
            log.arg = objectName();
            room->sendLog(log);

            room->setEmotion(effect.to, QString("weapon/%1").arg(objectName()));
            room->getThread()->delay(1500);

            room->slashResult(effect, NULL);
        }

        return false;
    }
};

Axe::Axe(Suit suit, int number)
    :Weapon(suit, number, 3)
{
    setObjectName("axe");
    skill = new AxeSkill;
    attach_skill = true;
}

Halberd::Halberd(Suit suit, int number)
    :Weapon(suit, number, 4)
{
    setObjectName("halberd");
}

class KylinBowSkill: public WeaponSkill{
public:
    KylinBowSkill():WeaponSkill("kylin_bow"){
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        QStringList horses;
        if(damage.card && damage.card->inherits("Slash") && !damage.chain){
            if(damage.to->getDefensiveHorse())
                horses << "dhorse";
            if(damage.to->getOffensiveHorse())
                horses << "ohorse";

            if(horses.isEmpty())
                return false;

            if(!player->askForSkillInvoke(objectName(), data))
                return false;

            QString horse_type;
            if(horses.length() == 2)
                horse_type = room->askForChoice(player, objectName(), horses.join("+"));
            else
                horse_type = horses.first();

            room->setEmotion(damage.from, QString("weapon/%1").arg(objectName()));

            if(horse_type == "dhorse")
                room->throwCard(damage.to->getDefensiveHorse(), damage.to);
            else if(horse_type == "ohorse")
                room->throwCard(damage.to->getOffensiveHorse(), damage.to);
        }

        return false;
    }
};

KylinBow::KylinBow(Suit suit, int number)
    :Weapon(suit, number, 5)
{
    setObjectName("kylin_bow");
    skill = new KylinBowSkill;
}

class EightDiagramSkill: public ArmorSkill{
private:
    EightDiagramSkill():ArmorSkill("eight_diagram"){
        events << CardAsked;
    }

public:
    static EightDiagramSkill *GetInstance(){
        static EightDiagramSkill *instance = NULL;
        if(instance == NULL)
            instance = new EightDiagramSkill;

        return instance;
    }

    virtual int getPriority() const{
        return 2;
    }

    virtual bool trigger(TriggerEvent, Room* room, ServerPlayer *player, QVariant &data) const{
        QString asked = data.toString();
        if(asked == "jink"){
            if(room->askForSkillInvoke(player, objectName())){
                room->setEmotion(player, QString("armor/%1").arg(objectName()));

                JudgeStruct judge;
                judge.pattern = QRegExp("(.*):(heart|diamond):(.*)");
                judge.good = true;
                judge.reason = objectName();
                judge.who = player;

                room->judge(judge);
                if(judge.isGood()){
                    Jink *jink = new Jink(Card::NoSuit, 0);
                    jink->setSkillName(objectName());
                    room->provide(jink);

                    return true;
                }
            }
        }
        return false;
    }
};



EightDiagram::EightDiagram(Suit suit, int number)
    :Armor(suit, number){
    setObjectName("eight_diagram");
    skill = EightDiagramSkill::GetInstance();
}

AmazingGrace::AmazingGrace(Suit suit, int number)
    :GlobalEffect(suit, number)
{
    setObjectName("amazing_grace");
    has_preact = true;
}

void AmazingGrace::doPreAction(Room *room, const CardUseStruct &card_use) const{
    QList<ServerPlayer *> players = card_use.to.isEmpty() ? room->getAllPlayers() : card_use.to;
    QList<int> card_ids = room->getNCards(players.length());
    room->fillAG(card_ids);

    QVariantList ag_list;
    foreach(int card_id, card_ids){
        room->setCardFlag(card_id, "visible");
        ag_list << card_id;
    }
    room->setTag("AmazingGrace", ag_list);
}

void AmazingGrace::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName());
    room->moveCardTo(this, source, NULL, Player::DiscardPile, reason);

    QList<ServerPlayer *> players = targets.isEmpty() ? room->getAllPlayers() : targets;
    GlobalEffect::use(room, source, players);
    QVariantList ag_list;
    ag_list = room->getTag("AmazingGrace").toList();

    // throw the rest cards
    foreach(QVariant card_id, ag_list){
        room->takeAG(NULL, card_id.toInt());
    }

    room->broadcastInvoke("clearAG");
}

void AmazingGrace::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.from->getRoom();
    QVariantList ag_list = room->getTag("AmazingGrace").toList();
    QList<int> card_ids;
    foreach(QVariant card_id, ag_list)
        card_ids << card_id.toInt();

    int card_id = room->askForAG(effect.to, card_ids, false, objectName());
    card_ids.removeOne(card_id);

    room->takeAG(effect.to, card_id);
    ag_list.removeOne(card_id);

    room->setTag("AmazingGrace", ag_list);
}

GodSalvation::GodSalvation(Suit suit, int number)
    :GlobalEffect(suit, number)
{
    setObjectName("god_salvation");
}

bool GodSalvation::isCancelable(const CardEffectStruct &effect) const{
    return effect.to->isWounded();
}

void GodSalvation::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();

    RecoverStruct recover;
    recover.card = this;
    recover.who = effect.from;
    room->recover(effect.to, recover);
}

SavageAssault::SavageAssault(Suit suit, int number)
    :AOE(suit, number)
{
    setObjectName("savage_assault");
}

void SavageAssault::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    const Card *slash = room->askForCard(effect.to, "slash", "savage-assault-slash:" + effect.from->objectName(), QVariant(), CardResponsed, effect.from);
    if(slash)
        room->setEmotion(effect.to, "killer");
    else{
        DamageStruct damage;
        damage.card = this;
        damage.damage = 1;
        damage.to = effect.to;
        damage.nature = DamageStruct::Normal;

        if(effect.from->isAlive())
            damage.from = effect.from;
        else
            damage.from = NULL;

        room->damage(damage);
    }
}

ArcheryAttack::ArcheryAttack(Card::Suit suit, int number)
    :AOE(suit, number)
{
    setObjectName("archery_attack");
}

void ArcheryAttack::onEffect(const CardEffectStruct &effect) const{
    Room *room = effect.to->getRoom();
    const Card *jink = room->askForCard(effect.to, "jink", "archery-attack-jink:" + effect.from->objectName(), QVariant(), CardResponsed, effect.from);
    if(jink)
        room->setEmotion(effect.to, "jink");
    else{
        DamageStruct damage;
        damage.card = this;
        damage.damage = 1;
        if(effect.from->isAlive())
            damage.from = effect.from;
        else
            damage.from = NULL;
        damage.to = effect.to;
        damage.nature = DamageStruct::Normal;

        room->damage(damage);
    }
}

void SingleTargetTrick::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &targets) const{
    CardEffectStruct effect;
    effect.card = this;
    effect.from = source;
    if(!targets.isEmpty()){
        foreach(ServerPlayer *tmp, targets){
            effect.to = tmp;
            room->cardEffect(effect);
        }
    }
    else{
        effect.to = source;
        room->cardEffect(effect);
    }

    if(room->getCardPlace(this->getEffectiveId()) == Player::DealingArea)
    {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName());
        reason.m_skillName = this->getSkillName();
        if (targets.size() == 1) reason.m_targetId = targets.first()->objectName();
        room->moveCardTo(this, source, NULL, Player::DiscardPile, reason);
    }
}

Collateral::Collateral(Card::Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("collateral");
}

bool Collateral::isAvailable(const Player *player) const{
    bool canUse = false;
    foreach(const Player *p, player->getSiblings()){
        if(p->getWeapon() && p->isAlive()){
            canUse = true;
            break;
        }
    }

    return canUse && SingleTargetTrick::isAvailable(player);
}

bool Collateral::targetsFeasible(const QList<const Player *> &targets, const Player *) const{
    return targets.length() == 2;
}

bool Collateral::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(targets.isEmpty()){
        if(to_select->hasSkill("weimu") && isBlack())
            return false;

        return to_select->getWeapon() && to_select != Self;
    }else if(targets.length() == 1){
        const Player *first = targets.first();
        if(Self->hasSkill("kongcheng") && Self->isLastHandCard(this))
            return to_select != Self && first != Self && first->getWeapon() && first->canSlash(to_select);
        return first != Self && first->getWeapon() && first->canSlash(to_select);
    }else
        return false;
}

bool Collateral::doCollateral(Room *room, ServerPlayer *killer, ServerPlayer *victim, const QString &prompt) const{
    bool useSlash = false;
    if(killer->canSlash(victim))
    {
        useSlash = room->askForSlashTo(killer, victim, prompt);
    }
    return useSlash;
}

void Collateral::onUse(Room *room, const CardUseStruct &card_use) const{
    ServerPlayer *source = card_use.from;
    ServerPlayer *killer = card_use.to.at(0);
    ServerPlayer *victim = card_use.to.at(1);
    RoomThread *thread = room->getThread();

    CardUseStruct new_use = card_use;
    new_use.to.removeAt(1);
    QVariant data = QVariant::fromValue(new_use);

    room->setTag("collateralVictim", QVariant::fromValue((PlayerStar)victim));

    thread->trigger(CardonUse, room, source, data);

    LogMessage log;
    log.from = source;
    log.to << killer;
    log.type = "#UseCard";
    log.card_str = this->toString();
    room->sendLog(log);

    log = LogMessage();
    log.from = killer;
    log.to << victim;
    log.type = "#Collateral";
    room->sendLog(log);

    room->broadcastInvoke("animate", QString("indicate:%1:%2").arg(killer->objectName()).arg(victim->objectName()));

    QList<int> used_cards;
    QList<CardsMoveStruct> moves;
    if(this->isVirtualCard()){
        foreach(int card_id, this->getSubcards()){
            used_cards << card_id;
        }
    }
    else{
        used_cards << this->getEffectiveId();
    }

    if(!used_cards.isEmpty() && this->getEffectiveId() > 0 && room->getCardPlace(this->getEffectiveId()) != Player::DealingArea)
    {
        CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName(), QString(), this->getSkillName(), QString());
        reason.m_targetId = killer->objectName();
        CardsMoveStruct move(used_cards, source, NULL, Player::DealingArea, reason);
        moves.append(move);
        room->moveCardsAtomic(moves, true);
    }

    thread->trigger(CardUsed, room, source, data);

    thread->trigger(CardFinished, room, source, data);
}

void Collateral::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *source = effect.from;
    Room *room = source->getRoom();
    ServerPlayer *killer = effect.to;
    ServerPlayer *victim = room->getTag("collateralVictim").value<PlayerStar>();
    room->removeTag("collateralVictim");
    const Weapon *weapon = killer->getWeapon();
    if(!weapon)
        return;

    QString prompt = QString("collateral-slash:%1:%2")
            .arg(source->objectName()).arg(victim->objectName());
    const Card *slash = NULL;

    if(killer->canSlash(victim))
    {
        if(killer->getAI())
            slash = room->askForCard(killer, "slash", prompt, QVariant(), NonTrigger);
    }
    if (victim->isDead()){
        if (source->isDead()){
            if(killer->isAlive() && killer->getWeapon()){
                int card_id = weapon->getId();
                room->throwCard(card_id, killer);
            }
        }
        else
        {
            if(killer->isAlive() && killer->getWeapon()){
                source->obtainCard(weapon);
            }
        }
    }
    else if (source->isDead()){
        if (killer->isAlive()){
            if(slash){
                CardUseStruct use;
                use.card = slash;
                use.from = killer;
                use.to << victim;
                room->useCard(use);
            }
            else if(!doCollateral(room, killer, victim, prompt)){
                if(killer->getWeapon()){
                    int card_id = weapon->getId();
                    room->throwCard(card_id, killer);
                }
            }
        }
    }
    else{
        if(killer->isDead()) ;
        else if(!killer->getWeapon()){
            if(slash){
                CardUseStruct use;
                use.card = slash;
                use.from = killer;
                use.to << victim;
                room->useCard(use);
            }
            else
                doCollateral(room, killer, victim, prompt);
        }
        else{
            if(slash){
                CardUseStruct use;
                use.card = slash;
                use.from = killer;
                use.to << victim;
                room->useCard(use);
            }
            else if(!doCollateral(room, killer, victim, prompt)){
                if(killer->getWeapon())
                    source->obtainCard(weapon);
            }
        }
    }
}

Nullification::Nullification(Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("nullification");
}

void Nullification::use(Room *room, ServerPlayer *source, const QList<ServerPlayer *> &) const{
    // does nothing, just throw it
    CardMoveReason reason(CardMoveReason::S_REASON_USE, source->objectName());
    room->moveCardTo(this, source, NULL, Player::DiscardPile, reason);
}

bool Nullification::isAvailable(const Player *) const{
    return false;
}

ExNihilo::ExNihilo(Suit suit, int number)
    :SingleTargetTrick(suit, number, false)
{
    setObjectName("ex_nihilo");
    target_fixed = true;
}

void ExNihilo::onEffect(const CardEffectStruct &effect) const{
    effect.to->drawCards(2);
}

Duel::Duel(Suit suit, int number)
    :SingleTargetTrick(suit, number, true)
{
    setObjectName("duel");
}

bool Duel::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(to_select->hasSkill("kongcheng") && to_select->isKongcheng())
        return false;

    if(to_select == Self)
        return false;

    return targets.isEmpty();
}

void Duel::onEffect(const CardEffectStruct &effect) const{
    ServerPlayer *first = effect.to;
    ServerPlayer *second = effect.from;
    Room *room = first->getRoom();

    room->setEmotion(first, "duel");
    room->setEmotion(second, "duel");

    while(first->isAlive())
    {
        if(second->hasSkill("wushuang")){
            room->playSkillEffect("wushuang");
            const Card *slash = room->askForCard(first, "slash", "@wushuang-slash-1:" + second->objectName(), QVariant(), CardResponsed, second);
            if(slash == NULL)
                break;

            slash = room->askForCard(first, "slash", "@wushuang-slash-2:" + second->objectName(), QVariant(), CardResponsed, second);
            if(slash == NULL)
                break;

        }else{
            const Card *slash = room->askForCard(first, "slash", "duel-slash:" + second->objectName(), QVariant(), CardResponsed, second);
            if(slash == NULL)
                break;
        }

        qSwap(first, second);
    }

    if(first->isAlive())
    {
        DamageStruct damage;
        damage.card = this;
        if(second->isAlive())
            damage.from = second;
        else
            damage.from = NULL;
        damage.to = first;

        room->damage(damage);
    }
}

Snatch::Snatch(Suit suit, int number):SingleTargetTrick(suit, number, true) {
    setObjectName("snatch");
}

bool Snatch::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(ServerInfo.GameMode == "03_3kingdoms" && ServerInfo.EnableSnatchHero)
    {
        if(to_select->isAllNude() && (to_select->getPile("heros").isEmpty() || Self->getMark("hero") == 0))
            return false;
    }
    else if(to_select->isAllNude())
        return false;

    if(to_select == Self)
        return false;

    if(Self->hasSkill("qicai"))
        return true;

    if(this->getSkillName()=="jixi")
        return Self->distanceTo(to_select,1) <= 1;
    else
        return Self->distanceTo(to_select) <= 1;

}

void Snatch::onEffect(const CardEffectStruct &effect) const{
    if(effect.from->isDead())
        return;
    if(effect.to->isAllNude())
        return;

    Room *room = effect.to->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", objectName());

    CardMoveReason reason(CardMoveReason::S_REASON_EXTRACTION, effect.from->objectName());
    room->obtainCard(effect.from, Sanguosha->getCard(card_id), reason, room->getCardPlace(card_id) != Player::PlaceHand);
}

Dismantlement::Dismantlement(Suit suit, int number)
    :SingleTargetTrick(suit, number, false) {
    setObjectName("dismantlement");
}

bool Dismantlement::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const{
    if(!targets.isEmpty())
        return false;

    if(ServerInfo.GameMode == "03_3kingdoms")
    {
        QList<int> heros = Self->getPile("heros");
        heros.removeOne(Self->getMark("hero"));
        if(to_select->isAllNude() && (to_select->getPile("heros").isEmpty() || heros.isEmpty()))
            return false;
    }
    else if(to_select->isAllNude())
        return false;

    if(to_select == Self)
        return false;

    return true;
}

void Dismantlement::onEffect(const CardEffectStruct &effect) const{
    if(effect.from->isDead())
        return;
    if(effect.to->isAllNude())
        return;

    Room *room = effect.to->getRoom();
    int card_id = room->askForCardChosen(effect.from, effect.to, "hej", objectName());
    CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, effect.to->objectName());
    reason.m_playerId = effect.from->objectName();
    reason.m_targetId = effect.to->objectName();
    room->moveCardTo(Sanguosha->getCard(card_id), effect.to, NULL, Player::DiscardPile, reason);

    LogMessage log;
    log.type = "$Dismantlement";
    log.from = effect.to;
    log.card_str = QString::number(card_id);
    room->sendLog(log);
}

Indulgence::Indulgence(Suit suit, int number)
    :DelayedTrick(suit, number)
{
    setObjectName("indulgence");
    target_fixed = false;

    judge.pattern = QRegExp("(.*):(heart):(.*)");
    judge.good = true;
    judge.reason = objectName();
}

bool Indulgence::targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const
{
    if(!targets.isEmpty())
        return false;

    if(to_select->containsTrick(objectName()))
        return false;

    if(to_select == Self)
        return false;

    return true;
}

void Indulgence::takeEffect(ServerPlayer *target) const{
    target->clearHistory();
    target->skip(Player::Play);
}

Disaster::Disaster(Card::Suit suit, int number)
    :DelayedTrick(suit, number, true)
{
    target_fixed = true;
}

bool Disaster::isAvailable(const Player *player) const{
    if(player->containsTrick(objectName()))
        return false;

    return ! player->isProhibited(player, this);
}

Lightning::Lightning(Suit suit, int number):Disaster(suit, number){
    setObjectName("lightning");

    judge.pattern = QRegExp("(.*):(spade):([2-9])");
    judge.good = false;
    judge.reason = objectName();
}

void Lightning::takeEffect(ServerPlayer *target) const{
    DamageStruct damage;
    damage.card = this;
    damage.damage = 3;
    damage.from = NULL;
    damage.to = target;
    damage.nature = DamageStruct::Thunder;

    target->getRoom()->damage(damage);
}


// EX cards

class IceSwordSkill: public WeaponSkill{
public:
    IceSwordSkill():WeaponSkill("ice_sword"){
        events << DamageCaused;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        DamageStruct damage = data.value<DamageStruct>();

        if(damage.card && damage.card->inherits("Slash") && !damage.to->isNude()
                && !damage.chain && player->askForSkillInvoke("ice_sword", data)){
            room->setEmotion(player, QString("weapon/%1").arg(objectName()));

            int card_id = room->askForCardChosen(player, damage.to, "he", "ice_sword");
            CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, damage.to->objectName());
            reason.m_playerId = damage.from->objectName();
            reason.m_targetId = damage.to->objectName();
            room->moveCardTo(Sanguosha->getCard(card_id), damage.to, NULL, Player::DiscardPile, reason);

            if(!damage.to->isNude()){
                card_id = room->askForCardChosen(player, damage.to, "he", "ice_sword");
                CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, damage.to->objectName());
                reason.m_playerId = damage.from->objectName();
                reason.m_targetId = damage.to->objectName();
                room->moveCardTo(Sanguosha->getCard(card_id), damage.to, NULL, Player::DiscardPile, reason);
            }

            return true;
        }

        return false;
    }
};

IceSword::IceSword(Suit suit, int number)
    :Weapon(suit, number, 2)
{
    setObjectName("ice_sword");
    skill = new IceSwordSkill;
}

class RenwangShieldSkill: public ArmorSkill{
public:
    RenwangShieldSkill():ArmorSkill("renwang_shield"){
        events << SlashEffected;
    }

    virtual bool trigger(TriggerEvent , Room* room, ServerPlayer *player, QVariant &data) const{
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if(effect.slash->isBlack()){
            LogMessage log;
            log.type = "#ArmorNullify";
            log.from = player;
            log.arg = objectName();
            log.arg2 = effect.slash->objectName();
            room->sendLog(log);

            room->setEmotion(player, QString("armor/%1").arg(objectName()));
            room->getThread()->delay(1200);

            return true;
        }else
            return false;
    }
};

RenwangShield::RenwangShield(Suit suit, int number)
    :Armor(suit, number)
{
    setObjectName("renwang_shield");
    skill = new RenwangShieldSkill;
}

class HorseSkill: public DistanceSkill{
public:
    HorseSkill():DistanceSkill("horse"){

    }

    virtual int getCorrect(const Player *from, const Player *to) const{
        int correct = 0;
        if(from->getOffensiveHorse())
            correct += from->getOffensiveHorse()->getCorrect();
        if(to->getDefensiveHorse())
            correct += to->getDefensiveHorse()->getCorrect();

        return correct;
    }
};

StandardCardPackage::StandardCardPackage()
    :Package("standard_cards")
{
    type = Package::CardPack;

    QList<Card*> cards;

    cards << new Slash(Card::Spade, 7)
          << new Slash(Card::Spade, 8)
          << new Slash(Card::Spade, 8)
          << new Slash(Card::Spade, 9)
          << new Slash(Card::Spade, 9)
          << new Slash(Card::Spade, 10)
          << new Slash(Card::Spade, 10)

          << new Slash(Card::Club, 2)
          << new Slash(Card::Club, 3)
          << new Slash(Card::Club, 4)
          << new Slash(Card::Club, 5)
          << new Slash(Card::Club, 6)
          << new Slash(Card::Club, 7)
          << new Slash(Card::Club, 8)
          << new Slash(Card::Club, 8)
          << new Slash(Card::Club, 9)
          << new Slash(Card::Club, 9)
          << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 10)
          << new Slash(Card::Club, 11)
          << new Slash(Card::Club, 11)

          << new Slash(Card::Heart, 10)
          << new Slash(Card::Heart, 10)
          << new Slash(Card::Heart, 11)

          << new Slash(Card::Diamond, 6)
          << new Slash(Card::Diamond, 7)
          << new Slash(Card::Diamond, 8)
          << new Slash(Card::Diamond, 9)
          << new Slash(Card::Diamond, 10)
          << new Slash(Card::Diamond, 13)

          << new Jink(Card::Heart, 2)
          << new Jink(Card::Heart, 2)
          << new Jink(Card::Heart, 13)

          << new Jink(Card::Diamond, 2)
          << new Jink(Card::Diamond, 2)
          << new Jink(Card::Diamond, 3)
          << new Jink(Card::Diamond, 4)
          << new Jink(Card::Diamond, 5)
          << new Jink(Card::Diamond, 6)
          << new Jink(Card::Diamond, 7)
          << new Jink(Card::Diamond, 8)
          << new Jink(Card::Diamond, 9)
          << new Jink(Card::Diamond, 10)
          << new Jink(Card::Diamond, 11)
          << new Jink(Card::Diamond, 11)

          << new Peach(Card::Heart, 3)
          << new Peach(Card::Heart, 4)
          << new Peach(Card::Heart, 6)
          << new Peach(Card::Heart, 7)
          << new Peach(Card::Heart, 8)
          << new Peach(Card::Heart, 9)
          << new Peach(Card::Heart, 12)

          << new Peach(Card::Diamond, 12)

          << new Crossbow(Card::Club)
          << new Crossbow(Card::Diamond)
          << new DoubleSword
          << new QinggangSword
          << new Blade
          << new Spear
          << new Axe
          << new Halberd
          << new KylinBow

          << new EightDiagram(Card::Spade)
          << new EightDiagram(Card::Club);

    skills << EightDiagramSkill::GetInstance();

    {
        QList<Card *> horses;
        horses << new DefensiveHorse(Card::Spade, 5)
               << new DefensiveHorse(Card::Club, 5)
               << new DefensiveHorse(Card::Heart, 13)
               << new OffensiveHorse(Card::Heart, 5)
               << new OffensiveHorse(Card::Spade, 13)
               << new OffensiveHorse(Card::Diamond, 13);

        horses.at(0)->setObjectName("jueying");
        horses.at(1)->setObjectName("dilu");
        horses.at(2)->setObjectName("zhuahuangfeidian");
        horses.at(3)->setObjectName("chitu");
        horses.at(4)->setObjectName("dayuan");
        horses.at(5)->setObjectName("zixing");

        cards << horses;

        skills << new HorseSkill;
    }

    cards << new AmazingGrace(Card::Heart, 3)
          << new AmazingGrace(Card::Heart, 4)
          << new GodSalvation
          << new SavageAssault(Card::Spade, 7)
          << new SavageAssault(Card::Spade, 13)
          << new SavageAssault(Card::Club, 7)
          << new ArcheryAttack
          << new Duel(Card::Spade, 1)
          << new Duel(Card::Club, 1)
          << new Duel(Card::Diamond, 1)
          << new ExNihilo(Card::Heart, 7)
          << new ExNihilo(Card::Heart, 8)
          << new ExNihilo(Card::Heart, 9)
          << new ExNihilo(Card::Heart, 11)
          << new Snatch(Card::Spade, 3)
          << new Snatch(Card::Spade, 4)
          << new Snatch(Card::Spade, 11)
          << new Snatch(Card::Diamond, 3)
          << new Snatch(Card::Diamond, 4)
          << new Dismantlement(Card::Spade, 3)
          << new Dismantlement(Card::Spade, 4)
          << new Dismantlement(Card::Spade, 12)
          << new Dismantlement(Card::Club, 3)
          << new Dismantlement(Card::Club, 4)
          << new Dismantlement(Card::Heart, 12)
          << new Collateral(Card::Club, 12)
          << new Collateral(Card::Club, 13)
          << new Nullification(Card::Spade, 11)
          << new Nullification(Card::Club, 12)
          << new Nullification(Card::Club, 13)
          << new Indulgence(Card::Spade, 6)
          << new Indulgence(Card::Club, 6)
          << new Indulgence(Card::Heart, 6)
          << new Lightning(Card::Spade, 1);

    foreach(Card *card, cards)
        card->setParent(this);

    skills << new SpearSkill << new AxeViewAsSkill;
}

StandardExCardPackage::StandardExCardPackage()
    :Package("standard_ex_cards")
{
    QList<Card *> cards;
    cards << new IceSword(Card::Spade, 2)
          << new RenwangShield(Card::Club, 2)
          << new Lightning(Card::Heart, 12)
          << new Nullification(Card::Diamond, 12);

    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

ADD_PACKAGE(StandardCard)
ADD_PACKAGE(StandardExCard)
