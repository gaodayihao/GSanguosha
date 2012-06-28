%module sgs

%{
#include "structs.h"
#include "engine.h"
#include "client.h"

#include <QDir>

%}

%include "naturalvar.i"
%include "native.i"
%include "qvariant.i"
%include "list.i"

// ----------------------------------------

class QObject{
public:
    QString objectName();
    void setObjectName(const char *name);
    bool inherits(const char *class_name);
    bool setProperty ( const char * name, const QVariant & value);
    QVariant property ( const char * name ) const;
    void setParent(QObject *parent);
};

%extend QObject{
    const char *className() const{
        return $self->metaObject()->className();
    }
};

class General : public QObject
{
public:
    explicit General(Package *package, const char *name, const char *kingdom, int max_hp = 4, bool male = true, bool hidden = false, bool never_shown = false);

    // property getters/setters
    int getMaxHp() const;
    QString getKingdom() const;
    bool isMale() const;
    bool isFemale() const;
    bool isNeuter() const;
    bool isLord() const;
    bool isHidden() const;
    bool isTotallyHidden() const;

    enum Gender {Male, Female, Neuter};
    Gender getGender() const;
    void setGender(Gender gender);

    void addSkill(Skill* skill);
    void addSkill(const char *skill_name);
    bool hasSkill(const char *skill_name) const;
    QList<const Skill *> getVisibleSkillList() const;
    QSet<const Skill *> getVisibleSkills() const;
    QSet<const TriggerSkill *> getTriggerSkills() const;

    QString getPackage() const;
    QString getSkillDescription() const;
    
    void lastWord() const;
};

class Player: public QObject
{
public:
    enum Phase {RoundStart, Start, Judge, Draw, Play, Discard, Finish, NotActive};
    enum Place {PlaceHand, PlaceEquip, PlaceDelayedTrick, PlaceSpecial,
                DiscardPile, DrawPile, PlaceTable, PlaceUnknown};
    enum Role {Lord, Loyalist, Rebel, Renegade};

    explicit Player(QObject *parent);

    void setScreenName(const char *screen_name);
    QString screenName() const;
    General::Gender getGender() const;

    // property setters/getters
    int getHp() const;
    void setHp(int hp);    
    int getMaxHp() const;
    void setMaxHp(int max_hp);
    int getLostHp() const;
    bool isWounded() const;

    int getMaxCards() const;

    QString getKingdom() const;
    void setKingdom(const char *kingdom);

    void setRole(const char *role);
    QString getRole() const;    
    Role getRoleEnum() const;

    void setGeneral(const General *general);
    void setGeneralName(const char *general_name);
    QString getGeneralName() const;    

    void setGeneral2Name(const char *general_name);
    QString getGeneral2Name() const;
    const General *getGeneral2() const;

    void setState(const char *state);
    QString getState() const;

    int getSeat() const;
    void setSeat(int seat);  
    QString getPhaseString() const;
    void setPhaseString(const char *phase_str);
    Phase getPhase() const;
    void setPhase(Phase phase);

    int getAttackRange() const;
    bool inMyAttackRange(const Player *other) const;

    bool isAlive() const;
    bool isDead() const;
    void setAlive(bool alive);

    QString getFlags() const;
    void setFlags(const char *flag);
    bool hasFlag(const char *flag) const;
    void clearFlags();


    bool faceUp() const;
    void setFaceUp(bool face_up);

    virtual int aliveCount() const = 0;
    int distanceTo(const Player *other, int distanse_fix = 0) const;
    void setFixedDistance(const Player *player, int distance);
    const General *getAvatarGeneral() const;
    const General *getGeneral() const;

    bool isLord() const;
    void acquireSkill(const char *skill_name);
    void loseSkill(const char *skill_name);
    void loseAllSkills();
    bool hasSkill(const char *skill_name) const;
    bool hasLordSkill(const char *skill_name) const;
    bool hasInnateSkill(const char *skill_name) const;

    void setEquip(const EquipCard *card);
    void removeEquip(const EquipCard *equip);
    bool hasEquip(const Card *card) const;
    bool hasEquip() const;

    QList<const Card *> getJudgingArea() const;
    void addDelayedTrick(const Card *trick);
    void removeDelayedTrick(const Card *trick);
    QList<const DelayedTrick *> delayedTricks() const;
    bool containsTrick(const char *trick_name) const;
    const DelayedTrick *topDelayedTrick() const;

    virtual int getHandcardNum() const = 0;
    virtual void removeCard(const Card *card, Place place) = 0;
    virtual void addCard(const Card *card, Place place) = 0;

    const Weapon *getWeapon() const;
    const Armor *getArmor() const;
    const Horse *getDefensiveHorse() const;
    const Horse *getOffensiveHorse() const;
    QList<const Card *> getEquips() const;
    const EquipCard *getEquip(int index) const;

    bool hasWeapon(const char *weapon_name) const;
    bool hasArmorEffect(const char *armor_name) const;

    bool isKongcheng() const;
    bool isNude() const;
    bool isAllNude() const;

    void addMark(const char *mark);
    void removeMark(const char *mark);
    virtual void setMark(const char *mark, int value);
    int getMark(const char *mark) const;

    void setChained(bool chained);
    bool isChained() const;

    bool canSlash(const Player *other, bool distance_limit = true, int rangefix = 0) const;
    int getCardCount(bool include_equip) const;

    QList<int> getPile(const char *pile_name);
    QString getPileName(int card_id) const;

    void addHistory(const char *name, int times = 1);
    void clearHistory();
    bool hasUsed(const char *card_class) const;
    int usedTimes(const char *card_class) const;
    int getSlashCount() const;

    QSet<const TriggerSkill *> getTriggerSkills() const;
    QSet<const Skill *> getVisibleSkills() const;
    QList<const Skill *> getVisibleSkillList() const;
    QSet<QString> getAcquiredSkills() const;

    virtual bool isProhibited(const Player *to, const Card *card) const;
    bool canSlashWithoutCrossbow() const;
    virtual bool isLastHandCard(const Card *card) const = 0;

    void jilei(const char *type);
    bool isJilei(const Card *card) const;

    void setCardLocked(const char *name);
    bool isLocked(const Card *card) const;
    bool hasCardLock(const char *card_str) const;

    bool isCaoCao() const;
    void copyFrom(Player* p);

    QList<const Player *> getSiblings() const;
};

%extend Player{
    void setTag(const char *key, QVariant &value){
        $self->tag[key] = value;
    }

    QVariant getTag(const char *key){
        return $self->tag[key];
    }

    void removeTag(const char *tag_name){
        $self->tag.remove(tag_name);
    }
};

class ServerPlayer : public Player
{
public:
    ServerPlayer(Room *room);

    void setSocket(ClientSocket *socket);
    void invoke(const char *method, const char *arg = ".");
    QString reportHeader() const;
    void unicast(const char *message) const;
    void drawCard(const Card *card);
    Room *getRoom() const;
    void broadcastSkillInvoke(const Card *card) const;
    void broadcastSkillInvoke(const char *card_name) const;
    int getRandomHandCardId() const;
    const Card *getRandomHandCard() const;
    void obtainCard(const Card *card, bool unhide = true);
    void throwAllEquips();
    void throwAllHandCards();
    void throwAllCards();
    void bury();
    void throwAllMarks();
    void clearPrivatePiles();
    void drawCards(int n, bool set_emotion = true, const char *reason = NULL);
    bool askForSkillInvoke(const char *skill_name, const QVariant &data = QVariant());
    QList<int> forceToDiscard(int discard_num, bool include_equip);
    QList<int> handCards() const;
    QList<const Card *> getHandcards() const;
    QList<const Card *> getCards(const char *flags) const;
    DummyCard *wholeHandCards() const;
    bool hasNullification() const;
    void kick();
    bool pindian(ServerPlayer *target, const char *reason, const Card *card1 = NULL);
    void turnOver();
    void play(QList<Player::Phase> set_phases = QList<Player::Phase>());

    QList<Player::Phase> &getPhases();
    void skip(Player::Phase phase);

    void gainMark(const char *mark, int n = 1);
    void loseMark(const char *mark, int n = 1);
    void loseAllMarks(const char *mark_name);

    void setAI(AI *ai);
    AI *getAI() const;
    AI *getSmartAI() const;

    virtual int aliveCount() const;
    virtual int getHandcardNum() const;
    virtual void removeCard(const Card *card, Place place);
    virtual void addCard(const Card *card, Place place);
    virtual bool isLastHandCard(const Card *card) const;

    void addVictim(ServerPlayer *victim);
    QList<ServerPlayer *> getVictims() const;

    void startRecord();
    void saveRecord(const char *filename);

    void setNext(ServerPlayer *next);
    ServerPlayer *getNext() const;
    ServerPlayer *getNextAlive() const;

    // 3v3 methods
    void addToSelected(const char *general);
    QStringList getSelected() const;
    QString findReasonable(const QStringList &generals, bool no_unreasonable = false);
    void clearSelected();

    int getGeneralMaxHp() const;
    virtual QString getGameMode() const;

    QString getIp() const;
    void introduceTo(ServerPlayer *player);
    void marshal(ServerPlayer *player) const;

    void addToPile(const char *pile_name, const Card *card, bool open = true);
    void addToPile(const char *pile_name, int card_id, bool open = true);
    void gainAnExtraTurn(ServerPlayer *clearflag = NULL);
};

%extend ServerPlayer{
    void speak(const char *msg){
        QString str = QByteArray(msg).toBase64();
        $self->getRoom()->speakCommand($self, str);
    }

    bool isSkipped(Player::Phase phase){
        return ! $self->getPhases().contains(phase);
    }
};

class ClientPlayer : public Player
{
public:
    explicit ClientPlayer(Client *client);
    virtual int aliveCount() const;
    virtual int getHandcardNum() const;	
    virtual void removeCard(const Card *card, Place place);
    virtual void addCard(const Card *card, Place place);
    virtual void addKnownHandCard(const Card *card);
    virtual bool isLastHandCard(const Card *card) const; 
};

extern ClientPlayer *Self;

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
	bool transfer;
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
    void parse(const char *str, Room *room);

    const Card *card;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
};

struct CardMoveStruct{
    CardMoveStruct();
    
    int card_id;
    Player::Place from_place, to_place;
    ServerPlayer *from, *to;
};

struct DyingStruct{
    DyingStruct();

    ServerPlayer *who; // who is ask for help
    DamageStruct *damage; // if it is NULL that means the dying is caused by losing hp
};

struct RecoverStruct{
    RecoverStruct();

    int recover;
    ServerPlayer *who;
    const Card *card;
};

struct JudgeStruct{
    JudgeStruct();
    bool isGood(const Card *card = NULL) const;
    bool isBad() const;

    ServerPlayer *who;
    const Card *card;
    QRegExp pattern;
    bool good;
    QString reason;
    bool time_consuming;
};

typedef JudgeStruct *JudgeStar;

struct PindianStruct{
    PindianStruct();

    ServerPlayer *from;
    ServerPlayer *to;
    const Card *from_card;
    const Card *to_card;
    QString reason;
};

struct PhaseChangeStruct{
    PhaseChangeStruct();
    Player::Phase from;
    Player::Phase to;
};

typedef PindianStruct *PindianStar;

enum TriggerEvent{
    NonTrigger,

    GameStart,
    TurnStart,
    PhaseChange,
    DrawNCards,
    HpRecover,
    HpLost,
    HpChanged,
	MaxHpLost,

    StartJudge,
    AskForRetrial,
    FinishJudge,

    Pindian,
    TurnedOver,

    Predamage,
    DamageForseen,
    DamageCaused,
    DamageInflicted,
    DamageDone,
    Damage,
    Damaged,
    DamageComplete,

    Dying,
    AskForPeaches,
    AskForPeachesDone,
    Death,
    GameOverJudge,

    SlashEffect,
    SlashEffected,
    SlashProceed,
    SlashHit,
    SlashMissed,

    CardAsked,
    CardResponsed,
    CardDiscarded,
    CardLostOnePiece,
    CardLostOneTime,
    CardGotOnePiece,
    CardGotOneTime,
    CardDrawing,
    CardDrawnDone,

    CardUsed,
    TargetConfirming,
    TargetConfirmed,
    CardEffect,
    CardEffected,
    CardFinished,

    ChoiceMade,

    NumOfEvents,
};

class CardMoveReason
{
public:
    int m_reason;
    QString m_playerId; // the cause (not the source) of the movement, such as "lusu" when "dimeng", or "zhanghe" when "qiaobian"
    QString m_targetId;
    QString m_skillName; // skill that triggers movement of the cards, such as "longdang", "dimeng"
    QString m_eventName; // additional arg such as "lebusishu" on top of "S_REASON_JUDGE"
};

class Card: public QObject
{

public:
    // enumeration type
    enum Suit {Spade, Club, Heart, Diamond, NoSuit};
    static const Suit AllSuits[4];
    
    // card types
    enum CardType{
        Skill,
        Basic,
        Trick,
        Equip,
    };

    // constructor
    Card(Suit suit, int number, bool target_fixed = false);

    // property getters/setters
    QString getSuitString() const;
    bool isRed() const;
    bool isBlack() const;
    int getId() const;
    void setId(int id);
    int getEffectiveId() const;
    QString getEffectIdString() const;

    int getNumber() const;
    void setNumber(int number);
    QString getNumberString() const;

    Suit getSuit() const;
    void setSuit(Suit suit);

    bool sameColorWith(const Card *other) const;
    bool isEquipped() const;

    QString getPackage() const;    
    QString getFullName(bool include_suit = false) const;
    QString getLogName() const;
    QString getName() const;
    QString getSkillName() const;   
    void setSkillName(const char *skill_name);
    QString getDescription() const;

    bool isVirtualCard() const;
    virtual bool match(const char *pattern) const;

    void addSubcard(int card_id);
    void addSubcard(const Card *card);
    QList<int> getSubcards() const;
    void clearSubcards();
    QString subcardString() const;
    void addSubcards(const QList<CardItem *> &card_items);
    int subcardsLength() const;

    virtual QString getType() const = 0;
    virtual QString getSubtype() const = 0;
    virtual CardType getTypeId() const = 0;
    virtual QString toString() const;
    bool isNDTrick() const;

    // card target selection
    bool targetFixed() const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *self) const;
    virtual bool isAvailable(const Player *player) const;
    virtual const Card *validate(const CardUseStruct *card_use) const;
    virtual const Card *validateInResposing(ServerPlayer *user, bool *continuable) const;

    // it can be used only once a turn or not
    bool isOnce() const;
    bool isMute() const;
    bool willThrow() const;
    bool canJilei() const;
    bool hasPreAction() const;
        
    void setFlags(const char *flag) const;
    bool hasFlag(const char *flag) const;
    void clearFlags() const;

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source,  const QList<ServerPlayer *> &targets) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isCancelable(const CardEffectStruct &effect) const;

    virtual void onMove(const CardMoveStruct &move) const;

    // static functions
    static bool CompareByColor(const Card *a, const Card *b);
    static bool CompareBySuitNumber(const Card *a, const Card *b);
    static bool CompareByType(const Card *a, const Card *b);
    static const Card *Parse(const char *str);
    static Card * Clone(const Card *card);
    static QString Suit2String(Suit suit);
    static QString Number2String(int number);
    static QStringList IdsToStrings(const QList<int> &ids);
    static QList<int> StringsToIds(const QStringList &strings);
};

%extend Card{
    Weapon* toWeapon(){
        return qobject_cast<Weapon*>($self);
    }	
};

class SkillCard: public Card{
public:
    SkillCard();
    void setUserString(const char *user_string);

    virtual QString getSubtype() const;    
    virtual QString getType() const;
    virtual CardType getTypeId() const;
    virtual QString toString() const;    

protected:
    QString user_string;
};

class DummyCard: public Card{

};

class Package: public QObject{
public:
    Package(const char *name);
};

class Engine: public QObject
{
public:
    void addTranslationEntry(const char *key, const char *value);
    QString translate(const char *to_translate) const;    

    void addPackage(Package *package);
    void addBanPackage(const char *package_name);
    QStringList getBanPackages() const;
    Card *cloneCard(const char *name, Card::Suit suit, int number) const;
    SkillCard *cloneSkillCard(const char *name) const;
    QString getVersion() const;
    QString getVersionName() const;
    QStringList getExtensions() const;
    QStringList getKingdoms() const;
    QColor getKingdomColor(const char *kingdom) const;
    QString getSetupString() const;

    QMap<QString, QString> getAvailableModes() const;
    QString getModeName(const char *mode) const;
    int getPlayerCount(const char *mode) const;
    QString getRoles(const char *mode) const;
    QStringList getRoleList(const char *mode) const;
    int getRoleIndex() const;

    const CardPattern *getPattern(const char *name) const;
    QList<const Skill *> getRelatedSkills(const char *skill_name) const;

    QStringList getScenarioNames() const;
    void addScenario(Scenario *scenario);
    const Scenario *getScenario(const char *name) const;

    const General *getGeneral(const char *name) const;
    int getGeneralCount(bool include_banned = false) const;
    const Skill *getSkill(const char *skill_name) const;
    const TriggerSkill *getTriggerSkill(const char *skill_name) const;
    const ViewAsSkill *getViewAsSkill(const char *skill_name) const;
    QList<const DistanceSkill *> getDistanceSkills() const;
    void addSkills(const QList<const Skill *> &skills);

    int getCardCount() const;
    const Card *getCard(int index) const;

    QStringList getLords() const;
    QStringList getRandomLords() const;
    QStringList getRandomGenerals(int count, const QSet<QString> &ban_set = QSet<QString>()) const;
    QList<int> getRandomCards() const;
    QString getRandomGeneralName() const;
    QStringList getLimitedGeneralNames() const;
    void playAudioEffect(const char *filename) const;

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card) const;
    int correctDistance(const Player *from, const Player *to) const;
};

extern Engine *Sanguosha;

class Skill : public QObject
{
public:
    enum Frequency{
        Frequent,
        NotFrequent,
        Compulsory,
        Limited,
        Wake
    };

    explicit Skill(const char *name, Frequency frequent = NotFrequent);
    bool isLordSkill() const;
    QString getDescription() const;
    QString getText() const;
    bool isVisible() const;

    virtual QString getDefaultChoice(ServerPlayer *player) const;
    virtual int getEffectIndex(const ServerPlayer *player, const Card *card) const;
    virtual QDialog *getDialog() const;

    void initMediaSource();
    void playAudioEffect(int index = -1) const;
    void setFlag(ServerPlayer *player) const;
    void unsetFlag(ServerPlayer *player) const;
    Frequency getFrequency() const;
    QStringList getSources() const;
};

class TriggerSkill:public Skill{
public:
    TriggerSkill(const char *name);
    const ViewAsSkill *getViewAsSkill() const;
    QList<TriggerEvent> getTriggerEvents() const;

    virtual int getPriority() const;
    virtual bool triggerable(const ServerPlayer *target) const;    
    virtual bool trigger(TriggerEvent event, Room* room, ServerPlayer *player, QVariant &data) const = 0;
};

class QThread: public QObject{
};

struct LogMessage{
    LogMessage();
    QString toString() const;

    QString type;
    ServerPlayer *from;
    QList<ServerPlayer *> to;
    QString card_str;
    QString arg;
    QString arg2;
};

class RoomThread : public QThread{
public:
    explicit RoomThread(Room *room);
    void constructTriggerTable();
    bool trigger(TriggerEvent event, Room* room, ServerPlayer *target, QVariant &data);
    bool trigger(TriggerEvent event, Room* room, ServerPlayer *target);

    void addPlayerSkills(ServerPlayer *player, bool invoke_game_start = false);

    void addTriggerSkill(const TriggerSkill *skill);
    void delay(unsigned long msecs = 1000);
    void run3v3();
    void action3v3(ServerPlayer *player);
};

class Room : public QThread{
public:
    explicit Room(QObject *parent, const char *mode);
    ServerPlayer *addSocket(ClientSocket *socket);
    bool isFull() const;
    bool isFinished() const;
    int getLack() const;
    QString getMode() const;
    const Scenario *getScenario() const;
    RoomThread *getThread() const;
    ServerPlayer *getCurrent() const;
    void setCurrent(ServerPlayer *current);
    int alivePlayerCount() const;
    QList<ServerPlayer *> getOtherPlayers(ServerPlayer *except) const;
    QList<ServerPlayer *> getPlayers() const;
    QList<ServerPlayer *> getAllPlayers() const;
    QList<ServerPlayer *> getAlivePlayers() const;
    void enterDying(ServerPlayer *player, DamageStruct *reason);
    void killPlayer(ServerPlayer *victim, DamageStruct *reason = NULL);
    void revivePlayer(ServerPlayer *player);
    QStringList aliveRoles(ServerPlayer *except = NULL) const;
    void gameOver(const char *winner);
    void slashEffect(const SlashEffectStruct &effect);
    void slashResult(const SlashEffectStruct &effect, const Card *jink);
    void attachSkillToPlayer(ServerPlayer *player, const char *skill_name);
    void detachSkillFromPlayer(ServerPlayer *player, const char *skill_name);
    void setPlayerFlag(ServerPlayer *player, const char *flag);
    void setPlayerProperty(ServerPlayer *player, const char *property_name, const QVariant &value);
    void setPlayerMark(ServerPlayer *player, const char *mark, int value);
    void setPlayerCardLock(ServerPlayer *player, const char *name);
    void clearPlayerCardLock(ServerPlayer *player);
    void setPlayerStatistics(ServerPlayer *player, const char *property_name, const QVariant &value);
    void setCardFlag(const Card *card, const char *flag, ServerPlayer *who = NULL);
    void setCardFlag(int card_id, const char *flag, ServerPlayer *who = NULL);
    void clearCardFlag(const Card *card, ServerPlayer *who = NULL);
    void clearCardFlag(int card_id, ServerPlayer *who = NULL);
    void useCard(const CardUseStruct &card_use, bool add_history = true);
    void damage(const DamageStruct &data);
    void sendDamageLog(const DamageStruct &data);
    void loseHp(ServerPlayer *victim, int lose = 1);
    void loseMaxHp(ServerPlayer *victim, int lose = 1);
    void applyDamage(ServerPlayer *victim, const DamageStruct &damage);
    void recover(ServerPlayer *player, const RecoverStruct &recover, bool set_emotion = false);
    bool cardEffect(const Card *card, ServerPlayer *from, ServerPlayer *to);
    bool cardEffect(const CardEffectStruct &effect);
    void judge(JudgeStruct &judge_struct);
    void sendJudgeResult(const JudgeStar judge);
    QList<int> getNCards(int n, bool update_pile_number = true);
    ServerPlayer *getLord() const;
    void askForGuanxing(ServerPlayer *zhuge, const QList<int> &cards, bool up_only);
    void doGongxin(ServerPlayer *shenlumeng, ServerPlayer *target);
    int drawCard();
    const Card *peek();
    void fillAG(const QList<int> &card_ids, ServerPlayer *who = NULL);
    void takeAG(ServerPlayer *player, int card_id);
    void provide(const Card *card);
    QList<ServerPlayer *> getLieges(const char *kingdom, ServerPlayer *lord) const;
    void sendLog(const LogMessage &log);
    void showCard(ServerPlayer *player, int card_id, ServerPlayer *only_viewer = NULL);
    void showAllCards(ServerPlayer *player, ServerPlayer *to = NULL);
    void acquireSkill(ServerPlayer *player, const Skill *skill, bool open = true);
    void acquireSkill(ServerPlayer *player, const char *skill_name, bool open = true);
    void adjustSeats();
    void swapPile();
    QList<int> getDiscardPile();
    QList<int> getDrawPile();
    int getCardFromPile(const char *card_name);
    ServerPlayer *findPlayer(const char *general_name, bool include_dead = false) const;
    ServerPlayer *findPlayerBySkillName(const char *skill_name, bool include_dead = false) const;
    QList<ServerPlayer *> findPlayersBySkillName(const QString &skill_name, bool include_dead = false) const;
    void installEquip(ServerPlayer *player, const char *equip_name);
    void resetAI(ServerPlayer *player);
    void transfigure(ServerPlayer *player, const char *new_general, bool full_state, bool invoke_start = true);
    void swapSeat(ServerPlayer *a, ServerPlayer *b);
    lua_State *getLuaState() const;
    void setFixedDistance(Player *from, const Player *to, int distance);
    void reverseFor3v3(const Card *card, ServerPlayer *player, QList<ServerPlayer *> &list);
    bool hasWelfare(const ServerPlayer *player) const;
    ServerPlayer *getFront(ServerPlayer *a, ServerPlayer *b) const;
    void signup(ServerPlayer *player, const char *screen_name, const char *avatar, bool is_robot);
    ServerPlayer *getOwner() const;

    void reconnect(ServerPlayer *player, ClientSocket *socket);
    void marshal(ServerPlayer *player);

    bool isVirtual();
    void setVirtual();
    void copyFrom(Room* rRoom);
    Room* duplicate();

    const ProhibitSkill *isProhibited(const Player *from, const Player *to, const Card *card) const;

    void setTag(const char *key, const QVariant &value);
    QVariant getTag(const char *key) const;
    void removeTag(const char *key);

    void setEmotion(ServerPlayer *target, const char *emotion);

    Player::Place getCardPlace(int card_id) const;
    ServerPlayer *getCardOwner(int card_id) const;
    void setCardMapping(int card_id, ServerPlayer *owner, Player::Place place);

    void drawCards(ServerPlayer *player, int n, const char *reason = NULL);
    void obtainCard(ServerPlayer *target, const Card *card, bool unhide = true);
    void obtainCard(ServerPlayer *target, int card_id, bool unhide = true);

    void throwCard(const Card *card, ServerPlayer *who);
    void throwCard(int card_id, ServerPlayer *who);
    void moveCardTo(const Card *card, ServerPlayer *from, ServerPlayer *to, Player::Place place, const CardMoveReason &reason, bool open = true);

    // interactive methods
    void activate(ServerPlayer *player, CardUseStruct &card_use);
    Card::Suit askForSuit(ServerPlayer *player, const char *reason);
    QString askForKingdom(ServerPlayer *player);
    bool askForSkillInvoke(ServerPlayer *player, const char *skill_name, const QVariant &data = QVariant());
    QString askForChoice(ServerPlayer *player, const char *skill_name, const char *choices, const QVariant &data = QVariant());
    bool askForDiscard(ServerPlayer *target, const char *reason, int discard_num, int min_num, bool optional = false, bool include_equip = false);
    const Card *askForExchange(ServerPlayer *player, const char *reason, int discard_num);
    bool askForNullification(const TrickCard *trick, ServerPlayer *from, ServerPlayer *to, bool positive);
    bool isCanceled(const CardEffectStruct &effect);
    int askForCardChosen(ServerPlayer *player, ServerPlayer *who, const char *flags, const char *reason);
    const Card *askForCard(ServerPlayer *player, const char *pattern, const char *prompt, const QVariant &data = QVariant());
    bool askForUseCard(ServerPlayer *player, const char *pattern, const char *prompt);
    int askForAG(ServerPlayer *player, const QList<int> &card_ids, bool refusable, const char *reason);
    const Card *askForCardShow(ServerPlayer *player, ServerPlayer *requestor, const char *reason);
    bool askForYiji(ServerPlayer *guojia, QList<int> &cards);
    const Card *askForPindian(ServerPlayer *player, ServerPlayer *from, ServerPlayer *to, const char *reason);
    ServerPlayer *askForPlayerChosen(ServerPlayer *player, const QList<ServerPlayer *> &targets, const char *reason);
    const Card *askForSinglePeach(ServerPlayer *player, ServerPlayer *dying);
};

%extend Room {
    ServerPlayer *nextPlayer() const{
        return $self->getCurrent()->getNextAlive();
    }

    void output(const char *msg){
        if(Config.value("DebugOutput", false).toBool())
            $self->output(msg);
    }

    void outputEventStack(){
        if(Config.value("DebugOutput", false).toBool())
            $self->outputEventStack();
    }

    void writeToConsole(const char *msg){
        $self->output(msg);
        qWarning("%s", msg);
    }
};

%{

void Room::doScript(const QString &script){
    SWIG_NewPointerObj(L, this, SWIGTYPE_p_Room, 0);
    lua_setglobal(L, "R");

    SWIG_NewPointerObj(L, current, SWIGTYPE_p_ServerPlayer, 0);
    lua_setglobal(L, "P");

    luaL_dostring(L, script.toAscii());
}

%}

class QRegExp{
public:
    QRegExp(const char *);
    bool exactMatch(const char *);
};


%include "luaskills.i"
%include "card.i"
%include "ai.i"

typedef DamageStruct *DamageStar;
