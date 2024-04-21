#ifndef _INCLUDE_SIGSEGV_STUB_TFENTITIES_H_
#define _INCLUDE_SIGSEGV_STUB_TFENTITIES_H_

#include "stub/entities.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"

class CTFDroppedWeapon : public CBaseAnimating {
public:
	static CTFDroppedWeapon *Create(CTFPlayer *pLastOwner, const Vector &vecOrigin, const QAngle &vecAngles, const char *pszModelName, const CEconItemView *pItem) { return ft_Create(pLastOwner, vecOrigin, vecAngles, pszModelName, pItem); }
	void InitDroppedWeapon(CTFPlayer *pPlayer, CTFWeaponBase *pWeapon,  bool bSwap, bool bIsSuicide)  { return ft_InitDroppedWeapon(this, pPlayer, pWeapon, bSwap, bIsSuicide); }

	DECL_SENDPROP(CEconItemView, m_Item);
	DECL_SENDPROP(float, m_flChargeLevel);
	DECL_RELATIVE(int, m_nAmmo);
private:
	static StaticFuncThunk<CTFDroppedWeapon *, CTFPlayer *, const Vector &, const QAngle &, const char *, const CEconItemView *> ft_Create;
	static MemberFuncThunk<CTFDroppedWeapon *, void, CTFPlayer *, CTFWeaponBase *, bool, bool> ft_InitDroppedWeapon;
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
	static CTFReviveMarker *Create(CTFPlayer *player) { return ft_Create(player); }

private:
	static StaticFuncThunk<CTFReviveMarker *, CTFPlayer *> ft_Create;
};


class CTFMedigunShield : public CBaseAnimating {};


class IHasGenericMeter
{
public:
	virtual bool ShouldUpdateMeter() const { return false; }
	virtual float GetMeterMultiplier() const { return 0.0f; }
	virtual void OnResourceMeterFilled() { return; }
	virtual float GetChargeInterval() const { return 0.0f; }
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
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hWeaponAssociatedWith);
	DECL_SENDPROP(bool, m_bDisguiseWearable);
	DECL_RELATIVE(bool, m_bAlwaysAllow);
};

class CTFWearableRobotArm   : public CTFWearable {};
class CTFWearableRazorback  : public CTFWearable {};
class CTFPowerupBottle      : public CTFWearable {
	
public:
	DECL_SENDPROP(int, m_usNumCharges);
};
class CTFWearableDemoShield : public CTFWearable {
public:
	void DoSpecialAction(CTFPlayer *player)  { ft_DoSpecialAction       (this, player); }
	void EndSpecialAction(CTFPlayer *player) { ft_EndSpecialAction      (this, player); }

private:
	static MemberFuncThunk<CTFWearableDemoShield *, void, CTFPlayer *> ft_DoSpecialAction;
	static MemberFuncThunk<CTFWearableDemoShield *, void, CTFPlayer *> ft_EndSpecialAction;
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
	void SetDisabled(bool disabled)      { this->m_bDisabled = disabled; }
	
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

class CUpgrades : public CBaseTrigger
{
public:
	const char *GetUpgradeAttributeName(int index) const { return ft_GetUpgradeAttributeName(this, index); }
	void GrantOrRemoveAllUpgrades(CTFPlayer *player, bool remove = false, bool refund = true) const { ft_GrantOrRemoveAllUpgrades(this, player, remove, refund); };
	void PlayerPurchasingUpgrade(CTFPlayer *player, int itemslot, int upgradeslot, bool sell, bool free, bool refund) { ft_PlayerPurchasingUpgrade(this, player, itemslot, upgradeslot, sell, free, refund); };
	attrib_definition_index_t ApplyUpgradeToItem(CTFPlayer *pTFPlayer, CEconItemView *pView, int iUpgrade, int nCost, bool bDowngrade = false, bool bIsFresh = false) { return ft_ApplyUpgradeToItem(this, pTFPlayer, pView, iUpgrade, nCost, bDowngrade, bIsFresh); };
	
	
private:
	static MemberFuncThunk<const CUpgrades *, const char *, int> ft_GetUpgradeAttributeName;
	static MemberFuncThunk<const CUpgrades *, void, CTFPlayer *, bool , bool > ft_GrantOrRemoveAllUpgrades;
	static MemberFuncThunk<CUpgrades *, void, CTFPlayer *, int , int, bool, bool, bool > ft_PlayerPurchasingUpgrade;
	static MemberFuncThunk<CUpgrades *, attrib_definition_index_t,  CTFPlayer*, CEconItemView *, int, int, bool, bool > ft_ApplyUpgradeToItem;
};
extern GlobalThunk<CHandle<CUpgrades>> g_hUpgradeEntity;

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
	void DistributedBy(CTFPlayer *player) { ft_DistributedBy(this, player); }
	
private:
	DECL_RELATIVE   (bool, m_bTouched);
	DECL_RELATIVE   (bool, m_bPulled);
	DECL_SENDPROP   (bool, m_bDistributed);
	DECL_RELATIVE   (int,  m_nAmount);
	
	static MemberVFuncThunk<const CCurrencyPack *, bool> vt_AffectedByRadiusCollection;

	static MemberFuncThunk<CCurrencyPack *, void, CTFPlayer *> ft_DistributedBy;
	
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


class CTFPointWeaponMimic : public CPointEntity
{
public:

	QAngle GetFiringAngles() const { return ft_GetFiringAngles(this); }
	void   Fire()                  {        ft_Fire(this); }

	DECL_DATAMAP (bool, m_bCrits);
	DECL_DATAMAP (float, m_flSpreadAngle);
	DECL_DATAMAP (float, m_flDamage);
	DECL_DATAMAP (float, m_flSpeedMax);
	DECL_DATAMAP (float, m_flSplashRadius);
	DECL_DATAMAP (string_t, m_pzsFireParticles);
	DECL_DATAMAP (string_t, m_pzsFireSound);
	DECL_DATAMAP (string_t, m_pzsModelOverride);
	
	DECL_DATAMAP (int, m_nWeaponType);
	
	DECL_RELATIVE(CUtlVector<CHandle<CTFGrenadePipebombProjectile>>,      m_Pipebombs);

private:
	static MemberFuncThunk<const CTFPointWeaponMimic *, QAngle> ft_GetFiringAngles;
	static MemberFuncThunk<CTFPointWeaponMimic *, void> ft_Fire;
};

class CMonsterResource : public CBaseEntity
{
public:
	DECL_SENDPROP(int, m_iBossHealthPercentageByte);
	DECL_SENDPROP(int, m_iBossStunPercentageByte);
	DECL_SENDPROP(int, m_iBossState);
};

extern GlobalThunk<CMonsterResource *> g_pMonsterResource;

class CTriggerIgnite : public CBaseTrigger
{
public:
	DECL_DATAMAP (float, m_flBurnDuration);
};

class CTriggerCatapult : public CBaseTrigger
{
public:
	DECL_DATAMAP (float[MAX_PLAYERS + 1], m_flRefireDelay);
};
#endif