#ifndef STRUCTS_H
#define STRUCTS_H

class Room;
class TriggerSkill;
class ServerPlayer;
class Card;
class Slash;
class GameRule;

#include "player.h"

#include <QVariant>
#include <json/json.h>

struct DamageStruct{
    DamageStruct();

    enum Nature{
        Normal, // normal slash, duel and most damage caused by skill
        Fire,  // fire slash, fire attack and few damage skill (Yeyan, etc)
        Thunder // lightning, thunder slash, and few damage skill (Leiji, etc)
    };

    ServerPlayer *from;
    ServerPlayer *to;
    const Card *card;
    int damage;
    Nature nature;
    bool chain;
};

struct CardEffectStruct{
    CardEffectStruct();

    const Card *card;

    ServerPlayer *from;
    ServerPlayer *to;

    bool multiple;
};

struct SlashEffectStruct{
    SlashEffectStruct();

    const Slash *slash;
    const Card *jink;

    ServerPlayer *from;
    ServerPlayer *to;

    bool drank;

    DamageStruct::Nature nature;
};

struct CardUseStruct{
    CardUseStruct();
    bool isValid() const;
    void parse(const QString &str, Room *room);
    bool tryParse(const Json::Value&, Room *room);

    const Card *card;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
};

struct CardsMoveOneTimeStruct{
    QList<int> card_ids;
    QList<Player::Place> from_places;
    Player::Place to_place;
    Player *from, *to;
};

struct CardMoveStruct{
    inline CardMoveStruct()
    {
        from_place = Player::PlaceUnknown;
        to_place = Player::PlaceUnknown;
        from = NULL;
        to = NULL;
    }
    int card_id;
    Player::Place from_place, to_place;
    QString from_player_name, to_player_name;
    QString from_pile_name, to_pile_name;
    Player *from, *to;
    bool open;
    bool tryParse(const Json::Value&);
    Json::Value toJsonValue() const;
    inline bool isRelevant(Player* player)
    {
        return (player != NULL && (from == player || to == player));
    }
    inline bool hasSameSourceAs(const CardMoveStruct &move)
    {
        return (from == move.from) && (from_place == move.from_place) &&
               (from_player_name == move.from_player_name) && (from_pile_name == move.from_pile_name);
    }
    inline bool hasSameDestinationAs(const CardMoveStruct &move)
    {
        return (to == move.to) && (to_place == move.to_place) &&
               (to_player_name == move.to_player_name) && (to_pile_name == move.to_pile_name);
    }
};

struct CardsMoveStruct{
    inline CardsMoveStruct()
    {
        from_place = Player::PlaceUnknown;
        to_place = Player::PlaceUnknown;
        from = NULL;
        to = NULL;
        countAsOneTime = false;
    }
    inline CardsMoveStruct(const QList<int> &ids, Player* to, Player::Place to_place)
    {
        this->card_ids = ids;
        this->from_place = Player::PlaceUnknown;
        this->to_place = to_place;
        this->from = NULL;
        this->to = to;
    }
    inline bool hasSameSourceAs(const CardsMoveStruct &move)
    {
        return (from == move.from) && (from_place == move.from_place) &&
               (from_player_name == move.from_player_name) && (from_pile_name == move.from_pile_name);
    }
    inline bool hasSameDestinationAs(const CardsMoveStruct &move)
    {
        return (to == move.to) && (to_place == move.to_place) &&
               (to_player_name == move.to_player_name) && (to_pile_name == move.to_pile_name);
    }
    QList<int> card_ids;
    Player::Place from_place, to_place;
    QString from_player_name, to_player_name;
    QString from_pile_name, to_pile_name;
    Player *from, *to;
    bool open; // helper to prevent sending card_id to unrelevant clients
    bool countAsOneTime; // helper to identify distinct move counted as one time
    bool tryParse(const Json::Value&);
    Json::Value toJsonValue() const;
    inline bool isRelevant(const Player* player)
    {
        return (player != NULL && (from == player || to == player));
    }
    QList<CardMoveStruct> flatten();
};

struct DyingStruct{
    DyingStruct();

    ServerPlayer *who; // who is ask for help
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
    QList<ServerPlayer *> savers; // savers are the available players who can use peach for the dying player
};

struct RecoverStruct{
    RecoverStruct();

    int recover;
    ServerPlayer *who;
    const Card *card;
};

struct PindianStruct{
    PindianStruct();
    bool isSuccess() const;

    ServerPlayer *from;
    ServerPlayer *to;
    const Card *from_card;
    const Card *to_card;
    QString reason;
};

class JudgeStructPattern{
private:
    QString pattern;
    bool isRegex;

public:
    JudgeStructPattern();
    JudgeStructPattern &operator=(const QRegExp &rx);
    JudgeStructPattern &operator=(const QString &str);
    bool match(const Player *player, const Card *card) const;
};

struct JudgeStruct{
    JudgeStruct();
    bool isGood(const Card *card = NULL) const;
    bool isBad() const;

    ServerPlayer *who;
    const Card *card;
    JudgeStructPattern pattern;
    bool good;
    QString reason;
    bool time_consuming;
};

struct PhaseChangeStruct{
    PhaseChangeStruct();
    Player::Phase from;
    Player::Phase to;
};

enum TriggerEvent{
    NonTrigger,

    GameStart,
    TurnStart,
    PhaseChange,
    DrawNCards,
    HpRecover,
    HpLost,
    HpChanged,

    StartJudge,
    AskForRetrial,
    FinishJudge,

    Pindian,
    PindianFinished,
    TurnedOver,

    Predamage,
    DamagedProceed,
    DamageProceed,
    Predamaged,
    DamageDone,
    Damage,
    Damaged,
    DamageComplete,

    Dying,
    AskForPeaches,
    AskForPeachesDone,
    Death,
    GameOverJudge,
    GameFinished,

    SlashEffect,
    SlashEffected,
    SlashProceed,
    SlashHit,
    SlashMissed,

    JinkUsed,

    CardAsked,
    CardonUse,
    CardUsed,
    CardResponsed,
    CardDiscarded,
    CardLostOnePiece,
    CardLostOneTime,
    CardGotOnePiece,
    CardGotOneTime,
    CardDrawing,
    CardDrawnDone,

    CardEffect,
    CardEffected,
    CardFinished,

    ChoiceMade,

    // For hulao pass only
    StageChange,

    NumOfEvents
};

typedef const Card *CardStar;
typedef ServerPlayer *PlayerStar;
typedef JudgeStruct *JudgeStar;
typedef DamageStruct *DamageStar;
typedef PindianStruct *PindianStar;
typedef const CardMoveStruct *CardMoveStar;
typedef const CardsMoveOneTimeStruct *CardsMoveOneTimeStar;
typedef const CardsMoveStruct *CardsMoveStar;

Q_DECLARE_METATYPE(DamageStruct)
Q_DECLARE_METATYPE(CardEffectStruct)
Q_DECLARE_METATYPE(SlashEffectStruct)
Q_DECLARE_METATYPE(CardUseStruct)
Q_DECLARE_METATYPE(CardsMoveStruct)
Q_DECLARE_METATYPE(CardsMoveStar)
Q_DECLARE_METATYPE(CardsMoveOneTimeStar)
Q_DECLARE_METATYPE(CardMoveStruct)
Q_DECLARE_METATYPE(CardMoveStar)
Q_DECLARE_METATYPE(CardStar)
Q_DECLARE_METATYPE(PlayerStar)
Q_DECLARE_METATYPE(DyingStruct)
Q_DECLARE_METATYPE(RecoverStruct)
Q_DECLARE_METATYPE(JudgeStar)
Q_DECLARE_METATYPE(DamageStar)
Q_DECLARE_METATYPE(PindianStar)
Q_DECLARE_METATYPE(PhaseChangeStruct)

#endif // STRUCTS_H
