#include "stub/tfentities.h"
#include "mem/extract.h"

#if defined _LINUX

static constexpr uint8_t s_Buf_CCurrencyPack_m_nAmount[] = {
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0xf3, 0x0f, 0x10, 0x45, 0x0c,       // +0003  movss xmm0,[ebp+0xc]
	0x8b, 0x45, 0x08,                   // +0008  mov eax,[ebp+this]
	0xf3, 0x0f, 0x2c, 0xd0,             // +000B  cvttss2si edx,xmm0
	0x89, 0x90, 0xfc, 0x04, 0x00, 0x00, // +000F  mov [eax+0xVVVVVVVV],edx
	0x5d,                               // +0015  pop ebp
	0xc3,                               // +0016  ret
};

struct CExtract_CCurrencyPack_m_nAmount : public IExtract<int *>
{
	CExtract_CCurrencyPack_m_nAmount() : IExtract<int *>(sizeof(s_Buf_CCurrencyPack_m_nAmount)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CCurrencyPack_m_nAmount);
		
		mask.SetRange(0x0f + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CCurrencyPack::SetAmount"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x000f + 2; }
};

#elif defined _WINDOWS

using CExtract_CCurrencyPack_m_nAmount = IExtractStub;

#endif

#if defined _LINUX

static constexpr uint8_t s_Buf_CTeamControlPointMaster_m_ControlPoints[] = {
	0x55,                                     // +0000  push ebp
	0x89, 0xe5,                               // +0001  mov ebp,esp
	0x56,                                     // +0003  push esi
	0x8b, 0x45, 0x08,                         // +0004  mov eax,[ebp+this]
	0x53,                                     // +0007  push ebx
	0x8b, 0x75, 0x0c,                         // +0008  mov esi,[ebp+0xc]
	0x0f, 0xb7, 0x98, 0x7a, 0x03, 0x00, 0x00, // +000B  movzx ebx,word ptr [eax+0xVVVVVVVV]
};

struct CExtract_CTeamControlPointMaster_m_ControlPoints : public IExtract<CUtlMap<int, CTeamControlPoint *> *>
{
	using T = CUtlMap<int, CTeamControlPoint *> *;
	
	CExtract_CTeamControlPointMaster_m_ControlPoints() : IExtract<T>(sizeof(s_Buf_CTeamControlPointMaster_m_ControlPoints)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTeamControlPointMaster_m_ControlPoints);
		
		mask.SetRange(0x0b + 3, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTeamControlPointMaster::PointLastContestedAt"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x000b + 3; }
	virtual T AdjustValue(T val) const override        { return reinterpret_cast<T>((uintptr_t)val - 0x12); }
};

#elif defined _WINDOWS

using CExtract_CTeamControlPointMaster_m_ControlPoints = IExtractStub;

#endif

StaticFuncThunk<CTFDroppedWeapon *, CTFPlayer *, const Vector &, const QAngle &, const char *, const CEconItemView *> CTFDroppedWeapon::ft_Create("CTFDroppedWeapon::Create");

MemberFuncThunk<CTFDroppedWeapon *, void, CTFPlayer *, CTFWeaponBase *, bool, bool> CTFDroppedWeapon::ft_InitDroppedWeapon("CTFDroppedWeapon::InitDroppedWeapon");

IMPL_SENDPROP(CEconItemView, CTFDroppedWeapon, m_Item,   CTFDroppedWeapon);
IMPL_SENDPROP(float, CTFDroppedWeapon, m_flChargeLevel, CTFDroppedWeapon);
IMPL_RELATIVE(int, CTFDroppedWeapon, m_nAmmo, m_flChargeLevel, 0x0c);

IMPL_SENDPROP(int,                CTFPowerupBottle, m_usNumCharges, CTFPowerupBottle);

IMPL_DATAMAP(bool,     CTFPowerup, m_bDisabled);
IMPL_DATAMAP(bool,     CTFPowerup, m_bAutoMaterialize);
IMPL_DATAMAP(string_t, CTFPowerup, m_iszModel);

MemberVFuncThunk<CTFPowerup *, float> CTFPowerup::vt_GetLifeTime(TypeName<CTFPowerup>(), "CTFPowerup::GetLifeTime");

MemberFuncThunk<CTFPowerup *, void, Vector &, CBaseCombatCharacter *, float, float> CTFPowerup::ft_DropSingleInstance("CTFPowerup::DropSingleInstance");


IMPL_DATAMAP(int, CSpellPickup, m_nTier);


IMPL_SENDPROP(CHandle<CBaseEntity>, CTFReviveMarker, m_hOwner,   CTFReviveMarker);
IMPL_SENDPROP(short,                CTFReviveMarker, m_nRevives, CTFReviveMarker);

StaticFuncThunk<CTFReviveMarker *, CTFPlayer *> CTFReviveMarker::ft_Create ("CTFReviveMarker::Create");

MemberVFuncThunk<const IHasGenericMeter *, bool>  IHasGenericMeter::vt_ShouldUpdateMeter    (TypeName<IHasGenericMeter>(), "IHasGenericMeter::ShouldUpdateMeter");
MemberVFuncThunk<const IHasGenericMeter *, float> IHasGenericMeter::vt_GetMeterMultiplier   (TypeName<IHasGenericMeter>(), "IHasGenericMeter::GetMeterMultiplier");
MemberVFuncThunk<      IHasGenericMeter *, void>  IHasGenericMeter::vt_OnResourceMeterFilled(TypeName<IHasGenericMeter>(), "IHasGenericMeter::OnResourceMeterFilled");
MemberVFuncThunk<const IHasGenericMeter *, float> IHasGenericMeter::vt_GetChargeInterval    (TypeName<IHasGenericMeter>(), "IHasGenericMeter::GetChargeInterval");


MemberVFuncThunk<CEconWearable *, void, CBaseEntity *> CEconWearable::vt_RemoveFrom(TypeName<CEconWearable>(), "CEconWearable::RemoveFrom");
MemberVFuncThunk<CEconWearable *, void, CBasePlayer *> CEconWearable::vt_UnEquip   (TypeName<CEconWearable>(), "CEconWearable::UnEquip");

IMPL_SENDPROP(CHandle<CBaseEntity>, CTFWearable, m_hWeaponAssociatedWith, CTFWearable);
IMPL_SENDPROP(bool, CTFWearable, m_bDisguiseWearable, CTFWearable);
IMPL_RELATIVE(bool, CTFWearable, m_bAlwaysAllow, m_bDisguiseWearable, -sizeof(uintptr_t) * 2);

MemberFuncThunk<CTFWearableDemoShield *, void, CTFPlayer *> CTFWearableDemoShield::ft_DoSpecialAction ( "CTFWearableDemoShield::DoSpecialAction");
MemberFuncThunk<CTFWearableDemoShield *, void, CTFPlayer *> CTFWearableDemoShield::ft_EndSpecialAction( "CTFWearableDemoShield::EndSpecialAction");

IMPL_SENDPROP(bool, CTFBotHintEngineerNest, m_bHasActiveTeleporter, CTFBotHintEngineerNest);

MemberFuncThunk<const CTFBotHintEngineerNest *, bool> CTFBotHintEngineerNest::ft_IsStaleNest      ("CTFBotHintEngineerNest::IsStaleNest");
MemberFuncThunk<      CTFBotHintEngineerNest *, void> CTFBotHintEngineerNest::ft_DetonateStaleNest("CTFBotHintEngineerNest::DetonateStaleNest");


GlobalThunk<CUtlVector<ITFBotHintEntityAutoList *>> ITFBotHintEntityAutoList::m_ITFBotHintEntityAutoListAutoList("ITFBotHintEntityAutoList::m_ITFBotHintEntityAutoListAutoList");


MemberVFuncThunk<const CTFItem *, int> CTFItem::vt_GetItemID(TypeName<CTFItem>(), "CTFItem::GetItemID");


IMPL_SENDPROP(bool, CCaptureFlag, m_bDisabled,   CCaptureFlag);
IMPL_SENDPROP(int,  CCaptureFlag, m_nFlagStatus, CCaptureFlag);


GlobalThunk<CUtlVector<ICaptureFlagAutoList *>> ICaptureFlagAutoList::m_ICaptureFlagAutoListAutoList("ICaptureFlagAutoList::m_ICaptureFlagAutoListAutoList");


MemberFuncThunk<const CUpgrades *, const char *, int> CUpgrades::ft_GetUpgradeAttributeName("CUpgrades::GetUpgradeAttributeName");
MemberFuncThunk<const CUpgrades *, void, CTFPlayer *, bool , bool > CUpgrades::ft_GrantOrRemoveAllUpgrades("CUpgrades::GrantOrRemoveAllUpgrades");
MemberFuncThunk<CUpgrades *, void, CTFPlayer *, int , int, bool, bool, bool > CUpgrades::ft_PlayerPurchasingUpgrade("CUpgrades::PlayerPurchasingUpgrade");
MemberFuncThunk<CUpgrades *, attrib_definition_index_t,  CTFPlayer*, CEconItemView *, int, int, bool, bool > CUpgrades::ft_ApplyUpgradeToItem("CUpgrades::ApplyUpgradeToItem");

GlobalThunk<CHandle<CUpgrades>> g_hUpgradeEntity("g_hUpgradeEntity");


IMPL_REL_BEFORE(CUtlStringList, CFilterTFBotHasTag, m_TagList, m_iszTags);
IMPL_DATAMAP   (string_t,       CFilterTFBotHasTag, m_iszTags);
IMPL_DATAMAP   (bool,           CFilterTFBotHasTag, m_bRequireAllTags);
IMPL_DATAMAP   (bool,           CFilterTFBotHasTag, m_bNegated);


IMPL_REL_BEFORE(bool, CCurrencyPack, m_bTouched,     m_bPulled);      // 20151007a
IMPL_REL_BEFORE(bool, CCurrencyPack, m_bPulled,      m_bDistributed); // 20151007a
IMPL_SENDPROP  (bool, CCurrencyPack, m_bDistributed, CCurrencyPack);
IMPL_EXTRACT   (int,  CCurrencyPack, m_nAmount, new CExtract_CCurrencyPack_m_nAmount());

MemberVFuncThunk<const CCurrencyPack *, bool> CCurrencyPack::vt_AffectedByRadiusCollection(TypeName<CCurrencyPack>(), "CCurrencyPack::AffectedByRadiusCollection");

MemberFuncThunk<CCurrencyPack *, void, CTFPlayer *> CCurrencyPack::ft_DistributedBy("CCurrencyPack::DistributedBy");


GlobalThunk<CUtlVector<ICurrencyPackAutoList *>> ICurrencyPackAutoList::m_ICurrencyPackAutoListAutoList("ICurrencyPackAutoList::m_ICurrencyPackAutoListAutoList");


IMPL_SENDPROP(bool, CCaptureZone, m_bDisabled, CCaptureZone);

MemberFuncThunk<CCaptureZone *, void, CBaseEntity *> CCaptureZone::ft_Capture("CCaptureZone::Capture");


GlobalThunk<CUtlVector<ICaptureZoneAutoList *>> ICaptureZoneAutoList::m_ICaptureZoneAutoListAutoList("ICaptureZoneAutoList::m_ICaptureZoneAutoListAutoList");


static StaticFuncThunk<bool, const CBaseEntity *, const Vector&, bool> ft_PointInRespawnRoom("PointInRespawnRoom");
bool PointInRespawnRoom(const CBaseEntity *ent, const Vector& vec, bool b1) { return ft_PointInRespawnRoom(ent, vec, b1); }


#if TOOLCHAIN_FIXES
IMPL_EXTRACT(CTeamControlPointMaster::ControlPointMap, CTeamControlPointMaster, m_ControlPoints, new CExtract_CTeamControlPointMaster_m_ControlPoints());
#endif

GlobalThunk<CUtlVector<CHandle<CTeamControlPointMaster>>> g_hControlPointMasters("g_hControlPointMasters");


IMPL_DATAMAP(bool,     CTFTeamSpawn, m_bDisabled);
IMPL_DATAMAP(int,      CTFTeamSpawn, m_nSpawnMode);
IMPL_DATAMAP(string_t, CTFTeamSpawn, m_iszControlPointName);
IMPL_DATAMAP(string_t, CTFTeamSpawn, m_iszRoundBlueSpawn);
IMPL_DATAMAP(string_t, CTFTeamSpawn, m_iszRoundRedSpawn);


GlobalThunk<CUtlVector<ITFFlameEntityAutoList *>> ITFFlameEntityAutoList::m_ITFFlameEntityAutoListAutoList("ITFFlameEntityAutoList::m_ITFFlameEntityAutoListAutoList");


IMPL_DATAMAP (bool, CTFPointWeaponMimic, m_bCrits);
IMPL_DATAMAP (float, CTFPointWeaponMimic, m_flSpreadAngle);
IMPL_DATAMAP (float, CTFPointWeaponMimic, m_flDamage);
IMPL_DATAMAP (float, CTFPointWeaponMimic, m_flSpeedMax);
IMPL_DATAMAP (float, CTFPointWeaponMimic, m_flSplashRadius);
IMPL_DATAMAP (string_t, CTFPointWeaponMimic, m_pzsFireParticles);
IMPL_DATAMAP (string_t, CTFPointWeaponMimic, m_pzsFireSound);
IMPL_DATAMAP (string_t, CTFPointWeaponMimic, m_pzsModelOverride);
IMPL_DATAMAP (int, CTFPointWeaponMimic, m_nWeaponType);
IMPL_RELATIVE(CUtlVector<CHandle<CTFGrenadePipebombProjectile>>, CTFPointWeaponMimic, m_Pipebombs, m_bCrits, 0x04);

MemberFuncThunk<const CTFPointWeaponMimic *, QAngle> CTFPointWeaponMimic::ft_GetFiringAngles("CTFPointWeaponMimic::GetFiringAngles");
MemberFuncThunk<CTFPointWeaponMimic *, void> CTFPointWeaponMimic::ft_Fire("CTFPointWeaponMimic::Fire");


IMPL_SENDPROP(int,      CMonsterResource, m_iBossHealthPercentageByte, CMonsterResource);
IMPL_SENDPROP(int,      CMonsterResource, m_iBossStunPercentageByte, CMonsterResource);
IMPL_SENDPROP(int,      CMonsterResource, m_iBossState, CMonsterResource);

GlobalThunk<CMonsterResource *> g_pMonsterResource("g_pMonsterResource");


IMPL_DATAMAP (float, CTriggerIgnite, m_flBurnDuration);

IMPL_DATAMAP (float[MAX_PLAYERS + 1], CTriggerCatapult, m_flRefireDelay);
