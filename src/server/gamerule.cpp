#include "gamerule.h"
#include "serverplayer.h"
#include "room.h"
#include "standard.h"
#include "engine.h"
#include "settings.h"
#include "clientstruct.h"

#include <QTime>

GameRule::GameRule(QObject *)
    :TriggerSkill("game_rule")
{
    //@todo: this setParent is illegitimate in QT and is equivalent to calling
    // setParent(NULL). So taking it off at the moment until we figure out
    // a way to do it.
    //setParent(parent);

    events << GameStart << TurnStart << PhaseChange
           << CardUsed << TargetConfirming << TargetConfirmed << CardEffected << CardFinished
           << HpRecover << HpLost
           << DamageDone << PostHpReuced << DamageComplete
           << AskForPeaches<< AskForPeachesDone << Dying << Death << GameOverJudge
           << SlashProceed << SlashEffected << SlashHit << SlashMissed
           << StartJudge << FinishJudge
           << Pindian << PindianFinished;
}

bool GameRule::triggerable(const ServerPlayer *) const{
    return true;
}

int GameRule::getPriority() const{
    return 0;
}

void GameRule::onPhaseChange(ServerPlayer *player) const{
    Room *room = player->getRoom();
    switch(player->getPhase()){
    case Player::RoundStart:{
            break;
        }
    case Player::Start: {
            player->setMark("SlashCount", 0);
            break;
        }
    case Player::Judge: {
            QList<const DelayedTrick *> tricks = player->delayedTricks();
            while(!tricks.isEmpty() && player->isAlive()){
                const DelayedTrick *trick = tricks.takeLast();
                bool on_effect = room->cardEffect(trick, NULL, player);
                if(!on_effect)
                    trick->onNullified(player);
            }

            break;
        }
    case Player::Draw: {
            QVariant num = 2;
            if(room->getTag("FirstRound").toBool()){
                room->setTag("FirstRound", false);
                if(room->getMode() == "02_1v1")
                    num = 1;
            }

            room->getThread()->trigger(DrawNCards, room, player, num);
            int n = num.toInt();
            if(n > 0)
                player->drawCards(n, false);

            break;
        }

    case Player::Play: {
            player->clearHistory();
            while(player->isAlive()){
                CardUseStruct card_use;
                room->activate(player, card_use);
                if(card_use.isValid()){
                    room->useCard(card_use);
                }else
                    break;
            }

            break;
        }

    case Player::Discard:{
        while (player->getHandcardNum() > player->getMaxCards())
        {
            int discard_num = player->getHandcardNum() - player->getMaxCards();
            if(player->hasFlag("jilei")){
                QSet<const Card *> jilei_cards;
                QList<const Card *> handcards = player->getHandcards();
                foreach(const Card *card, handcards){
                    if(player->isJilei(card))
                        jilei_cards << card;
                }

                if(jilei_cards.size() > player->getMaxCards()){
                    // show all his cards
                    room->showAllCards(player);

                    DummyCard *dummy_card = new DummyCard;
                    foreach(const Card *card, handcards.toSet() - jilei_cards){
                        dummy_card->addSubcard(card);
                    }
                    room->throwCard(dummy_card, player);

                    return;
                }
            }

            if(discard_num > 0)
                room->askForDiscard(player, "gamerule", discard_num, 1);
        }
        break;
    }
    case Player::Finish: {

            break;
        }

    case Player::NotActive:{
            if(player->hasFlag("drank")){
                LogMessage log;
                log.type = "#UnsetDrankEndOfTurn";
                log.from = player;
                room->sendLog(log);

                room->setPlayerFlag(player, "-drank");
            }

            if(player->hasFlag("willclearCardLock"))
                room->clearPlayerCardLock(player);

            if(player->hasFlag("willclearFixDistance"))
                foreach(ServerPlayer *other, room->getOtherPlayers(player))
                    room->setFixedDistance(player, other, -1);

            room->clearPlayerFlags(player);
            player->clearHistory();

            return;
        }
    }
}

void GameRule::setGameProcess(Room *room) const{
    int good = 0, bad = 0;
    QList<ServerPlayer *> players = room->getAlivePlayers();
    foreach(ServerPlayer *player, players){
        switch(player->getRoleEnum()){
        case Player::Lord:
        case Player::Loyalist: good ++; break;
        case Player::Rebel: bad++; break;
        case Player::Renegade: break;
        }
    }

    QString process;
    if(good == bad)
        process = "Balance";
    else if(good > bad)
        process = "LordSuperior";
    else
        process = "RebelSuperior";

    room->setTag("GameProcess", process);
}

bool GameRule::trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
    if(room->getTag("SkipGameRule").toBool()){
        room->removeTag("SkipGameRule");
        return false;
    }

    // Handle global events
    if (player == NULL)
    {
        if (event == GameStart) {
            foreach (ServerPlayer* player, room->getPlayers())
            {
                if(player->getGeneral()->getKingdom() == "god" && player->getGeneralName() != "anjiang"){
                    QString new_kingdom = room->askForKingdom(player);
                    room->setPlayerProperty(player, "kingdom", new_kingdom);

                    LogMessage log;
                    log.type = "#ChooseKingdom";
                    log.from = player;
                    log.arg = new_kingdom;
                    room->sendLog(log);
                }

                if((player->hasSkill("jueqing") || player->hasSkill("shangshi")) && !player->hasSkill("#onlyfortest"))
                    room->killPlayer(player);
            }
            setGameProcess(room);
            room->setTag("FirstRound", true);
            room->drawCards(room->getPlayers(), 4, false);

        }

        return false;
    }

    switch(event){
    case TurnStart:{
            player = room->getCurrent();
            if(!player->faceUp())
                player->turnOver();
            else if(player->isAlive())
            {
                player->play();
            }

            break;
        }

    case PhaseChange: onPhaseChange(player); break;
    case CardUsed: {
            if(data.canConvert<CardUseStruct>()){
                CardUseStruct card_use = data.value<CardUseStruct>();
                const Card *card = card_use.card;

                RoomThread *thread = room->getThread();
                card_use.from->playCardEffect(card);

                ServerPlayer *target;
                QList<ServerPlayer *> targets = card_use.to;

                if(card_use.card->hasPreAction())
                    card_use.card->doPreAction(room, card_use);
                if(card_use.from && !card_use.to.empty()){
                    foreach(ServerPlayer *to, card_use.to){
                        target = to;
                        while(thread->trigger(TargetConfirming, room, target, data)){
                            CardUseStruct new_use = data.value<CardUseStruct>();
                            target = new_use.to.at(targets.indexOf(target));
                            targets = new_use.to;
                        }
                    }
                }


                card_use = data.value<CardUseStruct>();
                if(card_use.from && !card_use.to.isEmpty())
                {
                    foreach(ServerPlayer *p, room->getAlivePlayers())
                        thread->trigger(TargetConfirmed, room, p, data);
                }

                card->use(room, card_use.from, card_use.to);
            }

            break;
        }

    case TargetConfirming:{
        break;
    }
    case TargetConfirmed:{
        break;
    }

    case CardFinished: {
            CardUseStruct use = data.value<CardUseStruct>();
            room->clearCardFlag(use.card);
            break;
        }

    case HpRecover:{
            RecoverStruct recover_struct = data.value<RecoverStruct>();
            int recover = recover_struct.recover;

            room->setPlayerStatistics(player, "recover", recover);

            int new_hp = qMin(player->getHp() + recover, player->getMaxHp());
            room->setPlayerProperty(player, "hp", new_hp);
            room->broadcastInvoke("hpChange", QString("%1:%2").arg(player->objectName()).arg(recover));

            break;
        }

    case HpLost:{
            int lose = data.toInt();

            LogMessage log;
            log.type = "#LoseHp";
            log.from = player;
            log.arg = QString::number(lose);
            room->sendLog(log);

            room->setPlayerProperty(player, "hp", player->getHp() - lose);
            QString str = QString("%1:%2L").arg(player->objectName()).arg(-lose);
            room->broadcastInvoke("hpChange", str);

            if(player->getHp() <= 0)
                room->enterDying(player, NULL);

            break;
    }

    case Dying:{
            if(player->getHp() > 0){
                room->setPlayerFlag(player, "-dying");
                break;
            }

            LogMessage log;
            log.type = "#AskForPeaches";
            log.from = player;
            log.to = room->getAllPlayers();
            log.arg = QString::number(1 - player->getHp());
            room->sendLog(log);

            RoomThread *thread = room->getThread();
            foreach(ServerPlayer *saver, room->getAllPlayers()){
                if(player->getHp() > 0)
                    break;

                thread->trigger(AskForPeaches, room, saver, data);
            }

            room->setPlayerFlag(player, "-dying");
            thread->trigger(AskForPeachesDone, room, player, data);

            break;
        }

    case AskForPeaches:{
            if(room->getThread()->trigger(AskForPeachesProceed, room, player, data))
                break;

            DyingStruct dying = data.value<DyingStruct>();

            while(dying.who->getHp() <= 0){
                const Card *peach = room->askForSinglePeach(player, dying.who);
                if(peach == NULL)
                    break;

                CardUseStruct use;
                use.card = peach;
                use.from = player;
                if(player != dying.who)
                    use.to << dying.who;

                room->useCard(use, false);

                if(player != dying.who && dying.who->getHp() > 0)
                    room->setPlayerStatistics(player, "save", 1);
            }

            break;
        }

    case AskForPeachesDone:{
            if(player->getHp() <= 0 && player->isAlive()){
                DyingStruct dying = data.value<DyingStruct>();
                room->killPlayer(player, dying.damage);
            }

            break;
        }

    case DamageDone:{
            DamageStruct damage = data.value<DamageStruct>();
            room->sendDamageLog(damage);

            if(damage.from)
                room->setPlayerStatistics(damage.from, "damage", damage.damage);

            if(player->isChained() && damage.nature != DamageStruct::Normal)
            {
                damage.PreChain = true;
                room->setPlayerProperty(player, "chained", false);
                data = QVariant::fromValue(damage);
            }

            room->applyDamage(player, damage);

            break;
        }
    case PostHpReuced:{
        DamageStruct damage = data.value<DamageStruct>();
        if(player->getHp() <= 0){
            room->enterDying(player, &damage);
        }

        break;
    }
    case DamageComplete:{
            if(room->getMode() == "02_1v1" && player->isDead()){
                QString new_general = player->tag["1v1ChangeGeneral"].toString();
                if(!new_general.isEmpty())
                    changeGeneral1v1(player);
            }

            DamageStruct damage = data.value<DamageStruct>();
            if(!damage.PreChain)
                break;

            // iron chain effect

            if(!room->getTag("Chaining").toBool())
            {
                room->setTag("Chaining", true);
                QList<ServerPlayer *> chained_players;
                if(room->getCurrent()->isDead())
                    chained_players = room->getOtherPlayers(room->getCurrent());
                else
                    chained_players = room->getAllPlayers();

                foreach(ServerPlayer *chained_player, chained_players){
                    if(chained_player->isChained()){
                        room->getThread()->delay();
                        LogMessage log;
                        log.type = "#IronChainDamage";
                        log.from = chained_player;
                        room->sendLog(log);

                        DamageStruct chain_damage = damage;
                        chain_damage.to = chained_player;
                        chain_damage.chain = true;

                        room->damage(chain_damage);
                    }
                }
                room->removeTag("Chaining");
            }

            break;
        }

    case CardEffected:{
            if(data.canConvert<CardEffectStruct>()){
                CardEffectStruct effect = data.value<CardEffectStruct>();
                if(room->isCanceled(effect))
                    return true;

                effect.card->onEffect(effect);
            }

            break;
        }

    case SlashEffected:{
            SlashEffectStruct effect = data.value<SlashEffectStruct>();

            QVariant data = QVariant::fromValue(effect);
            room->getThread()->trigger(SlashProceed, room, effect.from, data);

            break;
        }

    case SlashProceed:{
            SlashEffectStruct effect = data.value<SlashEffectStruct>();

            QString slasher = effect.from->objectName();

            const Card *jink;
            if(effect.to->hasFlag("dahe")){
                ServerPlayer *bgm_zhangfei = room->findPlayerBySkillName("dahe", true);
                jink = room->askForCard(effect.to, "jink",
                                        QString("@dahe-jink:%1:%2:%3")
                                        .arg(slasher)
                                        .arg(bgm_zhangfei->objectName())
                                        .arg("dahe"),
                                        data,
                                        CardUsed, effect.from);
            }
            else
                jink = room->askForCard(effect.to, "jink", "slash-jink:" + slasher, data, CardUsed, effect.from);
            room->slashResult(effect, jink);

            break;
        }

    case SlashHit:{
            SlashEffectStruct effect = data.value<SlashEffectStruct>();

            DamageStruct damage;
            damage.card = effect.slash;

            damage.damage = 1;
            if(effect.drank){
                LogMessage log;
                log.type = "#AnalepticBuff";
                log.from = effect.from;
                log.to << effect.to;
                log.arg = "analeptic";
                room->sendLog(log);

                damage.damage++;
            }

            damage.from = effect.from;
            damage.to = effect.to;
            damage.nature = effect.nature;
            room->damage(damage);

            effect.to->removeMark("qinggang");

            break;
        }

    case SlashMissed:{
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            effect.to->removeMark("qinggang");

            break;
        }

    case GameOverJudge:{
            if(room->getMode() == "02_1v1"){
                QStringList list = player->tag["1v1Arrange"].toStringList();

                if(!list.isEmpty())
                    return false;
            }

            QString winner = getWinner(player);
            if(!winner.isNull()){
                player->bury();
                room->gameOver(winner);
                return true;
            }

            break;
        }

    case Death:{
            player->bury();

            if(room->getTag("SkipNormalDeathProcess").toBool())
                return false;

            DamageStar damage = data.value<DamageStar>();
            ServerPlayer *killer = damage ? damage->from : NULL;
            if(killer){
                rewardAndPunish(killer, player);
            }

            setGameProcess(room);

            if(room->getMode() == "02_1v1"){
                QStringList list = player->tag["1v1Arrange"].toStringList();

                if(!list.isEmpty()){
                    player->tag["1v1ChangeGeneral"] = list.takeFirst();
                    player->tag["1v1Arrange"] = list;

                    DamageStar damage = data.value<DamageStar>();

                    if(damage == NULL){
                        changeGeneral1v1(player);
                        return false;
                    }
                }
            }

            break;
        }

    case StartJudge:{
            int card_id = room->drawCard();

            JudgeStar judge = data.value<JudgeStar>();
            judge->card = Sanguosha->getCard(card_id);
            room->moveCardTo(judge->card, NULL, NULL, Player::TopDrawPile,
                            CardMoveReason(CardMoveReason::S_REASON_JUDGE, judge->who->objectName(), QString(), QString(), judge->reason), true);
            LogMessage log;
            log.type = "$InitialJudge";
            log.from = player;
            log.card_str = judge->card->getEffectIdString();
            room->sendLog(log);

            room->sendJudgeResult(judge);

            int delay = Config.AIDelay;
            if(judge->time_consuming)
                delay /= 4;
            room->getThread()->delay(delay);

            break;
        }

    case FinishJudge:{
            JudgeStar judge = data.value<JudgeStar>();

            LogMessage log;
            log.type = "$JudgeResult";
            log.from = player;
            log.card_str = judge->card->getEffectIdString();
            room->sendLog(log);

            room->sendJudgeResult(judge);

            room->getThread()->delay();

            room->sendJudgeResult(judge);

            if(room->getCardPlace(judge->card->getEffectiveId()) == Player::TopDrawPile){
              CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, judge->who->objectName(), QString(), QString());
              room->throwCard(judge->card, reason, judge->who);
            }

            break;
        }

    case Pindian:{
            PindianStar pindian = data.value<PindianStar>();

            LogMessage log;


            CardMoveReason reason(CardMoveReason::S_REASON_PINDIAN, pindian->from->objectName(), pindian->to->objectName(),
                                  pindian->reason, QString());
            room->moveCardTo(pindian->from_card, pindian->from, NULL, Player::DealingArea, reason);

            log.type = "$PindianResult";
            log.from = pindian->from;
            log.card_str = pindian->from_card->getEffectIdString();
            room->sendLog(log);
            room->getThread()->delay();

            CardMoveReason reason2(CardMoveReason::S_REASON_PINDIAN, pindian->to->objectName());
            room->moveCardTo(pindian->to_card, pindian->to, NULL, Player::DealingArea, reason2);

            log.type = "$PindianResult";
            log.from = pindian->to;
            log.card_str = pindian->to_card->getEffectIdString();
            room->sendLog(log);
            room->getThread()->delay();

            break;
        }
    case PindianFinished:{
        PindianStar pindian = data.value<PindianStar>();
        CardMoveReason reason(CardMoveReason::S_REASON_PINDIAN, pindian->from->objectName(), pindian->to->objectName(),
                              pindian->reason, QString());
        if (room->getCardPlace(pindian->from_card->getEffectiveId()) == Player::DealingArea)
            room->moveCardTo(pindian->from_card, NULL, NULL, Player::DiscardPile, reason);

        CardMoveReason reason2(CardMoveReason::S_REASON_PINDIAN, pindian->to->objectName());
        if (room->getCardPlace(pindian->to_card->getEffectiveId()) == Player::DealingArea)
            room->moveCardTo(pindian->to_card, NULL, NULL, Player::DiscardPile, reason2);
    }

    default:
        ;
    }

    return false;
}

void GameRule::changeGeneral1v1(ServerPlayer *player) const{
    Room *room = player->getRoom();
    QString new_general = player->tag["1v1ChangeGeneral"].toString();
    player->tag.remove("1v1ChangeGeneral");

    QSet<QString> skills = player->getAdditionalSkills();
    if(!skills.isEmpty())
        foreach(QString skill, skills.toList())
            room->detachSkillFromPlayer(player, skill);

    room->revivePlayer(player);
    room->transfigure(player, new_general, true, true);
    if(player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    room->broadcastInvoke("revealGeneral",
                          QString("%1:%2").arg(player->objectName()).arg(new_general),
                          player);


    if(!player->faceUp())
        player->turnOver();

    if(player->isChained())
        room->setPlayerProperty(player, "chained", false);

    player->tag["FirstDraw"] = true;
    player->drawCards(4);
    player->tag.remove("FirstDraw");
}

void GameRule::rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const{
    if(killer->isDead())
        return;

    if(killer->getRoom()->getMode() == "06_3v3"){
        if(Config.value("3v3/UsingNewMode", false).toBool())
            killer->drawCards(2);
        else
            killer->drawCards(3);
    }
    else{
        if(victim->getRole() == "rebel" && killer != victim){
            killer->drawCards(3);
        }else if(victim->getRole() == "loyalist" && killer->getRole() == "lord"){
            killer->throwAllHandCards();
            killer->throwAllEquips();
        }
    }
}

QString GameRule::getWinner(ServerPlayer *victim) const{
    Room *room = victim->getRoom();
    QString winner;

    if(room->getMode() == "06_3v3"){
        switch(victim->getRoleEnum()){
        case Player::Lord: winner = "renegade+rebel"; break;
        case Player::Renegade: winner = "lord+loyalist"; break;
        default:
            break;
        }
    }else if(Config.EnableHegemony){
        bool has_anjiang = false, has_diff_kingdoms = false;
        QString init_kingdom;
        foreach(ServerPlayer *p, room->getAlivePlayers()){
            if(room->getTag(p->objectName()).toStringList().size()){
                has_anjiang = true;
            }

            if(init_kingdom.isEmpty()){
                init_kingdom = p->getKingdom();
            }
            else if(init_kingdom != p->getKingdom()){
                has_diff_kingdoms = true;
            }
        }

        if(!has_anjiang && !has_diff_kingdoms){
            QStringList winners;
            QString aliveKingdom = room->getAlivePlayers().first()->getKingdom();
            foreach(ServerPlayer *p, room->getPlayers()){
                if(p->isAlive())winners << p->objectName();
                if(p->getKingdom() == aliveKingdom)
                {
                    QStringList generals = room->getTag(p->objectName()).toStringList();
                    if(generals.size()&&!Config.Enable2ndGeneral)continue;
                    if(generals.size()>1)continue;

                    //if someone showed his kingdom before death,
                    //he should be considered victorious as well if his kingdom survives
                    winners << p->objectName();
                }
            }

            winner = winners.join("+");
        }
    }else{
        QStringList alive_roles = room->aliveRoles(victim);
        switch(victim->getRoleEnum()){
        case Player::Lord:{
                if(alive_roles.length() == 1 && alive_roles.first() == "renegade")
                    winner = room->getAlivePlayers().first()->objectName();
                else
                    winner = "rebel";
                break;
            }

        case Player::Rebel:
        case Player::Renegade:
            {
                if(!alive_roles.contains("rebel") && !alive_roles.contains("renegade")){
                    winner = "lord+loyalist";
                    if(victim->getRole() == "renegade" && !alive_roles.contains("loyalist"))
                        room->setTag("RenegadeInFinalPK", true);
                }
                break;
            }

        default:
            break;
        }
    }

    return winner;
}

ThreeKingdomsMode::ThreeKingdomsMode(QObject *parent)
    :GameRule(parent)
{
    setObjectName("threekingdoms_mode");
    events << CardEffect << CardDrawnDone << CardGotOnePiece << CardLostOnePiece;
}

bool ThreeKingdomsMode::trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
    switch(event) {
    case GameStart: {
        // Handle global events
        if (player == NULL)
        {
            room->setTag("FirstRound", true);
            room->drawCards(room->getPlayers(), 4, false);
            foreach(ServerPlayer *player, room->getPlayers())
            {
                room->acquireSkill(player, "usehero", true, false);
                room->acquireSkill(player, "herorecast", true, false);
                if (hasHeroCard(player)){
                    room->askForUseCard(player, "@prepare", "#prepare");
                    room->getThread()->delay();
                    addHeroCardsToPile(player);
                }
            }
            return false;
        }
        break;
    }

    case PhaseChange: {
        switch(player->getPhase()){
        case Player::Discard:{
            GameRule::trigger(event, room, player, data);
            QList<int> heros = player->getPile("heros");
            heros.removeOne(player->getMark("hero"));

            int willthrow = 2;

            while(heros.count() > willthrow)
            {
                room->fillAG(heros, player);
                int card_id = room->askForAG(player, heros, false, "throwhero");
                heros.removeOne(card_id);
                player->invoke("clearAG");
                room->setCardFlag(card_id, "-justdraw");
                room->throwCard(card_id, player);
            }
            return false;
        }
        case Player::NotActive:{
            removeHeroCardsFlag(player);
            break;
        }
        default:
            break;
        }
        break;
    }

    case CardDrawnDone:{
        if(room->getTag("FirstRound").toBool())
            break;

        if(player->getPhase() == Player::Draw)
            if(hasHeroCard(player)){
                addHeroCardsFlag(player);
                room->getThread()->delay();
            }
        addHeroCardsToPile(player);
        break;
    }

    case CardEffect:{
        if(data.canConvert<CardEffectStruct>()){
            CardEffectStruct effect = data.value<CardEffectStruct>();

            if (effect.card->inherits("Dismantlement"))
            {
                ServerPlayer *target = effect.to;
                QList<int> heros = player->getPile("heros");
                heros.removeOne(player->getMark("hero"));
                if(heros.isEmpty() || target->getPile("heros").isEmpty())
                    break;

                QString choice ;
                if(target->isNude())
                    choice = "throw";
                else
                    choice = room->askForChoice(player, objectName(), "normal+throw");

                if(choice == "normal")
                    break;

                if(room->isCanceled(effect))
                    return true;

                int hero = getPlayerHero(room, player, target, "throwTargetHero", false);

                if(hero == -1)
                    return true;

                int heroTarget = hero;

                heros = player->getPile("heros");
                heros.removeOne(player->getMark("hero"));

                room->fillAG(heros, player);
                hero = room->askForAG(player, heros, true, "throwSelfHero");
                player->invoke("clearAG");
                if(hero == -1)
                    hero = heros.at(qrand() % heros.length());

                room->setCardFlag(hero, "-justdraw");
                room->setCardFlag(heroTarget, "-justdraw");
                CardMoveReason reason(CardMoveReason::S_REASON_DISMANTLE, effect.to->objectName());
                reason.m_playerId = player->objectName();
                reason.m_targetId = target->objectName();
                room->throwCard(hero, player);
                room->throwCard(Sanguosha->getCard(heroTarget), reason, target);
                return true;
            }

            if (ServerInfo.EnableSnatchHero && effect.card->inherits("Snatch"))
            {
                ServerPlayer *target = effect.to;
                if(player->getMark("hero") == 0 || target->getPile("heros").isEmpty())
                    break;

                QString choice ;
                if(target->isNude())
                    choice = "snatch";
                else
                    choice = room->askForChoice(player, "SnatchTargetHero", "normal+snatch");

                if(choice == "normal")
                    break;

                if(room->isCanceled(effect))
                    return true;

                int hero = getPlayerHero(room, player, target, "throwTargetHero", false);

                if(hero == -1)
                    return true;

                if(player->getPile("heros").contains(player->getMark("hero")))
                    room->throwCard(player->getMark("hero"), player);

                const Card *heroCard = Sanguosha->getCard(hero);
                player->addToPile("heros", heroCard, false);
                player->fillHero();

                LogMessage log;
                log.from = player;
                log.type = "#EquipedHeroCard";
                log.arg = heroCard->objectName();
                room->sendLog(log);
                room->broadcastInvoke("playAudio", "equiphero");

                room->setPlayerMark(player, "hero", heroCard->getEffectiveId());
                room->transfigure(player, heroCard->objectName(), false);
                room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
                player->fillHero();


                return true;
            }
        }
        break;
    }

    case CardGotOnePiece:{
        CardMoveStar move = data.value<CardMoveStar>();
        const Card *card = Sanguosha->getCard(move->card_id);
        if(card->inherits("HeroCard"))
		{
            player->addToPile("heros", card, false);
            player->fillHero();
        }

        break;
    }

    case CardLostOnePiece:{
        if(!data.canConvert<CardMoveStar>())
            break;

        CardMoveStar move = data.value<CardMoveStar>();
        const Card *card = Sanguosha->getCard(move->card_id);
        if(card->inherits("HeroCard") && move->from_place == Player::PlaceSpecial)
        {
            player->fillHero();
            if((move->card_id == player->getMark("hero") || player->getPile("heros").isEmpty())
                    && player->getGeneralName() != "anjiang")
            {
                LogMessage log;
                log.from = player;
                log.type = "#LoseHeroCard";
                log.arg = player->getGeneralName();
                log.arg2 = "anjiang";
                room->sendLog(log);
                room->setPlayerMark(player, "hero", 0);
                room->transfigure(player, "anjiang", false);
                room->setPlayerProperty(player, "kingdom", "god");
                player->throwAllMarks();
                player->clearPrivatePilesExcept("heros");

                QSet<QString>skills = player->getAdditionalSkills();
                static QSet<QString>except;
                if(except.isEmpty())
                {
                    except << "usehero" << "herorecast";
                }
                skills.subtract(except);
                if(!skills.isEmpty())
                {
                    foreach(QString askill, skills.toList())
                    {
                        room->detachSkillFromPlayer(player, askill);
                    }
                }

                room->setPlayerFlag(player, "willclearCardLock");
                room->setPlayerFlag(player, "willclearFixDistance");
                if(player->getHp() <= 0 && player->isAlive())
                    room->enterDying(player, NULL);
            }
        }

        break;
    }

    case StartJudge:{
        JudgeStar judge = data.value<JudgeStar>();

        int delay = Config.AIDelay;
        if(judge->time_consuming)
            delay /= 4;

        do
        {
            int card_id = room->drawCard();
            const Card *judgeCard = Sanguosha->getCard(card_id);
            judge->card = judgeCard;
            if(judgeCard->inherits("HeroCard"))
                room->moveCardTo(judgeCard, NULL, Player::DiscardPile);
            else
                room->moveCardTo(judge->card, NULL, NULL, Player::TopDrawPile,
                                CardMoveReason(CardMoveReason::S_REASON_JUDGE, judge->who->objectName(), QString(), QString(), judge->reason), true);
            room->getThread()->delay(delay);
        }while(judge->card->inherits("HeroCard"));

        LogMessage log;
        log.type = "$InitialJudge";
        log.from = player;
        log.card_str = judge->card->getEffectIdString();
        room->sendLog(log);

        room->sendJudgeResult(judge);

        return false;
    }

    case GameOverJudge: {
        player->bury();
        QString winner = player->getNextAlive()->getNextAlive()->getRole();
        room->gameOver(winner);
        return true;
    }

    default:
        break;
    }

    return GameRule::trigger(event, room, player, data);
}

bool ThreeKingdomsMode::hasHeroCard(ServerPlayer *player) const{
    QList<const Card*> cards = player->getHandcards();
    foreach(const Card *card, cards){
        if (card->inherits("HeroCard"))
            return true;
    }
    return false;
}

void ThreeKingdomsMode::addHeroCardsToPile(ServerPlayer *player) const{
    QList<int> hero_card_ids;

    LogMessage log;
    log.from = player;
    log.type = "$AddHeroCardsToPile";

    foreach(int id, player->handCards()){
        const Card *card = Sanguosha->getCard(id);
        if (card->inherits("HeroCard"))
            hero_card_ids << id;
    }
    if(!hero_card_ids.isEmpty())
	{
        player->addToPile("heros", hero_card_ids, false);
    }
    player->fillHero();
}

void ThreeKingdomsMode::addHeroCardsFlag(ServerPlayer *player) const{
    foreach(int id, player->handCards()){
        const Card *card = Sanguosha->getCard(id);
        if (card->inherits("HeroCard"))
            player->getRoom()->setCardFlag(id, "justdraw");
    }
}

void ThreeKingdomsMode::removeHeroCardsFlag(ServerPlayer *player) const{
    foreach(int id, player->getPile("heros")){
        const Card *card = Sanguosha->getCard(id);
        if (card->hasFlag("justdraw"))
            player->getRoom()->setCardFlag(id, "-justdraw");
    }
}

int ThreeKingdomsMode::getPlayerHero(Room *room, ServerPlayer *player, ServerPlayer *who, const QString &reason, bool open) const{
    QList<int> heros = who->getPile("heros");
    if(heros.isEmpty())
        return -1;
    if (open)
    {
        room->fillAG(heros, player);
        int hero = room->askForAG(player, heros, false, reason);
        player->invoke("clearAG");
        return hero;
    }
    else
    {
        int hero = who->getMark("hero");
        bool equiped = hero > 0;
        int unkown = Sanguosha->getCardCount() - 1;
        QList<int> newlist;
        if(equiped)
        {
            newlist << hero;
            for(int i = 0; i < heros.length() - 1; i++)
                newlist << unkown;
            room->fillAG(newlist, player);
            int ahero = room->askForAG(player, newlist, true, reason);
            player->invoke("clearAG");
            if(ahero == -1)
                ahero = heros.at(qrand() % heros.length());
            if (ahero == unkown)
            {
                heros.removeOne(hero);
                return heros.at(qrand() % heros.length());
            }
            else
                return ahero;
        }
        else
        {
            for(int i = 0; i < heros.length(); i++)
                newlist << unkown;
            room->fillAG(newlist, player);
            room->askForAG(player, newlist, true, reason);
            player->invoke("clearAG");
            return heros.at(qrand() % heros.length());
        }
    }
    return -1;
}

HulaoPassMode::HulaoPassMode(QObject *parent)
    :GameRule(parent)
{
    setObjectName("hulaopass_mode");

    events << HpChanged << StageChange;
    default_choice = "recover";
}

bool HulaoPassMode::trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
    switch(event) {
    case StageChange: {
        ServerPlayer* lord = room->getLord();
        room->transfigure(lord, "shenlvbu2", true, true);

        QList<const Card *> tricks = lord->getJudgingArea();
        foreach(const Card *trick, tricks)
        {
            CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
            room->throwCard(trick, reason, NULL);
        }

        if(room->alivePlayerCount() == room->getPlayers().count() && lord->getWeapon()
                && lord->askForSkillInvoke("weapon_recast"))
        {
            lord->playCardEffect("@recast");
            CardMoveReason reason(CardMoveReason::S_REASON_RECAST, player->objectName());
            room->throwCard(lord->getWeapon(), reason, NULL);
            lord->drawCards(1, false);
        }


        break;
    }

    case GameStart: {
        // Handle global events
        if (player == NULL)
        {
            ServerPlayer* lord = room->getLord();
            lord->drawCards(8, false);
            foreach (ServerPlayer* player, room->getPlayers())
            {
                if(player->isLord())
                    continue;
                else
                    player->drawCards(player->getSeat() + 1, false);
            }
            return false;
        }
        break;
    }

    case CardUsed:{
        CardUseStruct use = data.value<CardUseStruct>();
        if(use.card->inherits("Weapon") && player->askForSkillInvoke("weapon_recast", data)){
            player->playCardEffect("@recast");
            CardMoveReason reason(CardMoveReason::S_REASON_RECAST, player->objectName());
            room->throwCard(use.card, reason, NULL);
            player->drawCards(1, false);
            return false;
        }

        break;
    }

    case HpChanged:{
            if(player->getGeneralName() == "shenlvbu1" && player->getHp() <= 4){
                throw StageChange;
            }

            return false;
        }

    case Death:{
            if(player->isLord()){
                room->gameOver("rebel");
            }else{
                if(room->aliveRoles(player).length() == 1)
                    room->gameOver("lord");

                LogMessage log;
                log.type = "#Reforming";
                log.from = player;
                room->sendLog(log);

                player->bury();
                room->setPlayerProperty(player, "hp", 0);

                foreach(ServerPlayer *player, room->getOtherPlayers(room->getLord())){
                    if(player->askForSkillInvoke("draw_1v3"))
                        player->drawCards(1, false);
                }
            }

            return false;
        }

    case TurnStart:{
            if(player->isLord()){
                if(!player->faceUp())
                    player->turnOver();
                else
                {
                    player->play();
                }
            }else{
                if(player->isDead()){
                    if(player->getHp() + player->getHandcardNum() == 6){
                        LogMessage log;
                        log.type = "#ReformingRevive";
                        log.from = player;
                        room->sendLog(log);

                        room->revivePlayer(player);
                    }else if(player->isWounded()){
                        if(player->getHp() > 0 && (room->askForChoice(player, "Hulaopass", "recover+draw") == "draw")){
                            LogMessage log;
                            log.type = "#ReformingDraw";
                            log.from = player;
                            room->sendLog(log);
                            player->drawCards(1, false);
                            return false;
                        }

                        LogMessage log;
                        log.type = "#ReformingRecover";
                        log.from = player;
                        room->sendLog(log);

                        room->setPlayerProperty(player, "hp", player->getHp() + 1);
                    }else
                        player->drawCards(1, false);
                }else if(!player->faceUp())
                    player->turnOver();
                else
                {
                    player->play();
                }
            }

            return false;
        }

    default:
        break;
    }

    return GameRule::trigger(event, room, player, data);
}

BasaraMode::BasaraMode(QObject *parent)
    :GameRule(parent)
{
    setObjectName("basara_mode");

    events << CardLostOnePiece << DamageInflicted;

    skill_mark["niepan"] = "@nirvana";
    skill_mark["smallyeyan"] = "@flame";
    skill_mark["luanwu"] = "@chaos";
    skill_mark["fuli"] = "@laoji";
    skill_mark["zuixiang"] = "@sleep";
}

QString BasaraMode::getMappedRole(const QString &role){
    static QMap<QString, QString> roles;
    if(roles.isEmpty()){
        roles["wei"] = "lord";
        roles["shu"] = "loyalist";
        roles["wu"] = "rebel";
        roles["qun"] = "renegade";
    }
    return roles[role];
}

int BasaraMode::getPriority() const
{
    return 5;
}

void BasaraMode::playerShowed(ServerPlayer *player) const{
    Room *room = player->getRoom();
    QStringList names = room->getTag(player->objectName()).toStringList();
    if(names.isEmpty())
        return;

    if(Config.EnableHegemony){
        QMap<QString, int> kingdom_roles;
        foreach(ServerPlayer *p, room->getOtherPlayers(player)){
            kingdom_roles[p->getKingdom()]++;
        }

        if(kingdom_roles[Sanguosha->getGeneral(names.first())->getKingdom()] >= 2
                && player->getGeneralName() == "anjiang")
            return;
    }

    QString answer = room->askForChoice(player, "RevealGeneral", "yes+no");
    if(answer == "yes"){

        QString general_name = room->askForGeneral(player,names);

        generalShowed(player,general_name);
        if(Config.EnableHegemony)room->getThread()->trigger(GameOverJudge, room, player);
        playerShowed(player);
    }
}

void BasaraMode::generalShowed(ServerPlayer *player, QString general_name) const
{
    Room * room = player->getRoom();
    QStringList names = room->getTag(player->objectName()).toStringList();
    if(names.isEmpty())return;

    if(player->getGeneralName() == "anjiang")
    {
        QString transfigure_str = QString("%1:%2").arg(player->getGeneralName()).arg(general_name);
        player->invoke("transfigure", transfigure_str);
        room->setPlayerProperty(player,"general",general_name);
        foreach(QString skill_name, skill_mark.keys()){
            if(player->hasSkill(skill_name))
                room->setPlayerMark(player, skill_mark[skill_name], 1);
        }
    }
    else{
        QString transfigure_str = QString("%1:%2").arg(player->getGeneral2Name()).arg(general_name);
        player->invoke("transfigure", transfigure_str);
        room->setPlayerProperty(player,"general2",general_name);
    }

    room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
    if(Config.EnableHegemony)room->setPlayerProperty(player, "role", getMappedRole(player->getGeneral()->getKingdom()));

    names.removeOne(general_name);
    room->setTag(player->objectName(),QVariant::fromValue(names));

    LogMessage log;
    log.type = "#BasaraReveal";
    log.from = player;
    log.arg  = player->getGeneralName();
    log.arg2 = player->getGeneral2Name();

    room->sendLog(log);
    room->broadcastInvoke("playAudio","choose-item");
}

bool BasaraMode::trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const{
    if (player != NULL){
        player->tag["event"] = event;
        player->tag["event_data"] = data;
    }

    // Handle global events
    if (player == NULL)
    {
        if (event == GameStart)
        {
            if(Config.EnableHegemony)
                room->setTag("SkipNormalDeathProcess", true);
            foreach(ServerPlayer* sp, room->getAlivePlayers())
            {
                QString transfigure_str = QString("%1:%2").arg(sp->getGeneralName()).arg("anjiang");
                sp->invoke("transfigure", transfigure_str);
                room->setPlayerProperty(sp,"general","anjiang");
                room->setPlayerProperty(sp,"kingdom","god");

                LogMessage log;
                log.type = "#BasaraGeneralChosen";
                log.arg = room->getTag(sp->objectName()).toStringList().at(0);

                if(Config.Enable2ndGeneral)
                {

                    transfigure_str = QString("%1:%2").arg(sp->getGeneral2Name()).arg("anjiang");
                    sp->invoke("transfigure", transfigure_str);
                    room->setPlayerProperty(sp,"general2","anjiang");

                    log.arg2 = room->getTag(sp->objectName()).toStringList().at(1);
                }

                sp->invoke("log",log.toString());
                sp->tag["roles"] = room->getTag(sp->objectName()).toStringList().join("+");
            }
        }
    }

    switch(event){
    case CardEffected:{
        if(player->getPhase() == Player::NotActive){
            CardEffectStruct ces = data.value<CardEffectStruct>();
            if(ces.card)
                if(ces.card->inherits("TrickCard") ||
                        ces.card->inherits("Slash"))
                playerShowed(player);

            const ProhibitSkill* prohibit = room->isProhibited(ces.from,ces.to,ces.card);
            if(prohibit)
            {
                LogMessage log;
                log.type = "#SkillAvoid";
                log.from = ces.to;
                log.arg  = prohibit->objectName();
                log.arg2 = ces.card->objectName();

                room->sendLog(log);

                return true;
            }
        }
        break;
    }

    case PhaseChange:{
        if(player->getPhase() == Player::Start)
            playerShowed(player);

        break;
    }
    case DamageInflicted:{
        playerShowed(player);
        break;
    }
    case GameOverJudge:{
        if(Config.EnableHegemony){
            if(player->getGeneralName() == "anjiang"){
                QStringList generals = room->getTag(player->objectName()).toStringList();
                room->setPlayerProperty(player, "general", generals.at(0));
                if(Config.Enable2ndGeneral)room->setPlayerProperty(player, "general2", generals.at(1));
                room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());
                room->setPlayerProperty(player, "role", getMappedRole(player->getKingdom()));
            }
        }
        break;
    }

    case Death:{
        if(Config.EnableHegemony){
            DamageStar damage = data.value<DamageStar>();
            ServerPlayer *killer = damage ? damage->from : NULL;
            if(killer && killer->getKingdom() == damage->to->getKingdom()){
                killer->throwAllEquips();
                killer->throwAllHandCards();
            }
            else if(killer && killer->isAlive()){
                killer->drawCards(3);
            }
        }

        break;
    }

    default:
        break;
    }

    return false;
}
