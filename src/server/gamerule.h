#ifndef GAMERULE_H
#define GAMERULE_H

#include "skill.h"

class GameRule : public TriggerSkill{
    Q_OBJECT

public:
    GameRule(QObject *parent);
    void setGameProcess(Room *room) const;

    virtual bool triggerable(const ServerPlayer *target) const;
    virtual int getPriority() const;
    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const;

private:
    void onPhaseChange(ServerPlayer *player) const;
    void rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const;
    void changeGeneral1v1(ServerPlayer *player) const;
    QString getWinner(ServerPlayer *victim) const;
};

class ThreeKingdomsMode: public GameRule{
    Q_OBJECT

public:
    ThreeKingdomsMode(QObject *parent);
    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const;

private:
    bool hasHeroCard(ServerPlayer *player) const;
    void sendHeroCardsToPileLog(int card, ServerPlayer *player) const;
    void sendHeroCardsToPileLog(const QList<int> &cards, ServerPlayer* player) const;
    void addHeroCardsToPile(ServerPlayer *player) const;
    void addHeroCardsFlag(ServerPlayer *player) const;
    void removeHeroCardsFlag(ServerPlayer *player) const;
};

class HulaoPassMode: public GameRule{
    Q_OBJECT

public:
    HulaoPassMode(QObject *parent);
    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const;
};

class BasaraMode: public GameRule{
    Q_OBJECT

public:
    BasaraMode(QObject *parent);

    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const;
    virtual int getPriority() const;
    void playerShowed(ServerPlayer *player) const;
    void generalShowed(ServerPlayer *player,QString general_name) const;
    static QString getMappedRole(const QString& role);

private:
    QMap<QString, QString> skill_mark;
};

#endif // GAMERULE_H
