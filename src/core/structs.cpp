#include "structs.h"
#include "jsonutils.h"
#include "protocol.h"

using namespace QSanProtocol::Utils;

bool CardMoveStruct::tryParse(const Json::Value &arg)
{
    if (!arg.isArray() || arg.size() != 7) return false;
    if (!arg[0].isInt() || !isIntArray(arg, 1, 2) || !isStringArray(arg, 3, 6)) return false;
    card_id = arg[0].asInt();    
    from_place = (Player::Place)arg[1].asInt();
    to_place = (Player::Place)arg[2].asInt();
    from_player_name = toQString(arg[3]);
    to_player_name = toQString(arg[4]);
    from_pile_name = toQString(arg[5]);
    to_pile_name = toQString(arg[6]);
    return true;
}

Json::Value CardMoveStruct::toJsonValue() const
{
    Json::Value arg(Json::arrayValue);
    if (open) arg[0] = card_id;
    else arg[0] = Card::S_UNKNOWN_CARD_ID;
    arg[1] = (int)from_place;
    arg[2] = (int)to_place;
    arg[3] = toJsonString(from_player_name);
    arg[4] = toJsonString(to_player_name);
    arg[5] = toJsonString(from_pile_name);
    arg[6] = toJsonString(to_pile_name);
    return arg;
}

bool CardsMoveStruct::tryParse(const Json::Value &arg)
{
    if (!arg.isArray() || arg.size() != 7) return false;
    if ((!arg[0].isInt() && !arg[0].isArray()) ||
        !isIntArray(arg, 1, 2) || !isStringArray(arg, 3, 6)) return false;
    if (arg[0].isInt())
    {
        int size = arg[0].asInt();
        for (int i = 0; i < size; i++)        
            card_ids.append(Card::S_UNKNOWN_CARD_ID);        
    }
    else if (!QSanProtocol::Utils::tryParse(arg[0], card_ids))
        return false;
    from_place = (Player::Place)arg[1].asInt();
    to_place = (Player::Place)arg[2].asInt();
    from_player_name = toQString(arg[3]);
    to_player_name = toQString(arg[4]);
    from_pile_name = toQString(arg[5]);
    to_pile_name = toQString(arg[6]);
    return true;
}

Json::Value CardsMoveStruct::toJsonValue() const
{
    Json::Value arg(Json::arrayValue);
    if (open) arg[0] = toJsonArray(card_ids);
    else arg[0] = card_ids.size();
    arg[1] = (int)from_place;
    arg[2] = (int)to_place;
    arg[3] = toJsonString(from_player_name);
    arg[4] = toJsonString(to_player_name);
    arg[5] = toJsonString(from_pile_name);
    arg[6] = toJsonString(to_pile_name);
    return arg;
}

QList<CardMoveStruct> CardsMoveStruct::flatten()
{
    QList<CardMoveStruct> result;
    foreach (int card_id, card_ids)
    {
        CardMoveStruct move;
        move.from = from;
        move.to = to;
        move.from_pile_name = from_pile_name;
        move.to_pile_name = to_pile_name;
        move.from_place = from_place;
        move.to_place = to_place;
        move.card_id = card_id;
        move.from_player_name = from_player_name;
        move.to_player_name = to_player_name;
        move.open = open;
        result.append(move);
    }
    return result;
}