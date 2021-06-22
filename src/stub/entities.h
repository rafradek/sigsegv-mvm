#ifndef _INCLUDE_SIGSEGV_STUB_ENTITIES_H_
#define _INCLUDE_SIGSEGV_STUB_ENTITIES_H_


#include "link/link.h"
#include "prop.h"
#include "stub/baseplayer.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"
#include "stub/projectiles.h"

class CPointEntity : public CBaseEntity {};


class CAmbientGeneric : public CPointEntity {};


class CPathTrack : public CPointEntity
{
public:
	CPathTrack *GetNext() { return ft_GetNext(this); }
	
	DECL_DATAMAP(int, m_eOrientationType);
	
private:
	static MemberFuncThunk<CPathTrack *, CPathTrack *> ft_GetNext;
};

class CEnvEntityMaker : public CPointEntity
{
public:
	DECL_DATAMAP(string_t,               m_iszTemplate);
	DECL_DATAMAP(QAngle,                 m_angPostSpawnDirection);
	DECL_DATAMAP(float,                  m_flPostSpawnDirectionVariance);
	DECL_DATAMAP(float,                  m_flPostSpawnSpeed);
	DECL_DATAMAP(bool,                   m_bPostSpawnUseAngles);
};

class CTFDroppedWeapon : public CBaseAnimating {
public:
	static CTFDroppedWeapon *Create(CTFPlayer *pLastOwner, const Vector &vecOrigin, const QAngle &vecAngles, const char *pszModelName, const CEconItemView *pItem) { return ft_Create(pLastOwner, vecOrigin, vecAngles, pszModelName, pItem); }
	
	DECL_SENDPROP(CEconItemView, m_Item);
private:
	static StaticFuncThunk<CTFDroppedWeapon *, CTFPlayer *, const Vector &, const QAngle &, const char *, const CEconItemView *> ft_Create;
};


class CItem : public CBaseAnimating
{
	DECL_DATAMAP(float,                m_flNextResetCheckTime);
	DECL_DATAMAP(bool,                 m_bActivateWhenAtRest);
	DECL_DATAMAP(Vector,               m_vOriginalSpawnOrigin);
	DECL_DATAMAP(QAngle,               m_vOriginalSpawnAngles);
	DECL_DATAMAP(IPhysicsConstraint *, m_pConstraint);
};

class CTFAmmoPack : public CItem {};

class CTFPowerup : public CItem
{
public:
	float GetLifeTime() { return vt_GetLifeTime(this); }
	void DropSingleInstance(Vector &vecLaunchVel, CBaseCombatCharacter *pThrower, float flThrowerTouchDelay, float flResetTime = 0.1f) { return ft_DropSingleInstance(this, vecLaunchVel, pThrower, flThrowerTouchDelay, flResetTime); }
	
	DECL_DATAMAP(bool,     m_bDisabled);
	DECL_DATAMAP(bool,     m_bAutoMaterialize);
	DECL_DATAMAP(string_t, m_iszModel);
	
private:
	static MemberVFuncThunk<CTFPowerup *, float> vt_GetLifeTime;

	static MemberFuncThunk<CTFPowerup *, void, Vector &, CBaseCombatCharacter *, float, float> ft_DropSingleInstance;
};

class CSpellPickup : public CTFPowerup
{
public:
	DECL_DATAMAP(int, m_nTier);
};


class CTFReviveMarker : public CBaseAnimating
{
public:
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hOwner);   // +0x488
	// 48c
	DECL_SENDPROP(short,                m_nRevives); // +0x490
	// 494 dword 0
	// 498 byte 0
	// 499 byte
	// 49a byte 0
	// 49b?
	// 49c float 0.0f
	// 4a0 byte, probably: have we landed on the ground
};


class CTFMedigunShield : public CBaseAnimating {};


class IHasGenericMeter
{
public:
	bool ShouldUpdateMeter() const   { return vt_ShouldUpdateMeter    (rtti_cast<const IHasGenericMeter *>(this)); }
	float GetMeterMultiplier() const { return vt_GetMeterMultiplier   (rtti_cast<const IHasGenericMeter *>(this)); }
	void OnResourceMeterFilled()     {        vt_OnResourceMeterFilled(rtti_cast<      IHasGenericMeter *>(this)); }
	float GetChargeInterval() const  { return vt_GetChargeInterval    (rtti_cast<const IHasGenericMeter *>(this)); }
	
private:
	static MemberVFuncThunk<const IHasGenericMeter *, bool>  vt_ShouldUpdateMeter;
	static MemberVFuncThunk<const IHasGenericMeter *, float> vt_GetMeterMultiplier;
	static MemberVFuncThunk<      IHasGenericMeter *, void>  vt_OnResourceMeterFilled;
	static MemberVFuncThunk<const IHasGenericMeter *, float> vt_GetChargeInterval;
};


class CEconWearable : public CEconEntity
{
public:
	void RemoveFrom(CBaseEntity *ent) { vt_RemoveFrom(this, ent); }
	// TODO: CanEquip
	// TODO: Equip
	void UnEquip(CBasePlayer *player) { vt_UnEquip   (this, player); }
	// TODO: OnWearerDeath
	// TODO: GetDropType
	// TODO: IsViewModelWearable
	// TODO: GetSkin
	// TODO: InternalSetPlayerDisplayModel
	
private:
	static MemberVFuncThunk<CEconWearable *, void, CBaseEntity *> vt_RemoveFrom;
	static MemberVFuncThunk<CEconWearable *, void, CBasePlayer *> vt_UnEquip;
};

class CTFWearable : public CEconWearable, public IHasGenericMeter
{
public:
	DECL_SENDPROP(CHandle<CBaseEntity *>, m_hWeaponAssociatedWith);
};

class CTFWearableDemoShield : public CTFWearable {};
class CTFWearableRobotArm   : public CTFWearable {};
class CTFWearableRazorback  : public CTFWearable {};
class CTFPowerupBottle      : public CTFWearable {
	
public:
	DECL_SENDPROP(int, m_usNumCharges);
};


class CBaseTFBotHintEntity : public CPointEntity {};
class CTFBotHintSentrygun : public CBaseTFBotHintEntity {};
class CTFBotHintTeleporterExit : public CBaseTFBotHintEntity {};

class CTFBotHintEngineerNest : public CBaseTFBotHintEntity
{
public:
	bool IsStaleNest() const { return ft_IsStaleNest      (this); }
	void DetonateStaleNest() {        ft_DetonateStaleNest(this); }
	
	DECL_SENDPROP(bool, m_bHasActiveTeleporter);
	
private:
	static MemberFuncThunk<const CTFBotHintEngineerNest *, bool> ft_IsStaleNest;
	static MemberFuncThunk<      CTFBotHintEngineerNest *, void> ft_DetonateStaleNest;
};


class ITFBotHintEntityAutoList
{
public:
	static const CUtlVector<ITFBotHintEntityAutoList *>& AutoList() { return m_ITFBotHintEntityAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<ITFBotHintEntityAutoList *>> m_ITFBotHintEntityAutoListAutoList;
};


class CBaseProp : public CBaseAnimating {};
class CBreakableProp : public CBaseProp {};
class CDynamicProp : public CBreakableProp {};

class CTFItem : public CDynamicProp
{
public:
	int GetItemID() const { return vt_GetItemID(this); }
	
private:
	static MemberVFuncThunk<const CTFItem *, int> vt_GetItemID;
};

class CCaptureFlag : public CTFItem
{
public:
	bool IsDropped()        { return (this->m_nFlagStatus == TF_FLAGINFO_DROPPED); }
	bool IsHome()           { return (this->m_nFlagStatus == TF_FLAGINFO_NONE); }
	bool IsStolen()         { return (this->m_nFlagStatus == TF_FLAGINFO_STOLEN); }
	bool IsDisabled() const { return this->m_bDisabled; }
	
private:
	DECL_SENDPROP(bool, m_bDisabled);
	DECL_SENDPROP(int,  m_nFlagStatus);
};


class ICaptureFlagAutoList
{
public:
	static const CUtlVector<ICaptureFlagAutoList *>& AutoList() { return m_ICaptureFlagAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<ICaptureFlagAutoList *>> m_ICaptureFlagAutoListAutoList;
};

class IBody;

class CBaseToggle : public CBaseEntity {};

class CBaseTrigger : public CBaseToggle
{
public:
	DECL_DATAMAP(bool, m_bDisabled);
};

class CUpgrades : public CBaseTrigger
{
public:
	const char *GetUpgradeAttributeName(int index) const { return ft_GetUpgradeAttributeName(this, index); }
	void GrantOrRemoveAllUpgrades(CTFPlayer *player, bool remove = false, bool refund = true) const { ft_GrantOrRemoveAllUpgrades(this, player, remove, refund); };
	void PlayerPurchasingUpgrade(CTFPlayer *player, int itemslot, int upgradeslot, bool sell, bool free, bool refund) { ft_PlayerPurchasingUpgrade(this, player, itemslot, upgradeslot, sell, free, refund); };

	
private:
	static MemberFuncThunk<const CUpgrades *, const char *, int> ft_GetUpgradeAttributeName;
	static MemberFuncThunk<const CUpgrades *, void, CTFPlayer *, bool , bool > ft_GrantOrRemoveAllUpgrades;
	static MemberFuncThunk<CUpgrades *, void, CTFPlayer *, int , int, bool, bool, bool > ft_PlayerPurchasingUpgrade;
};
extern GlobalThunk<CHandle<CUpgrades>> g_hUpgradeEntity;

class CFuncNavPrerequisite : public CBaseTrigger
{
public:
	enum TaskType : int32_t
	{
		DESTROY_ENTITY = 1,
		MOVE_TO        = 2,
		WAIT           = 3,
	};
	
	DECL_DATAMAP(int,      m_task);
	DECL_DATAMAP(string_t, m_taskEntityName);
	DECL_DATAMAP(float,    m_taskValue);
	DECL_DATAMAP(bool,     m_isDisabled);
	// CHandle<T> @ m_isDisabled+4
};


class CServerOnlyEntity : public CBaseEntity {};
class CLogicalEntity : public CServerOnlyEntity {};

class CLogicCase : public CLogicalEntity
{
public:
	DECL_DATAMAP(CBaseEntityOutput, m_OnDefault);

};

class CBaseFilter : public CLogicalEntity
{
public:
	// TODO
};

class CFilterMultiple : public CBaseFilter
{
public:
	// TODO
};

class CFilterTFBotHasTag : public CBaseFilter
{
public:
	DECL_RELATIVE(CUtlStringList, m_TagList);
	DECL_DATAMAP (string_t,       m_iszTags);
	DECL_DATAMAP (bool,           m_bRequireAllTags);
	DECL_DATAMAP (bool,           m_bNegated);
};


class CCurrencyPack : public CTFPowerup
{
public:
	bool HasBeenTouched() const { return this->m_bTouched; }
	bool IsBeingPulled() const  { return this->m_bPulled; }
	bool IsDistributed() const  { return this->m_bDistributed; }
	int GetAmount() const       { return this->m_nAmount; }

	void SetDistributed(bool val) { this->m_bDistributed = val; }
	void SetAmount(int amount)    { this->m_nAmount = amount; }
	
	bool AffectedByRadiusCollection() const { return vt_AffectedByRadiusCollection(this); }
	
private:
	DECL_RELATIVE   (bool, m_bTouched);
	DECL_RELATIVE   (bool, m_bPulled);
	DECL_SENDPROP_RW(bool, m_bDistributed);
	DECL_EXTRACT    (int,  m_nAmount);
	
	static MemberVFuncThunk<const CCurrencyPack *, bool> vt_AffectedByRadiusCollection;
};

class CCurrencyPackCustom : public CCurrencyPack
{

};

class ICurrencyPackAutoList
{
public:
	static const CUtlVector<ICurrencyPackAutoList *>& AutoList() { return m_ICurrencyPackAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<ICurrencyPackAutoList *>> m_ICurrencyPackAutoListAutoList;
};


class CCaptureZone : public CBaseTrigger
{
public:
	bool IsDisabled() const { return this->m_bDisabled; }
	
	void Capture(CBaseEntity *ent) { ft_Capture(this, ent); }
	
private:
	// yes, they really put a member variable in this class with the same name
	// as one that's in the parent class... sigh...
	DECL_SENDPROP(bool, m_bDisabled);
	
	static MemberFuncThunk<CCaptureZone *, void, CBaseEntity *> ft_Capture;
};


class ICaptureZoneAutoList
{
public:
	static const CUtlVector<ICaptureZoneAutoList *>& AutoList() { return m_ICaptureZoneAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<ICaptureZoneAutoList *>> m_ICaptureZoneAutoListAutoList;
};


class CTeamControlPoint : public CBaseAnimating
{
public:
	
};


class CFuncRespawnRoom : public CBaseTrigger {};
bool PointInRespawnRoom(const CBaseEntity *ent, const Vector& vec, bool b1);


class CTeamControlPointMaster : public CBaseEntity
{
public:
	using ControlPointMap = CUtlMap<int, CTeamControlPoint *>;
#if TOOLCHAIN_FIXES
	DECL_EXTRACT(ControlPointMap, m_ControlPoints);
#endif
};
extern GlobalThunk<CUtlVector<CHandle<CTeamControlPointMaster>>> g_hControlPointMasters;


class CTFTeamSpawn : public CPointEntity
{
public:
	DECL_DATAMAP(bool,     m_bDisabled);
	DECL_DATAMAP(int,      m_nSpawnMode);
	DECL_DATAMAP(string_t, m_iszControlPointName);
	DECL_DATAMAP(string_t, m_iszRoundBlueSpawn);
	DECL_DATAMAP(string_t, m_iszRoundRedSpawn);
};


class CTFFlameEntity : public CBaseEntity {};

class ITFFlameEntityAutoList
{
public:
	static const CUtlVector<ITFFlameEntityAutoList *>& AutoList() { return m_ITFFlameEntityAutoListAutoList; }
private:
	static GlobalThunk<CUtlVector<ITFFlameEntityAutoList *>> m_ITFFlameEntityAutoListAutoList;
};


class CBaseParticleEntity : public CBaseEntity {};

class CSmokeStack : public CBaseParticleEntity
{
public:
	DECL_DATAMAP (string_t, m_strMaterialModel);
	DECL_SENDPROP(int,      m_iMaterialModel);
};

class CTFPointWeaponMimic : public CPointEntity
{
public:
	DECL_DATAMAP (bool, m_bCrits);
	DECL_RELATIVE(CUtlVector<CHandle<CTFGrenadePipebombProjectile>>,      m_Pipebombs);
};

class CGameUI : public CBaseEntity
{
public:
	DECL_DATAMAP (CHandle<CBasePlayer>, m_player);
};

class CMonsterResource : public CBaseEntity
{
public:
	DECL_SENDPROP(int, m_iBossHealthPercentageByte);
	DECL_SENDPROP(int, m_iBossStunPercentageByte);
	DECL_SENDPROP(int, m_iBossState);
};

extern GlobalThunk<CHandle<CMonsterResource>> g_pMonsterResource;

// 20151007a

// CTFPlayer::Event_Killed
// - CTFReviveMarker::Create
//   - CBaseEntity::Create
//     - ctor
//       - m_nRevives = 0
//     - CTFReviveMarker::Spawn
//       - m_iHealth = 1
//       - set model, solid, collision, move, effects, sequence, think, etc
//   - SetOwner
//     - m_hOwner = player
//     - change team to match player
//     - set bodygroup based on player class
//     - set angles to match player
//   - change team to match player
// - player->handle_0x2974 = marker

// CTFReviveMarker::ReviveThink (100 ms interval)
// - if m_hOwner == null: UTIL_Remove(this)
// - if m_hOwner not in same team: UTIL_Remove(this)
// - if m_iMaxHealth == 0:
//   - maxhealth = (float)(player->GetMaxHealth() / 2)
//     - TODO: does CTFPlayer::GetMaxHealth factor in items and stuff?
//   - if (stats = CTF_GameStats.FindPlayerStats(player)) != null:
//     - m_nRevives = (probably round stat TFSTAT_DEATHS)
//     - maxhealth += 10 * old value of m_nRevives
//   - m_iMaxHealth = (int)maxhealth
// - if on the ground and haven't been yet
//   - set the bool for that (+0x4a0)
//   - if landed in a trigger_hurt: UTIL_Remove(this)
// - if m_nRevives > 7:   DispatchParticleEffect("speech_revivecall_hard")
// - elif m_nRevives > 3: DispatchParticleEffect("speech_revivecall_medium")
// - else:                DispatchParticleEffect("speech_revivecall")
// - TODO: stuff related to "revive_player_stopped"

// CTFReviveMarker::AddMarkerHealth
// - 


#endif
