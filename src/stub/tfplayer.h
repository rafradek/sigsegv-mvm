#ifndef _INCLUDE_SIGSEGV_STUB_TFPLAYER_H_
#define _INCLUDE_SIGSEGV_STUB_TFPLAYER_H_


#include "stub/baseplayer.h"
#include "stub/econ.h"


class CBaseObject;
class CTFWeaponBase;
class CTFPlayer;
class CTFItem;
class CTFWearable;
enum ETFFlagType : int32_t;

enum PlayerAnimEvent_t : int32_t
{
	PLAYERANIMEVENT_ATTACK_PRIMARY          =  0,
	PLAYERANIMEVENT_ATTACK_SECONDARY        =  1,
	PLAYERANIMEVENT_ATTACK_GRENADE          =  2,
	PLAYERANIMEVENT_RELOAD                  =  3,
	PLAYERANIMEVENT_RELOAD_LOOP             =  4,
	PLAYERANIMEVENT_RELOAD_END              =  5,
	PLAYERANIMEVENT_JUMP                    =  6,
	PLAYERANIMEVENT_SWIM                    =  7,
	PLAYERANIMEVENT_DIE                     =  8,
	PLAYERANIMEVENT_FLINCH_CHEST            =  9,
	PLAYERANIMEVENT_FLINCH_HEAD             = 10,
	PLAYERANIMEVENT_FLINCH_LEFTARM          = 11,
	PLAYERANIMEVENT_FLINCH_RIGHTARM         = 12,
	PLAYERANIMEVENT_FLINCH_LEFTLEG          = 13,
	PLAYERANIMEVENT_FLINCH_RIGHTLEG         = 14,
	PLAYERANIMEVENT_DOUBLEJUMP              = 15,
	PLAYERANIMEVENT_CANCEL                  = 16,
	PLAYERANIMEVENT_SPAWN                   = 17,
	PLAYERANIMEVENT_SNAP_YAW                = 18,
	PLAYERANIMEVENT_CUSTOM                  = 19,
	PLAYERANIMEVENT_CUSTOM_GESTURE          = 20,
	PLAYERANIMEVENT_CUSTOM_SEQUENCE         = 21,
	PLAYERANIMEVENT_CUSTOM_GESTURE_SEQUENCE = 22,
	PLAYERANIMEVENT_ATTACK_PRE              = 23,
	PLAYERANIMEVENT_ATTACK_POST             = 24,
	PLAYERANIMEVENT_GRENADE1_DRAW           = 25,
	PLAYERANIMEVENT_GRENADE2_DRAW           = 26,
	PLAYERANIMEVENT_GRENADE1_THROW          = 27,
	PLAYERANIMEVENT_GRENADE2_THROW          = 28,
	PLAYERANIMEVENT_VOICE_COMMAND_GESTURE   = 29,
	PLAYERANIMEVENT_DOUBLEJUMP_CROUCH       = 30,
	PLAYERANIMEVENT_STUN_BEGIN              = 31,
	PLAYERANIMEVENT_STUN_MIDDLE             = 32,
	PLAYERANIMEVENT_STUN_END                = 33,
	PLAYERANIMEVENT_PASSTIME_THROW_BEGIN    = 34,
	PLAYERANIMEVENT_PASSTIME_THROW_MIDDLE   = 35,
	PLAYERANIMEVENT_PASSTIME_THROW_END      = 36,
	
	/* this seems to have been removed */
//	PLAYERANIMEVENT_PASSTIME_THROW_CANCEL,
	
	/* these seem to have been added (note: unofficial names) */
	PLAYERANIMEVENT_CYOA_PDA_INTRO          = 37,
	PLAYERANIMEVENT_CYOA_PDA_IDLE           = 38,
	PLAYERANIMEVENT_CYOA_PDA_OUTRO          = 39,
	
	/* this had its number bumped, since it remained at the end of the list */
	PLAYERANIMEVENT_ATTACK_PRIMARY_SUPER    = 40,
	
	PLAYERANIMEVENT_COUNT,
};

enum : int32_t
{
	TF_CLASS_UNDEFINED = 0,
	
	TF_CLASS_SCOUT,
	TF_CLASS_SNIPER,
	TF_CLASS_SOLDIER,
	TF_CLASS_DEMOMAN,
	TF_CLASS_MEDIC,
	TF_CLASS_HEAVYWEAPONS,
	TF_CLASS_PYRO,
	TF_CLASS_SPY,
	TF_CLASS_ENGINEER,
	
	TF_CLASS_COUNT,
};


enum ETFCond : int32_t
{
	TF_COND_INVALID                          =  -1,
	
	TF_COND_AIMING                           =   0,
	TF_COND_ZOOMED                           =   1,
	TF_COND_DISGUISING                       =   2,
	TF_COND_DISGUISED                        =   3,
	TF_COND_STEALTHED                        =   4,
	TF_COND_INVULNERABLE                     =   5,
	TF_COND_TELEPORTED                       =   6,
	TF_COND_TAUNTING                         =   7,
	TF_COND_INVULNERABLE_WEARINGOFF          =   8,
	TF_COND_STEALTHED_BLINK                  =   9,
	TF_COND_SELECTED_TO_TELEPORT             =  10,
	TF_COND_CRITBOOSTED                      =  11,
	TF_COND_TMPDAMAGEBONUS                   =  12,
	TF_COND_FEIGN_DEATH                      =  13,
	TF_COND_PHASE                            =  14,
	TF_COND_STUNNED                          =  15,
	TF_COND_OFFENSEBUFF                      =  16,
	TF_COND_SHIELD_CHARGE                    =  17,
	TF_COND_DEMO_BUFF                        =  18,
	TF_COND_ENERGY_BUFF                      =  19,
	TF_COND_RADIUSHEAL                       =  20,
	TF_COND_HEALTH_BUFF                      =  21,
	TF_COND_BURNING                          =  22,
	TF_COND_HEALTH_OVERHEALED                =  23,
	TF_COND_URINE                            =  24,
	TF_COND_BLEEDING                         =  25,
	TF_COND_DEFENSEBUFF                      =  26,
	TF_COND_MAD_MILK                         =  27,
	TF_COND_MEGAHEAL                         =  28,
	TF_COND_REGENONDAMAGEBUFF                =  29,
	TF_COND_MARKEDFORDEATH                   =  30,
	TF_COND_NOHEALINGDAMAGEBUFF              =  31,
	TF_COND_SPEED_BOOST                      =  32,
	TF_COND_CRITBOOSTED_PUMPKIN              =  33,
	TF_COND_CRITBOOSTED_USER_BUFF            =  34,
	TF_COND_CRITBOOSTED_DEMO_CHARGE          =  35,
	TF_COND_SODAPOPPER_HYPE                  =  36,
	TF_COND_CRITBOOSTED_FIRST_BLOOD          =  37,
	TF_COND_CRITBOOSTED_BONUS_TIME           =  38,
	TF_COND_CRITBOOSTED_CTF_CAPTURE          =  39,
	TF_COND_CRITBOOSTED_ON_KILL              =  40,
	TF_COND_CANNOT_SWITCH_FROM_MELEE         =  41,
	TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK        =  42,
	TF_COND_REPROGRAMMED                     =  43,
	TF_COND_CRITBOOSTED_RAGE_BUFF            =  44,
	TF_COND_DEFENSEBUFF_HIGH                 =  45,
	TF_COND_SNIPERCHARGE_RAGE_BUFF           =  46,
	TF_COND_DISGUISE_WEARINGOFF              =  47,
	TF_COND_MARKEDFORDEATH_SILENT            =  48,
	TF_COND_DISGUISED_AS_DISPENSER           =  49,
	TF_COND_SAPPED                           =  50,
	TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED =  51,
	TF_COND_INVULNERABLE_USER_BUFF           =  52,
	TF_COND_HALLOWEEN_BOMB_HEAD              =  53,
	TF_COND_HALLOWEEN_THRILLER               =  54,
	TF_COND_RADIUSHEAL_ON_DAMAGE             =  55,
	TF_COND_CRITBOOSTED_CARD_EFFECT          =  56,
	TF_COND_INVULNERABLE_CARD_EFFECT         =  57,
	TF_COND_MEDIGUN_UBER_BULLET_RESIST       =  58,
	TF_COND_MEDIGUN_UBER_BLAST_RESIST        =  59,
	TF_COND_MEDIGUN_UBER_FIRE_RESIST         =  60,
	TF_COND_MEDIGUN_SMALL_BULLET_RESIST      =  61,
	TF_COND_MEDIGUN_SMALL_BLAST_RESIST       =  62,
	TF_COND_MEDIGUN_SMALL_FIRE_RESIST        =  63,
	TF_COND_STEALTHED_USER_BUFF              =  64,
	TF_COND_MEDIGUN_DEBUFF                   =  65,
	TF_COND_STEALTHED_USER_BUFF_FADING       =  66,
	TF_COND_BULLET_IMMUNE                    =  67,
	TF_COND_BLAST_IMMUNE                     =  68,
	TF_COND_FIRE_IMMUNE                      =  69,
	TF_COND_PREVENT_DEATH                    =  70,
	TF_COND_MVM_BOT_STUN_RADIOWAVE           =  71,
	TF_COND_HALLOWEEN_SPEED_BOOST            =  72,
	TF_COND_HALLOWEEN_QUICK_HEAL             =  73,
	TF_COND_HALLOWEEN_GIANT                  =  74,
	TF_COND_HALLOWEEN_TINY                   =  75,
	TF_COND_HALLOWEEN_IN_HELL                =  76,
	TF_COND_HALLOWEEN_GHOST_MODE             =  77,
	TF_COND_MINICRITBOOSTED_ON_KILL          =  78,
	TF_COND_OBSCURED_SMOKE                   =  79,
	TF_COND_PARACHUTE_ACTIVE                 =  80,
	TF_COND_BLASTJUMPING                     =  81,
	TF_COND_HALLOWEEN_KART                   =  82,
	TF_COND_HALLOWEEN_KART_DASH              =  83,
	TF_COND_BALLOON_HEAD                     =  84,
	TF_COND_MELEE_ONLY                       =  85,
	TF_COND_SWIMMING_CURSE                   =  86,
	TF_COND_FREEZE_INPUT                     =  87,
	TF_COND_HALLOWEEN_KART_CAGE              =  88,
	TF_COND_DONOTUSE_0                       =  89,
	TF_COND_RUNE_STRENGTH                    =  90,
	TF_COND_RUNE_HASTE                       =  91,
	TF_COND_RUNE_REGEN                       =  92,
	TF_COND_RUNE_RESIST                      =  93,
	TF_COND_RUNE_VAMPIRE                     =  94,
	TF_COND_RUNE_REFLECT                     =  95,
	TF_COND_RUNE_PRECISION                   =  96,
	TF_COND_RUNE_AGILITY                     =  97,
	TF_COND_GRAPPLINGHOOK                    =  98,
	TF_COND_GRAPPLINGHOOK_SAFEFALL           =  99,
	TF_COND_GRAPPLINGHOOK_LATCHED            = 100,
	TF_COND_GRAPPLINGHOOK_BLEEDING           = 101,
	TF_COND_AFTERBURN_IMMUNE                 = 102,
	TF_COND_RUNE_KNOCKOUT                    = 103,
	TF_COND_RUNE_IMBALANCE                   = 104,
	TF_COND_CRITBOOSTED_RUNE_TEMP            = 105,
	TF_COND_PASSTIME_INTERCEPTION            = 106,
	TF_COND_SWIMMING_NO_EFFECTS              = 107,
	TF_COND_PURGATORY                        = 108,
	TF_COND_RUNE_KING                        = 109,
	TF_COND_RUNE_PLAGUE                      = 110,
	TF_COND_RUNE_SUPERNOVA                   = 111,
	TF_COND_PLAGUE                           = 112,
	TF_COND_KING_BUFFED                      = 113,
	TF_COND_TEAM_GLOWS                       = 114,
	TF_COND_KNOCKED_INTO_AIR                 = 115,
	TF_COND_COMPETITIVE_WINNER               = 116,
	TF_COND_COMPETITIVE_LOSER                = 117,
	TF_COND_HEALING_DEBUFF                   = 118,
	TF_COND_PASSTIME_PENALTY_DEBUFF          = 119,
	TF_COND_GRAPPLED_TO_PLAYER               = 120,
	TF_COND_GRAPPLED_BY_PLAYER               = 121,
	TF_COND_PARACHUTE_DEPLOYED               = 122,
	TF_COND_GAS                              = 123,
	TF_COND_BURNING_PYRO                     = 124,
	TF_COND_ROCKETPACK                       = 125,
	TF_COND_LOST_FOOTING                     = 126,
	TF_COND_AIR_CURRENT                      = 127,
	TF_COND_HALLOWEEN_HELL_HEAL              = 128,
	TF_COND_POWERUPMODE_DOMINANT             = 129,
	TF_COND_COUNT,
};

struct condition_source_t
{
	int datatable;
	int	m_nPreventedDamageFromCondition;
	float m_flExpireTime;
	float m_pProvider;
	bool m_bPrevActive;
};

class CMultiplayerAnimState
{
public:
	void OnNewModel() { ft_OnNewModel(this); }
	
private:
	static MemberFuncThunk<CMultiplayerAnimState *, void> ft_OnNewModel;
};
class CTFPlayerAnimState : public CMultiplayerAnimState {};


class CTFPlayerClassShared
{
public:
	void NetworkStateChanged();
	void NetworkStateChanged(void *pVar);
	
	CTFPlayer *GetOuter();
	
	bool IsClass(int iClass) const { return (this->m_iClass == iClass); }
	
	int GetClassIndex() const { return this->m_iClass; }
	void SetCustomModel(const char *pszModelPath, bool bUseClassAnimations = true) { ft_SetCustomModel(this, pszModelPath, bUseClassAnimations); }
	// TODO: accessor for m_iszClassIcon
	// TODO: accessor for m_iszCustomModel
	const char *GetCustomModel() { return STRING((string_t)this->m_iszCustomModel);}
	
	DECL_SENDPROP(bool,                 m_bUseClassAnimations);
private:
	DECL_SENDPROP(int,      m_iClass);
	DECL_SENDPROP(string_t, m_iszClassIcon);
	DECL_SENDPROP(string_t, m_iszCustomModel);
	static MemberFuncThunk<CTFPlayerClassShared *, void, const char *, bool> ft_SetCustomModel;
};
class CTFPlayerClass : public CTFPlayerClassShared {};

class CTFPlayerShared
{
public:
	void NetworkStateChanged();
	void NetworkStateChanged(void *pVar);
	
	CTFPlayer *GetOuter()          { return this->m_pOuter; }
	int GetState() const           { return this->m_nPlayerState; }
	void SetState(int nState)      { this->m_nPlayerState = nState; }
	bool InState(int nState) const { return (this->m_nPlayerState == nState); }
	
	void AddCond(ETFCond cond, float duration = -1.0f, CBaseEntity *provider = nullptr) {        ft_AddCond                   (this, cond, duration, provider); }
	void RemoveCond(ETFCond cond, bool b1 = false)                                      {        ft_RemoveCond                (this, cond, b1); }
	bool InCond(ETFCond cond) const                                                     { return ft_InCond                    (this, cond); }
	bool IsInvulnerable() const                                                         { return ft_IsInvulnerable            (this); }
	void StunPlayer(float duration, float amount, int flags, CTFPlayer *stunner)        {        ft_StunPlayer                (this, duration, amount, flags, stunner); }
	void GetConditionsBits(CBitVec<192>& bitvec)                                        {        ft_GetConditionsBits         (this, bitvec); }
	float GetConditionDuration(ETFCond cond)                                            { return ft_GetConditionDuration      (this, cond); }
	CBaseEntity *GetConditionProvider(ETFCond cond)                                     { return ft_GetConditionProvider      (this, cond); }
	int GetDisguiseTeam() const                                                         { return ft_GetDisguiseTeam           (this); }
	bool IsStealthed() const                                                            { return ft_IsStealthed               (this); }
	float GetPercentInvisible() const                                                   { return ft_GetPercentInvisible       (this); }
	bool IsControlStunned()                                                             { return ft_IsControlStunned          (this); }
	bool IsLoserStateStunned() const                                                    { return ft_IsLoserStateStunned       (this); }
	void SetDefaultItemChargeMeters()                                                   {        ft_SetDefaultItemChargeMeters(this); }
	void SetItemChargeMeter(loadout_positions_t slot, float value)                      {        ft_SetItemChargeMeter        (this, slot, value); }
	void Burn(CTFPlayer *igniter, CTFWeaponBase *weapon, float duration = 10.0f)                      {        ft_Burn        (this, igniter, weapon, duration); }
	DECL_SENDPROP(float,       m_flCloakMeter);
	DECL_SENDPROP(float,       m_flEnergyDrinkMeter);
	DECL_SENDPROP(float,       m_flHypeMeter);
	DECL_SENDPROP(float,       m_flChargeMeter);
	DECL_SENDPROP(float,       m_flRageMeter);
	DECL_SENDPROP(bool,        m_bRageDraining);
	DECL_SENDPROP(int,         m_iCritMult);
	DECL_SENDPROP(bool,        m_bInUpgradeZone);
	DECL_SENDPROP(float,       m_flStealthNoAttackExpire);
	DECL_SENDPROP(int,         m_iAirDash);
	DECL_SENDPROP(int,         m_iDesiredPlayerClass);
	DECL_EXTRACT (CUtlVector<condition_source_t>, m_ConditionData);
	
private:
	DECL_SENDPROP(int,         m_nPlayerState);
	DECL_EXTRACT (CTFPlayer *, m_pOuter);
	
	static MemberFuncThunk<      CTFPlayerShared *, void, ETFCond, float, CBaseEntity * > ft_AddCond;
	static MemberFuncThunk<      CTFPlayerShared *, void, ETFCond, bool                 > ft_RemoveCond;
	static MemberFuncThunk<const CTFPlayerShared *, bool, ETFCond                       > ft_InCond;
	static MemberFuncThunk<const CTFPlayerShared *, bool                                > ft_IsInvulnerable;
	static MemberFuncThunk<      CTFPlayerShared *, void, float, float, int, CTFPlayer *> ft_StunPlayer;
	static MemberFuncThunk<      CTFPlayerShared *, void, CBitVec<192>&                 > ft_GetConditionsBits;
	static MemberFuncThunk<      CTFPlayerShared *, float, ETFCond                      > ft_GetConditionDuration;
	static MemberFuncThunk<      CTFPlayerShared *, CBaseEntity *, ETFCond              > ft_GetConditionProvider;
	static MemberFuncThunk<const CTFPlayerShared *, int                                 > ft_GetDisguiseTeam;
	static MemberFuncThunk<const CTFPlayerShared *, bool                                > ft_IsStealthed;
	static MemberFuncThunk<const CTFPlayerShared *, float                               > ft_GetPercentInvisible;
	static MemberFuncThunk<      CTFPlayerShared *, bool                                > ft_IsControlStunned;
	static MemberFuncThunk<const CTFPlayerShared *, bool                                > ft_IsLoserStateStunned;
	static MemberFuncThunk<      CTFPlayerShared *, void                                > ft_SetDefaultItemChargeMeters;
	static MemberFuncThunk<      CTFPlayerShared *, void, loadout_positions_t, float    > ft_SetItemChargeMeter;
	static MemberFuncThunk<      CTFPlayerShared *, void, CTFPlayer *, CTFWeaponBase*, float   > ft_Burn;
};

class CTFPlayer : public CBaseMultiplayerPlayer
{
public:
	CTFPlayerClass *GetPlayerClass()             { return &m_PlayerClass; }
	const CTFPlayerClass *GetPlayerClass() const { return &m_PlayerClass; }
	
	bool IsPlayerClass(int iClass) const;
	int StateGet() const        { return this->m_Shared->GetState(); }
	bool HasItem() const        { return this->m_hItem != nullptr; }
	CTFItem *GetItem() const    { return this->m_hItem; }
	bool IsMiniBoss() const     { return this->m_bIsMiniBoss; }
	int GetCurrency() const     { return this->m_nCurrency; }
	void SetMiniBoss(bool boss) { this->m_bIsMiniBoss = boss; }
	void SetForcedSkin(int skin){ this->m_bForcedSkin = true; this->m_nForcedSkin = skin; }
	int GetForcedSkin()         { return this->m_nForcedSkin; }
	void ResetForcedSkin()      { this->m_bForcedSkin = false; this->m_nForcedSkin = 0; }

	CBaseEntity *GetGrapplingHookTarget() const {return this->m_hGrapplingHookTarget;}
	
	CTFWeaponBase *GetActiveTFWeapon() const;
	
	void ForceChangeTeam(int team, bool b1)                      {        ft_ForceChangeTeam                  (this, team, b1); }
	void ClientCommand(CCommand& command)                        {        ft_ClientCommand                    (this, command); }
	void StartBuildingObjectOfType(int iType, int iMode)         {        ft_StartBuildingObjectOfType        (this, iType, iMode); }
	bool HasTheFlag(ETFFlagType *p1 = nullptr, int i1 = 0) const { return ft_HasTheFlag                       (this, p1, i1); }
	int GetAutoTeam(int team)                                    { return ft_GetAutoTeam                      (this, team); }
	float MedicGetChargeLevel(CTFWeaponBase **medigun = nullptr) { return ft_MedicGetChargeLevel              (this, medigun); }
	float TeamFortress_CalculateMaxSpeed(bool b1 = false)        { return ft_TeamFortress_CalculateMaxSpeed   (this, b1); }
	void UpdateModel()                                           {        ft_UpdateModel                      (this); }
	CTFWeaponBase *Weapon_OwnsThisID(int id) const               { return ft_Weapon_OwnsThisID                (this, id); }
	CTFWeaponBase *Weapon_GetWeaponByType(int type)              { return ft_Weapon_GetWeaponByType           (this, type); }
	CBaseObject *GetObjectOfType(int iType, int iMode)           { return ft_GetObjectOfType                  (this, iType, iMode); }
	int GetMaxAmmo(int iAmmoIndex, int iClassNumber = -1)        { return ft_GetMaxAmmo                       (this, iAmmoIndex, iClassNumber); }
	CTFWearable *GetEquippedWearableForLoadoutSlot(int iSlot)    { return ft_GetEquippedWearableForLoadoutSlot(this, iSlot); }
	CBaseEntity *GetEntityForLoadoutSlot(int iSlot)    { return ft_GetEntityForLoadoutSlot(this, iSlot); }
	void RemoveInvisibility()                                    {        ft_RemoveInvisibility               (this); }
	void DoAnimationEvent(PlayerAnimEvent_t event, int nData = 0){        ft_DoAnimationEvent                 (this, event, nData); }
	void PlaySpecificSequence (const char *sequence)             {        ft_PlaySpecificSequence   (this, sequence); }
	CBaseObject *GetObject(int id)                               { return ft_GetObject                      (this, id); }
	int GetObjectCount()                                         { return ft_GetObjectCount                 (this); }
	void StateTransition(int state)                              {        ft_StateTransition                (this, state); }

	void HandleCommand_JoinTeam(const char *pTeamName)                   { ft_HandleCommand_JoinTeam        (this, pTeamName); }
	void HandleCommand_JoinTeam_NoMenus(const char *pTeamName)           { ft_HandleCommand_JoinTeam_NoMenus(this, pTeamName); }
	void HandleCommand_JoinClass(const char *pClassName, bool b1 = true) { ft_HandleCommand_JoinClass       (this, pClassName, b1); }
	
	void AddCustomAttribute(const char *s1, float f1, float f2) { ft_AddCustomAttribute       (this, s1, f1, f2); }
	void RemoveCustomAttribute(const char *s1)                  { ft_RemoveCustomAttribute    (this, s1); }
	void RemoveAllCustomAttributes()                            { ft_RemoveAllCustomAttributes(this); }
	void ReapplyPlayerUpgrades()                                { ft_ReapplyPlayerUpgrades  (this); }
	
	void UseActionSlotItemPressed()								{ ft_UseActionSlotItemPressed  (this);  }
	void UseActionSlotItemReleased()							{ ft_UseActionSlotItemReleased  (this); }
	void RemoveCurrency(int currency)                           { ft_RemoveCurrency  (this, currency);  }

	CAttributeList *GetAttributeList()								{ return ft_GetAttributeList  (this); }
	CAttributeManager *GetAttributeManager()						{ return ft_GetAttributeManager  (this); }
	CBaseEntity *GiveNamedItem(const char *pszName, int iSubType, CEconItemView *pItem, bool bDontTranslateForClass) { return vt_GiveNamedItem(this, pszName, iSubType, pItem, bDontTranslateForClass); }

	void Taunt(taunts_t taunt, int concept)                                       { ft_Taunt                   (this, taunt, concept); }
	void PlayTauntSceneFromItem(CEconItemView *view)                              { ft_PlayTauntSceneFromItem  (this, view); }

	void CheckInstantLoadoutRespawn()                              { ft_CheckInstantLoadoutRespawn  (this); }
	void ForceRegenerateAndRespawn()                               { ft_ForceRegenerateAndRespawn  (this); }
	
	void ApplyGenericPushbackImpulse(Vector &vec)                  { ft_ApplyGenericPushbackImpulse (this, vec); }
	
	
	CEconEntity *GetEconEntityByName(const char *name);
	CEconEntity *GetEconEntityById(int id);

	DECL_SENDPROP_RW(CTFPlayerShared,      m_Shared);
	DECL_SENDPROP   (float,                m_flMvMLastDamageTime);
	DECL_RELATIVE   (CTFPlayerAnimState *, m_PlayerAnimState);
	DECL_RELATIVE   (bool,                 m_bFeigningDeath);
	DECL_SENDPROP   (CHandle<CBaseEntity>, m_hGrapplingHookTarget);
	DECL_SENDPROP   (int,        m_nBotSkill);
	DECL_SENDPROP   (bool,       m_bAllowMoveDuringTaunt);
	DECL_SENDPROP   (float,      m_flCurrentTauntMoveSpeed);
	DECL_SENDPROP   (short,      m_iTauntItemDefIndex);
	
private:
	DECL_SENDPROP_RW(CTFPlayerClass,   m_PlayerClass);
	DECL_SENDPROP   (CHandle<CTFItem>, m_hItem);
	DECL_SENDPROP   (bool,             m_bIsMiniBoss);
	DECL_SENDPROP   (int,              m_nCurrency);
	DECL_SENDPROP   (bool,             m_bForcedSkin);
	DECL_SENDPROP   (int,              m_nForcedSkin);
	DECL_SENDPROP   (bool,             m_bArenaSpectator);
	
	static MemberFuncThunk<      CTFPlayer *, void, int, bool                 > ft_ForceChangeTeam;
	static MemberFuncThunk<      CTFPlayer *, void, CCommand&                 > ft_ClientCommand;
	static MemberFuncThunk<      CTFPlayer *, void, int, int                  > ft_StartBuildingObjectOfType;
	static MemberFuncThunk<const CTFPlayer *, bool, ETFFlagType *, int        > ft_HasTheFlag;
	static MemberFuncThunk<      CTFPlayer *, int, int                        > ft_GetAutoTeam;
	static MemberFuncThunk<      CTFPlayer *, float, CTFWeaponBase **         > ft_MedicGetChargeLevel;
	static MemberFuncThunk<      CTFPlayer *, float, bool                     > ft_TeamFortress_CalculateMaxSpeed;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_UpdateModel;
	static MemberFuncThunk<const CTFPlayer *, CTFWeaponBase *, int            > ft_Weapon_OwnsThisID;
	static MemberFuncThunk<      CTFPlayer *, CTFWeaponBase *, int            > ft_Weapon_GetWeaponByType;
	static MemberFuncThunk<      CTFPlayer *, CBaseObject *, int, int         > ft_GetObjectOfType;
	static MemberFuncThunk<      CTFPlayer *, int, int, int                   > ft_GetMaxAmmo;
	static MemberFuncThunk<      CTFPlayer *, CTFWearable *, int              > ft_GetEquippedWearableForLoadoutSlot;
	static MemberFuncThunk<      CTFPlayer *, CBaseEntity *, int              > ft_GetEntityForLoadoutSlot;
	static MemberFuncThunk<      CTFPlayer *, void, const char *              > ft_HandleCommand_JoinTeam;
	static MemberFuncThunk<      CTFPlayer *, void, const char *              > ft_HandleCommand_JoinTeam_NoMenus;
	static MemberFuncThunk<      CTFPlayer *, void, const char *, bool        > ft_HandleCommand_JoinClass;
	static MemberFuncThunk<      CTFPlayer *, void, const char *, float, float> ft_AddCustomAttribute;
	static MemberFuncThunk<      CTFPlayer *, void, const char *              > ft_RemoveCustomAttribute;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_RemoveAllCustomAttributes;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_ReapplyPlayerUpgrades;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_UseActionSlotItemPressed;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_UseActionSlotItemReleased;
	static MemberFuncThunk<      CTFPlayer *, CAttributeList *                > ft_GetAttributeList;
	static MemberFuncThunk<      CTFPlayer *, CAttributeManager *             > ft_GetAttributeManager;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_RemoveInvisibility;
	static MemberFuncThunk<      CTFPlayer *, void, PlayerAnimEvent_t, int    > ft_DoAnimationEvent;
	static MemberFuncThunk<      CTFPlayer *, void, const char *              > ft_PlaySpecificSequence;
	static MemberFuncThunk<      CTFPlayer *, void, taunts_t, int             > ft_Taunt;
	static MemberFuncThunk<      CTFPlayer *, void, CEconItemView*            > ft_PlayTauntSceneFromItem;
	static MemberFuncThunk<      CTFPlayer *, CBaseObject *, int              > ft_GetObject;
	static MemberFuncThunk<      CTFPlayer *, int                             > ft_GetObjectCount;
	static MemberFuncThunk<      CTFPlayer *, void, int                       > ft_StateTransition;
	static MemberFuncThunk<      CTFPlayer *, void, int                       > ft_RemoveCurrency;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_CheckInstantLoadoutRespawn;
	static MemberFuncThunk<      CTFPlayer *, void                            > ft_ForceRegenerateAndRespawn;
	static MemberFuncThunk<      CTFPlayer *, void, Vector &                  > ft_ApplyGenericPushbackImpulse;
	
	static MemberFuncThunk<CTFPlayer *, CBaseEntity *, const char *, int, CEconItemView *, bool> vt_GiveNamedItem;
};

class CTFPlayerSharedUtils
{
public:
	static CEconItemView *GetEconItemViewByLoadoutSlot(CTFPlayer *player, int slot, CEconEntity **ent = nullptr);
	
private:
	static StaticFuncThunk<CEconItemView *, CTFPlayer *, int, CEconEntity **> ft_GetEconItemViewByLoadoutSlot;
};

class CTFRagdoll : public CBaseAnimatingOverlay
{
public:
	DECL_SENDPROP(float, m_flHeadScale);
	DECL_SENDPROP(float, m_flTorsoScale);
	DECL_SENDPROP(float, m_flHandScale);
};


inline CTFPlayer *CTFPlayerClassShared::GetOuter()
{
	static int off = Prop::FindOffsetAssert("CTFPlayer", "m_PlayerClass");
	return (CTFPlayer *)((uintptr_t)this - off);
}

inline void CTFPlayerClassShared::NetworkStateChanged()           { this->GetOuter()->NetworkStateChanged(); }
inline void CTFPlayerClassShared::NetworkStateChanged(void *pVar) { this->GetOuter()->NetworkStateChanged(pVar); }


//inline CTFPlayer *CTFPlayerShared::GetOuter()
//{
//	static int off = Prop::FindOffsetAssert("CTFPlayer", "m_Shared");
//	return (CTFPlayer *)((uintptr_t)this - off);
//}

inline void CTFPlayerShared::NetworkStateChanged()           { this->GetOuter()->NetworkStateChanged(); }
inline void CTFPlayerShared::NetworkStateChanged(void *pVar) { this->GetOuter()->NetworkStateChanged(pVar); }


inline CTFPlayer *ToTFPlayer(CBaseEntity *pEntity)
{
	if (pEntity == nullptr)   return nullptr;
	if (!pEntity->IsPlayer()) return nullptr;
	
	// Its not really probable for a player not to be an instance of CTFPlayer
	return static_cast<CTFPlayer *>(pEntity);

	// return rtti_cast<CTFPlayer *>(pEntity);
}

int GetNumberOfTFConds();

bool IsValidTFConditionNumber(int num);
ETFCond ClampTFConditionNumber(int num);

const char *GetTFConditionName(ETFCond cond);
ETFCond GetTFConditionFromName(const char *name);

extern StaticFuncThunk<int, CUtlVector<CTFPlayer *> *, int, bool, bool> ft_CollectPlayers_CTFPlayer;
template<> inline int CollectPlayers<CTFPlayer>(CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend) { return ft_CollectPlayers_CTFPlayer(playerVector, team, isAlive, shouldAppend); }

extern StaticFuncThunk<void, CBasePlayer *, int, int> ft_TE_PlayerAnimEvent;
inline void TE_PlayerAnimEvent(CBasePlayer *player, int anim, int data) { ft_TE_PlayerAnimEvent(player, anim, data); }

bool GiveItemToPlayer(CTFPlayer *player, CEconEntity *entity, bool no_remove, bool force_give, const char *item_name);


#endif
