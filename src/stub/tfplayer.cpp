#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/entities.h"
#include "stub/strings.h"
#include "mod/pop/popmgr_extensions.h"
#include "util/misc.h"
#include "mem/extract.h"
#include "util/iterate.h"

#if defined _LINUX

static constexpr uint8_t s_Buf_CTFPlayerShared_m_pOuter[] = {
	0x55,                               // +0000  push ebp
	0x89, 0xe5,                         // +0001  mov ebp,esp
	0x8b, 0x45, 0x08,                   // +0003  mov eax,[ebp+this]
	0x8b, 0x80, 0x00, 0x00, 0x00, 0x00, // +0006  mov eax,[eax+m_pOuter]
	0x89, 0x45, 0x08,                   // +000C  mov [ebp+this],eax
	0x5d,                               // +000F  pop ebp
	0xe9,                               // +0010  jmp CBaseCombatCharacter::GetActiveWeapon
};

struct CExtract_CTFPlayerShared_m_pOuter : public IExtract<CTFPlayer **>
{
	CExtract_CTFPlayerShared_m_pOuter() : IExtract<CTFPlayer **>(sizeof(s_Buf_CTFPlayerShared_m_pOuter)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFPlayerShared_m_pOuter);
		
		mask.SetRange(0x06 + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFPlayerShared::GetActiveTFWeapon"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x0006 + 2; }
};


static constexpr uint8_t s_Buf_CTFPlayer_m_bFeigningDeath[] = {
	0x8b, 0x45, 0x0c,                               // +0000  mov eax,[ebp+arg_dmginfo]
	0xc6, 0x83, 0x00, 0x00, 0x00, 0x00, 0x01,       // +0003  mov byte ptr [ebx+m_bFeigningDeath],1
	0x89, 0x1c, 0x24,                               // +000A  mov [esp],ebx
	0x89, 0x44, 0x24, 0x04,                         // +000D  mov [esp+4],eax
	0xe8, 0x00, 0x00, 0x00, 0x00,                   // +0011  call CTFPlayer::FeignDeath
	0xa1, 0x00, 0x00, 0x00, 0x00,                   // +0016  mov eax,ds:tf_feign_death_duration.m_pParent
	0xc7, 0x44, 0x24, 0x0c, 0x00, 0x00, 0x00, 0x00, // +001B  mov dword ptr [esp+0xc],nullptr
	0xf3, 0x0f, 0x10, 0x40, 0x2c,                   // +0023  movss xmm0,dword ptr [eax+m_fValue]
	0xc7, 0x44, 0x24, 0x04, 0x00, 0x00, 0x00, 0x00, // +0028  mov dword ptr [esp+4],TF_COND_FEIGN_DEATH
	0x89, 0x34, 0x24,                               // +0030  mov [esp],esi
	0xf3, 0x0f, 0x11, 0x44, 0x24, 0x08,             // +0033  movss dword ptr [esp+8],xmm0
	0xe8, 0x00, 0x00, 0x00, 0x00,                   // +0039  call CTFPlayerShared::AddCond
	0xc6, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00,       // +003E  mov byte ptr [ebx+m_bFeigningDeath],0
};

struct CExtract_CTFPlayer_m_bFeigningDeath : public IExtract<bool *>
{
	static constexpr ptrdiff_t off__ConVar__m_pParent = 0x001c;
	static constexpr ptrdiff_t off__ConVar__m_fValue  = 0x002c;
	
	CExtract_CTFPlayer_m_bFeigningDeath() : IExtract<bool *>(sizeof(s_Buf_CTFPlayer_m_bFeigningDeath)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFPlayer_m_bFeigningDeath);
		
		uintptr_t addr__tf_feign_death_duration = GetAddrOfConVar__tf_feign_death_duration();
		if (addr__tf_feign_death_duration == 0) return false;
		
		buf.SetDword(0x16 + 1, addr__tf_feign_death_duration);
		buf[0x23 + 4] = off__ConVar__m_fValue;
		buf.SetDword(0x28 + 4, TF_COND_FEIGN_DEATH);
		
		mask.SetRange(0x03 + 2, 4, 0x00);
		mask.SetRange(0x11 + 1, 4, 0x00);
		mask.SetRange(0x39 + 1, 4, 0x00);
		mask.SetRange(0x3e + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFPlayer::SpyDeadRingerDeath"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0120; }
	virtual uint32_t GetExtractOffset() const override { return 0x0003 + 2; }
	
	static uintptr_t GetAddrOfConVar__tf_feign_death_duration()
	{
		ConVarRef cvref("tf_feign_death_duration");
		if (!cvref.IsValid()) return 0;
		
		ConVar *cvar = static_cast<ConVar *>(cvref.GetLinkedConVar());
		return (uintptr_t)cvar + off__ConVar__m_pParent;
	}
};

static constexpr uint8_t s_Buf_CTFPlayerShared_m_ConditionData[] = {
	0x8B, 0x56, 0x08,
	0x8D, 0x04, 0x9B,
	0x8D, 0x04, 0x82,
	0xF3, 0x0F, 0x10, 0x40, 0x08,
};

struct CExtract_CTFPlayerShared_m_ConditionData : public IExtract<CUtlVector<condition_source_t>*>
{
	using T = CUtlVector<condition_source_t>*;

	CExtract_CTFPlayerShared_m_ConditionData() : IExtract<T>(sizeof(s_Buf_CTFPlayerShared_m_ConditionData)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFPlayerShared_m_ConditionData);
		
		mask.SetRange(0x00 + 2, 1, 0x00);
		mask.SetRange(0x09 + 4, 1, 0x00);
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFPlayerShared::GetConditionDuration"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0060; }
	virtual uint32_t GetExtractOffset() const override { return 0x0000 + 2; }

	virtual T AdjustValue(T val) const override        { return reinterpret_cast<T>((uintptr_t)val & 0x000000ff); }
};


#elif defined _WINDOWS

using CExtract_CTFPlayerShared_m_pOuter   = IExtractStub;
using CExtract_CTFPlayer_m_bFeigningDeath = IExtractStub;
using CExtract_CTFPlayerShared_m_ConditionData = IExtractStub;

#endif

IMPL_SENDPROP(int, CTFConditionList, _condition_bits, CTFPlayer);

MemberFuncThunk<CMultiplayerAnimState *, void> CMultiplayerAnimState::ft_OnNewModel("CMultiplayerAnimState::OnNewModel");


IMPL_SENDPROP(int,      CTFPlayerClassShared, m_iClass,         CTFPlayer);
IMPL_SENDPROP(string_t, CTFPlayerClassShared, m_iszClassIcon,   CTFPlayer);
IMPL_SENDPROP(string_t, CTFPlayerClassShared, m_iszCustomModel, CTFPlayer);
IMPL_SENDPROP(bool,     CTFPlayerClassShared, m_bUseClassAnimations, CTFPlayer);

MemberFuncThunk<CTFPlayerClassShared *, void, const char *, bool> CTFPlayerClassShared::ft_SetCustomModel("CTFPlayerClassShared::SetCustomModel");


IMPL_SENDPROP(float,       CTFPlayerShared, m_flCloakMeter,            CTFPlayer);
IMPL_SENDPROP(float,       CTFPlayerShared, m_flEnergyDrinkMeter,      CTFPlayer);
IMPL_SENDPROP(float,       CTFPlayerShared, m_flHypeMeter,             CTFPlayer);
IMPL_SENDPROP(float,       CTFPlayerShared, m_flChargeMeter,           CTFPlayer);
IMPL_SENDPROP(float,       CTFPlayerShared, m_flRageMeter,             CTFPlayer);
IMPL_SENDPROP(bool,        CTFPlayerShared, m_bRageDraining,           CTFPlayer);
IMPL_SENDPROP(int,         CTFPlayerShared, m_iCritMult,               CTFPlayer);
IMPL_SENDPROP(bool,        CTFPlayerShared, m_bInUpgradeZone,          CTFPlayer);
IMPL_SENDPROP(float,       CTFPlayerShared, m_flStealthNoAttackExpire, CTFPlayer);
IMPL_SENDPROP(int,         CTFPlayerShared, m_nPlayerState,            CTFPlayer);
IMPL_SENDPROP(int,         CTFPlayerShared, m_iAirDash,                CTFPlayer);
IMPL_SENDPROP(int,         CTFPlayerShared, m_iDesiredPlayerClass,     CTFPlayer);
IMPL_SENDPROP(bool,        CTFPlayerShared, m_bHasPasstimeBall,        CTFPlayer);
IMPL_SENDPROP(uint,        CTFPlayerShared, m_nPlayerCond,             CTFPlayer);
IMPL_SENDPROP(CTFConditionList,     CTFPlayerShared, m_ConditionList,  CTFPlayer);
IMPL_SENDPROP(bool,        CTFPlayerShared, m_bLastDisguisedAsOwnTeam, CTFPlayer);
IMPL_SENDPROP(int,         CTFPlayerShared, m_nDisguiseTeam,           CTFPlayer);
IMPL_RELATIVE(CHandle<CTFWeaponBase>, CTFPlayerShared, m_hBurnWeapon, m_bLastDisguisedAsOwnTeam, +10);
IMPL_RELATIVE(float,       CTFPlayerShared, m_flFlameBurnTime, m_bLastDisguisedAsOwnTeam, +14);
IMPL_RELATIVE(float,       CTFPlayerShared, m_flFlameRemoveTime, m_bLastDisguisedAsOwnTeam, +18);
IMPL_EXTRACT (CTFPlayer *, CTFPlayerShared, m_pOuter,                  new CExtract_CTFPlayerShared_m_pOuter());
IMPL_EXTRACT (CUtlVector<condition_source_t>, CTFPlayerShared, m_ConditionData, new CExtract_CTFPlayerShared_m_ConditionData());

MemberFuncThunk<      CTFPlayerShared *, void, ETFCond, float, CBaseEntity * > CTFPlayerShared::ft_AddCond                   ("CTFPlayerShared::AddCond");
MemberFuncThunk<      CTFPlayerShared *, void, ETFCond, bool                 > CTFPlayerShared::ft_RemoveCond                ("CTFPlayerShared::RemoveCond");
MemberFuncThunk<const CTFPlayerShared *, bool, ETFCond                       > CTFPlayerShared::ft_InCond                    ("CTFPlayerShared::InCond");
MemberFuncThunk<const CTFPlayerShared *, bool                                > CTFPlayerShared::ft_IsInvulnerable            ("CTFPlayerShared::IsInvulnerable");
MemberFuncThunk<      CTFPlayerShared *, void, float, float, int, CTFPlayer *> CTFPlayerShared::ft_StunPlayer                ("CTFPlayerShared::StunPlayer");
MemberFuncThunk<      CTFPlayerShared *, void, CBitVec<192>&                 > CTFPlayerShared::ft_GetConditionsBits         ("CTFPlayerShared::GetConditionsBits");
MemberFuncThunk<      CTFPlayerShared *, float, ETFCond                      > CTFPlayerShared::ft_GetConditionDuration      ("CTFPlayerShared::GetConditionDuration");
MemberFuncThunk<      CTFPlayerShared *, CBaseEntity *, ETFCond              > CTFPlayerShared::ft_GetConditionProvider      ("CTFPlayerShared::GetConditionProvider");
MemberFuncThunk<const CTFPlayerShared *, int                                 > CTFPlayerShared::ft_GetDisguiseTeam           ("CTFPlayerShared::GetDisguiseTeam");
MemberFuncThunk<const CTFPlayerShared *, bool                                > CTFPlayerShared::ft_IsStealthed               ("CTFPlayerShared::IsStealthed");
MemberFuncThunk<const CTFPlayerShared *, float                               > CTFPlayerShared::ft_GetPercentInvisible       ("CTFPlayerShared::GetPercentInvisible");
MemberFuncThunk<      CTFPlayerShared *, bool                                > CTFPlayerShared::ft_IsControlStunned          ("CTFPlayerShared::IsControlStunned");
MemberFuncThunk<const CTFPlayerShared *, bool                                > CTFPlayerShared::ft_IsLoserStateStunned       ("CTFPlayerShared::IsLoserStateStunned");
MemberFuncThunk<      CTFPlayerShared *, void                                > CTFPlayerShared::ft_SetDefaultItemChargeMeters("CTFPlayerShared::SetDefaultItemChargeMeters");
MemberFuncThunk<      CTFPlayerShared *, void, loadout_positions_t, float    > CTFPlayerShared::ft_SetItemChargeMeter        ("CTFPlayerShared::SetItemChargeMeter");
MemberFuncThunk<      CTFPlayerShared *, void, CTFPlayer *, CTFWeaponBase*, float   > CTFPlayerShared::ft_Burn 					 ("CTFPlayerShared::Burn");


IMPL_SENDPROP(CTFPlayerShared,      CTFPlayer, m_Shared,                 CTFPlayer);
IMPL_SENDPROP(float,                CTFPlayer, m_flMvMLastDamageTime,    CTFPlayer);
IMPL_SENDPROP(bool,                 CTFPlayer, m_iSpawnCounter,          CTFPlayer);
IMPL_RELATIVE(CTFPlayerAnimState *, CTFPlayer, m_PlayerAnimState,        m_hItem, -0x18); // 20170116a
//IMPL_EXTRACT (bool,                 CTFPlayer, m_bFeigningDeath,         new CExtract_CTFPlayer_m_bFeigningDeath());
IMPL_RELATIVE(bool,                 CTFPlayer, m_bFeigningDeath,         m_bArenaSpectator, -0x14);
IMPL_RELATIVE(float,                CTFPlayer, m_flTauntAttackTime, m_iSpawnCounter, -32);
IMPL_RELATIVE(float,                CTFPlayer, m_flTauntInhaleTime, m_iSpawnCounter, -28);
IMPL_RELATIVE(int,                  CTFPlayer, m_iTauntAttack, m_iSpawnCounter, -24);
IMPL_RELATIVE(int,                  CTFPlayer, m_iTauntAttackCount, m_iSpawnCounter, -20);
IMPL_RELATIVE(int,                  CTFPlayer, m_iTauntRPSResult, m_iSpawnCounter, -16);
IMPL_RELATIVE(int,                  CTFPlayer, m_iPreTauntWeaponSlot, m_iSpawnCounter, -12);
IMPL_RELATIVE(int,                  CTFPlayer, m_iPreTauntFOV, m_iSpawnCounter, -8);
IMPL_SENDPROP(bool,                 CTFPlayer, m_bArenaSpectator,        CTFPlayer);
IMPL_SENDPROP(CTFPlayerClass,       CTFPlayer, m_PlayerClass,            CTFPlayer);
IMPL_SENDPROP(CHandle<CTFItem>,     CTFPlayer, m_hItem,                  CTFPlayer);
IMPL_SENDPROP(bool,                 CTFPlayer, m_bIsMiniBoss,            CTFPlayer);
IMPL_SENDPROP(int,                  CTFPlayer, m_nCurrency,              CTFPlayer);
IMPL_SENDPROP(int,                  CTFPlayer, m_nBotSkill,              CTFPlayer);
IMPL_SENDPROP(CHandle<CBaseEntity> ,CTFPlayer, m_hGrapplingHookTarget,   CTFPlayer);
IMPL_SENDPROP(bool,                 CTFPlayer, m_bAllowMoveDuringTaunt,  CTFPlayer);
IMPL_SENDPROP(float,                CTFPlayer, m_flCurrentTauntMoveSpeed,CTFPlayer);
IMPL_SENDPROP(short,                CTFPlayer, m_iTauntItemDefIndex  ,   CTFPlayer);
IMPL_SENDPROP(bool,                 CTFPlayer, m_bForcedSkin         ,   CTFPlayer);
IMPL_SENDPROP(int,                  CTFPlayer, m_nForcedSkin         ,   CTFPlayer);
IMPL_SENDPROP(bool,                 CTFPlayer, m_bMatchSafeToLeave   ,   CTFPlayer);
IMPL_RELATIVE(int,                  CTFPlayer, m_nCanPurchaseUpgradesCount, m_bMatchSafeToLeave, +12);
void NetworkStateChanged_CTFPlayer_m_angEyeAngles(void *obj, void *var) { reinterpret_cast<CTFPlayer *>(obj)->NetworkStateChanged(var); } \
const size_t CTFPlayer::_adj_m_angEyeAngles = offsetof(CTFPlayer, m_angEyeAngles);
CProp_SendProp CTFPlayer::s_prop_m_angEyeAngles("CTFPlayer", "m_angEyeAngles[0]", "CTFPlayer", NetworkStateChanged_CTFPlayer_m_angEyeAngles);

MemberFuncThunk<      CTFPlayer *, void, int, bool                 > CTFPlayer::ft_ForceChangeTeam                  ("CTFPlayer::ForceChangeTeam");
MemberFuncThunk<      CTFPlayer *, void, CCommand&                 > CTFPlayer::ft_ClientCommand                    ("CTFPlayer::ClientCommand");
MemberFuncThunk<      CTFPlayer *, void, int, int                  > CTFPlayer::ft_StartBuildingObjectOfType        ("CTFPlayer::StartBuildingObjectOfType");
MemberFuncThunk<const CTFPlayer *, bool, ETFFlagType *, int        > CTFPlayer::ft_HasTheFlag                       ("CTFPlayer::HasTheFlag");
MemberFuncThunk<      CTFPlayer *, int, int                        > CTFPlayer::ft_GetAutoTeam                      ("CTFPlayer::GetAutoTeam");
MemberFuncThunk<      CTFPlayer *, float, CTFWeaponBase **         > CTFPlayer::ft_MedicGetChargeLevel              ("CTFPlayer::MedicGetChargeLevel");
MemberFuncThunk<      CTFPlayer *, float, bool                     > CTFPlayer::ft_TeamFortress_CalculateMaxSpeed   ("CTFPlayer::TeamFortress_CalculateMaxSpeed");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_TeamFortress_SetSpeed            ("CTFPlayer::TeamFortress_SetSpeed");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_UpdateModel                      ("CTFPlayer::UpdateModel");
MemberFuncThunk<const CTFPlayer *, CTFWeaponBase *, int            > CTFPlayer::ft_Weapon_OwnsThisID                ("CTFPlayer::Weapon_OwnsThisID");
MemberFuncThunk<      CTFPlayer *, CTFWeaponBase *, int            > CTFPlayer::ft_Weapon_GetWeaponByType           ("CTFPlayer::Weapon_GetWeaponByType");
MemberFuncThunk<      CTFPlayer *, CBaseObject *, int, int         > CTFPlayer::ft_GetObjectOfType                  ("CTFPlayer::GetObjectOfType");
MemberFuncThunk<      CTFPlayer *, int, int, int                   > CTFPlayer::ft_GetMaxAmmo                       ("CTFPlayer::GetMaxAmmo");
MemberFuncThunk<      CTFPlayer *, CTFWearable *, int              > CTFPlayer::ft_GetEquippedWearableForLoadoutSlot("CTFPlayer::GetEquippedWearableForLoadoutSlot");
MemberFuncThunk<      CTFPlayer *, void, const char *              > CTFPlayer::ft_HandleCommand_JoinTeam           ("CTFPlayer::HandleCommand_JoinTeam");
MemberFuncThunk<      CTFPlayer *, void, const char *              > CTFPlayer::ft_HandleCommand_JoinTeam_NoMenus   ("CTFPlayer::HandleCommand_JoinTeam_NoMenus");
MemberFuncThunk<      CTFPlayer *, void, const char *, bool        > CTFPlayer::ft_HandleCommand_JoinClass          ("CTFPlayer::HandleCommand_JoinClass");
MemberFuncThunk<      CTFPlayer *, void, const char *, float, float> CTFPlayer::ft_AddCustomAttribute               ("CTFPlayer::AddCustomAttribute");
MemberFuncThunk<      CTFPlayer *, void, const char *              > CTFPlayer::ft_RemoveCustomAttribute            ("CTFPlayer::RemoveCustomAttribute");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_RemoveAllCustomAttributes        ("CTFPlayer::RemoveAllCustomAttributes");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_ReapplyPlayerUpgrades            ("CTFPlayer::ReapplyPlayerUpgrades");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_UseActionSlotItemPressed            ("CTFPlayer::UseActionSlotItemPressed");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_UseActionSlotItemReleased            ("CTFPlayer::UseActionSlotItemReleased");
MemberFuncThunk<      CTFPlayer *, CAttributeManager *             > CTFPlayer::ft_GetAttributeManager              ("CTFPlayer::GetAttributeManager");
MemberFuncThunk<      CTFPlayer *, CAttributeList*                 > CTFPlayer::ft_GetAttributeList                    ("CTFPlayer::GetAttributeList");
MemberFuncThunk<      CTFPlayer *, CBaseEntity *, int              > CTFPlayer::ft_GetEntityForLoadoutSlot            ("CTFPlayer::GetEntityForLoadoutSlot");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_RemoveInvisibility               ("CTFPlayer::RemoveInvisibility");
MemberFuncThunk<      CTFPlayer *, void, PlayerAnimEvent_t, int    > CTFPlayer::ft_DoAnimationEvent            ("CTFPlayer::DoAnimationEvent");
MemberFuncThunk<      CTFPlayer *, void, const char *              > CTFPlayer::ft_PlaySpecificSequence        ("CTFPlayer::PlaySpecificSequence");
MemberFuncThunk<      CTFPlayer *, void, taunts_t, int             > CTFPlayer::ft_Taunt                       ("CTFPlayer::Taunt");
MemberFuncThunk<      CTFPlayer *, void, CEconItemView*            > CTFPlayer::ft_PlayTauntSceneFromItem      ("CTFPlayer::PlayTauntSceneFromItem");
MemberFuncThunk<      CTFPlayer *, CBaseObject *, int              > CTFPlayer::ft_GetObject                   ("CTFPlayer::GetObject");
MemberFuncThunk<      CTFPlayer *, int                             > CTFPlayer::ft_GetObjectCount              ("CTFPlayer::GetObjectCount");
MemberFuncThunk<      CTFPlayer *, void, int                       > CTFPlayer::ft_StateTransition             ("CTFPlayer::StateTransition");
MemberFuncThunk<      CTFPlayer *, void, int                       > CTFPlayer::ft_RemoveCurrency              ("CTFPlayer::RemoveCurrency");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_CheckInstantLoadoutRespawn  ("CTFPlayer::CheckInstantLoadoutRespawn");
MemberFuncThunk<      CTFPlayer *, void                            > CTFPlayer::ft_ForceRegenerateAndRespawn   ("CTFPlayer::ForceRegenerateAndRespawn");
MemberFuncThunk<      CTFPlayer *, void, Vector &, CTFPlayer *     > CTFPlayer::ft_ApplyGenericPushbackImpulse ("CTFPlayer::ApplyGenericPushbackImpulse");
MemberFuncThunk<const CTFPlayer *, bool							   > CTFPlayer::ft_CanAirDash 				   ("CTFPlayer::CanAirDash");
MemberFuncThunk<	  CTFPlayer *, void, bool					   > CTFPlayer::ft_Regenerate				   ("CTFPlayer::Regenerate");
MemberFuncThunk<      CTFPlayer *, void, TFPlayerClassData_t *	   > CTFPlayer::ft_ManageRegularWeapons        ("CTFPlayer::ManageRegularWeapons");
MemberFuncThunk<      CTFPlayer *, void, TFPlayerClassData_t *	   > CTFPlayer::ft_ManageBuilderWeapons        ("CTFPlayer::ManageBuilderWeapons");
MemberFuncThunk<	  CTFPlayer *, void                      	   > CTFPlayer::ft_GiveDefaultItems            ("CTFPlayer::GiveDefaultItems");
MemberFuncThunk<	  CTFPlayer *, bool                      	   > CTFPlayer::ft_TryToPickupBuilding         ("CTFPlayer::TryToPickupBuilding");
MemberFuncThunk<	  CTFPlayer *, void, int, int                  > CTFPlayer::ft_DetonateObjectOfType        ("CTFPlayer::DetonateObjectOfType");
MemberFuncThunk<	  CTFPlayer *, void, CUtlVector<CTFWeaponBase *> &> CTFPlayer::ft_GetPassiveWeapons        ("CTFPlayer::GetPassiveWeapons");
MemberFuncThunk<	  CTFPlayer *, bool                            > CTFPlayer::ft_IsReadyToPlay               ("CTFPlayer::IsReadyToPlay");
MemberFuncThunk<      CTFPlayer *, void, const Vector &            > CTFPlayer::ft_ApplyAbsVelocityImpulse     ("CTFPlayer::ApplyAbsVelocityImpulse");

MemberFuncThunk<      CTFPlayer *, float, const char *, float, void *, IRecipientFilter *> CTFPlayer::ft_PlayScene("CTFPlayer::PlayScene");


MemberFuncThunk<CTFPlayer *, CBaseEntity *, const char *, int, CEconItemView *, bool> CTFPlayer::vt_GiveNamedItem("CTFPlayer::GiveNamedItem");

StaticFuncThunk<void> CTFPlayer::ft_PrecacheMVM("CTFPlayer::PrecacheMvM");

StaticFuncThunk<CEconItemView *, CTFPlayer *, int, CEconEntity **> CTFPlayerSharedUtils::ft_GetEconItemViewByLoadoutSlot("CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot");


IMPL_SENDPROP(float, CTFRagdoll, m_flHeadScale,  CTFRagdoll);
IMPL_SENDPROP(float, CTFRagdoll, m_flTorsoScale, CTFRagdoll);
IMPL_SENDPROP(float, CTFRagdoll, m_flHandScale,  CTFRagdoll);


bool CTFPlayer::IsPlayerClass(int iClass) const
{
	const CTFPlayerClass *pClass = this->GetPlayerClass();
	if (pClass == nullptr) return false;
	
	return pClass->IsClass(iClass);
}


CTFWeaponBase *CTFPlayer::GetActiveTFWeapon() const
{
	// The game implementation just statically casts the weapon so
	// return rtti_cast<CTFWeaponBase *>(this->GetActiveWeapon());
	return static_cast<CTFWeaponBase *>(this->GetActiveWeapon());
}


int GetNumberOfTFConds()
{
	static int iNumTFConds =
	[]{
		ConColorMsg(Color(0xff, 0x00, 0xff, 0xff), "GetNumberOfTFConds: in lambda\n");
		
		const SegInfo& info_seg_server_rodata = LibMgr::GetInfo(Library::SERVER).GetSeg(Segment::RODATA);
		
		constexpr char prefix[] = "TF_COND_";
		
		for (int i = 0; i < 256; ++i) {
			const char *str = g_aConditionNames[i];
			
			if (str == nullptr || !info_seg_server_rodata.ContainsAddr(str, 1) || strncmp(str, prefix, strlen(prefix)) != 0) {
				return i;
			}
		}
		
		assert(false);
		return 0;
	}();
	
	return iNumTFConds;
}


bool IsValidTFConditionNumber(int num)
{
	return (num >= 0 && num < GetNumberOfTFConds());
}

ETFCond ClampTFConditionNumber(int num)
{
	return static_cast<ETFCond>(Clamp(num, 0, GetNumberOfTFConds() - 1));
}


const char *GetTFConditionName(ETFCond cond)
{
	int num = static_cast<int>(cond);
	
	if (!IsValidTFConditionNumber(num)) {
		return nullptr;
	}
	
	return g_aConditionNames[num];
}

ETFCond GetTFConditionFromName(const char *name)
{
	int cond_count = GetNumberOfTFConds();
	for (int i = 0; i < cond_count; ++i) {
		if (FStrEq(g_aConditionNames[i], name)) {
			return static_cast<ETFCond>(i);
		}
	}
	if (FStrEq("TF_COND_REPROGRAMMED_NEUTRAL", name))
		return TF_COND_HALLOWEEN_HELL_HEAL;
	
	return TF_COND_INVALID;
}


StaticFuncThunk<int, CUtlVector<CTFPlayer *> *, int, bool, bool> ft_CollectPlayers_CTFPlayer("CollectPlayers<CTFPlayer>");
StaticFuncThunk<void, CBasePlayer *, int, int> ft_TE_PlayerAnimEvent("TE_PlayerAnimEvent");
StaticFuncThunk<TFPlayerClassData_t *, uint> ft_GetPlayerClassData("GetPlayerClassData");

CEconItemView *CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot(CTFPlayer *player, int slot, CEconEntity **ent)
{ 
	//also search invalid class weapons
	
	/*int classindex = player->GetPlayerClass()->GetClassIndex();
	for(int i = 0; i < MAX_WEAPONS; i++) {
		CEconEntity *weapon = player->GetWeapon(i);
		if (weapon == nullptr)
			continue;

		CEconItemView *view= weapon->GetItem();
		if (view == nullptr)
			continue;

		int weapon_slot = view->GetStaticData()->GetLoadoutSlot(classindex);
		if (weapon_slot == -1)
		{
			weapon_slot = view->GetStaticData()->GetLoadoutSlot(TF_CLASS_UNDEFINED);
		}
		if (weapon_slot == slot)
		{
			if (ent)
			{
				*ent = weapon;
			}
			return view;
		}
	}*/
	return ft_GetEconItemViewByLoadoutSlot(player, slot, ent); 
}

CEconEntity *CreateItemByName(CTFPlayer *player, const char *item_name)
{
	auto item_def = GetItemSchema()->GetItemDefinitionByName(item_name);
	if (item_def != nullptr) {
		const char *classname = TranslateWeaponEntForClass_improved(item_def->GetItemClass(), player->GetPlayerClass()->GetClassIndex());
		CEconEntity *entity = static_cast<CEconEntity *>(ItemGeneration()->SpawnItem(item_def->m_iItemDefIndex, player->WorldSpaceCenter(), vec3_angle, 1, 6, classname));

		if (entity != nullptr) {
			Mod::Pop::PopMgr_Extensions::AddCustomWeaponAttributes(item_name, entity->GetItem());
			DispatchSpawn(entity);
		}
		return entity;
	}
	return nullptr;
}

CEconEntity *GiveItemByName(CTFPlayer *player, const char *item_name, bool no_remove, bool force_give)
{
	auto entity = CreateItemByName(player, item_name);
	if (entity != nullptr && !GiveItemToPlayer(player, entity, no_remove, force_give, item_name)) {
		entity->Remove();
		entity = nullptr;
	}
	return entity;
}

bool GiveItemToPlayer(CTFPlayer *player, CEconEntity *entity, bool no_remove, bool force_give, const char *item_name)
{
	if (entity == nullptr)
		return false;

	CEconItemView *view = entity->GetItem();
	int slot = view->GetStaticData()->GetLoadoutSlot(player->GetPlayerClass()->GetClassIndex());
	if (slot == -1) {
		if (!force_give) {
			return false;
		}
		slot = view->GetStaticData()->GetLoadoutSlot(TF_CLASS_UNDEFINED);
	}

	if (!no_remove) {
		

		if (IsLoadoutSlot_Cosmetic(static_cast<loadout_positions_t>(slot))) {
			/* equip-region-conflict-based old item removal */
			
			unsigned int mask1 = view->GetStaticData()->GetEquipRegionMask();
			
			for (int i = player->GetNumWearables() - 1; i >= 0; --i) {
				CEconWearable *wearable = player->GetWearable(i);
				if (wearable == nullptr) continue;
				
				unsigned int mask2 = wearable->GetAttributeContainer()->GetItem()->GetStaticData()->GetEquipRegionMask();
				
				if ((mask1 & mask2) != 0) {
					player->RemoveWearable(wearable);
				}
			}
		} else {
			/* slot-based old item removal */
			
			CEconEntity *old_econ_entity = nullptr;
			(void)CTFPlayerSharedUtils::GetEconItemViewByLoadoutSlot(player, slot, &old_econ_entity);
			
			if (old_econ_entity != nullptr) {
				// Prevent replacing extra loadout item
				int extraLoadoutItem = 0;
				int forced = 0;
				CALL_ATTRIB_HOOK_INT_ON_OTHER(entity, forced, is_forced_item);
				CALL_ATTRIB_HOOK_INT_ON_OTHER(old_econ_entity, extraLoadoutItem, is_extra_loadout_item);
				if (extraLoadoutItem != 0 && forced == 0) {
					return false;
				}

				if (old_econ_entity->IsBaseCombatWeapon()) {
					auto old_weapon = ToBaseCombatWeapon(old_econ_entity);
					
					player->Weapon_Detach(old_weapon);
					old_weapon->Remove();
				} else if (old_econ_entity->IsWearable()) {
					auto old_wearable = rtti_cast<CEconWearable *>(old_econ_entity);
					
					player->RemoveWearable(old_wearable);
				} else {
					old_econ_entity->Remove();
					
				}
			} else {
			//	Msg("No old entity in slot %d\n", slot);
			}
		}
	}
	/* make the model visible for other players */
	entity->Validate();
	
	entity->GiveTo(player);
	return true;
}

CEconEntity *CTFPlayer::GetEconEntityByName(const char *name)
{
	CEconItemDefinition *item_def = GetItemSchema()->GetItemDefinitionByName(name);
	if (item_def == nullptr) return nullptr;

	CEconEntity *value = nullptr;
	ForEachTFPlayerEconEntity(this, [&](CEconEntity *entity) {
		CEconItemView *item_view = entity->GetItem();
		//DevMsg("item name %s\n", item_view->GetItemDefinition()->GetItemName());
		if (item_view->GetItemDefIndex() == item_def->m_iItemDefIndex) {
			value = entity;
			return false;
		}
		return true;
	});

	return value;
}

CEconEntity *CTFPlayer::GetEconEntityById(int id)
{
	CEconEntity *value = nullptr;
	ForEachTFPlayerEconEntity(this, [&](CEconEntity *entity) {
		CEconItemView *item_view = entity->GetItem();
		if (item_view->GetItemDefIndex() == id) {
			value = entity;
			return false;
		}
		return true;
	});

	return value;
}

void CTFPlayer::GiveDefaultItemsNoAmmo()
{
	float ammoFraction[TF_AMMO_COUNT];

	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		if (GetMaxAmmo(iAmmo) != 0) {
			ammoFraction[iAmmo] = ((float) GetAmmoCount(iAmmo) / (float) GetMaxAmmo(iAmmo));
		}
	}
	CTFPlayer::GiveDefaultItems();

	for ( int iAmmo = 0; iAmmo < TF_AMMO_COUNT; ++iAmmo )
	{
		if (GetMaxAmmo(iAmmo) != 0) {
			SetAmmoCount( ammoFraction[iAmmo] * GetMaxAmmo(iAmmo), iAmmo );
		}
	}
}
