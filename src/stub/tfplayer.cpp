#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/entities.h"
#include "stub/strings.h"
#include "util/misc.h"


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

#elif defined _WINDOWS

using CExtract_CTFPlayerShared_m_pOuter   = IExtractStub;
using CExtract_CTFPlayer_m_bFeigningDeath = IExtractStub;

#endif


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
IMPL_EXTRACT (CTFPlayer *, CTFPlayerShared, m_pOuter,                  new CExtract_CTFPlayerShared_m_pOuter());

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


IMPL_SENDPROP(CTFPlayerShared,      CTFPlayer, m_Shared,              CTFPlayer);
IMPL_SENDPROP(float,                CTFPlayer, m_flMvMLastDamageTime, CTFPlayer);
IMPL_RELATIVE(CTFPlayerAnimState *, CTFPlayer, m_PlayerAnimState,     m_hItem, -0x18); // 20170116a
IMPL_EXTRACT (bool,                 CTFPlayer, m_bFeigningDeath,      new CExtract_CTFPlayer_m_bFeigningDeath());
IMPL_SENDPROP(CTFPlayerClass,       CTFPlayer, m_PlayerClass,         CTFPlayer);
IMPL_SENDPROP(CHandle<CTFItem>,     CTFPlayer, m_hItem,               CTFPlayer);
IMPL_SENDPROP(bool,                 CTFPlayer, m_bIsMiniBoss,         CTFPlayer);
IMPL_SENDPROP(int,                  CTFPlayer, m_nCurrency,           CTFPlayer);
IMPL_SENDPROP(CHandle<CBaseEntity> ,CTFPlayer, m_hGrapplingHookTarget,CTFPlayer);
MemberFuncThunk<      CTFPlayer *, void, int, bool                 > CTFPlayer::ft_ForceChangeTeam                  ("CTFPlayer::ForceChangeTeam");
MemberFuncThunk<      CTFPlayer *, void, CCommand&                 > CTFPlayer::ft_ClientCommand                    ("CTFPlayer::ClientCommand");
MemberFuncThunk<      CTFPlayer *, void, int, int                  > CTFPlayer::ft_StartBuildingObjectOfType        ("CTFPlayer::StartBuildingObjectOfType");
MemberFuncThunk<const CTFPlayer *, bool, ETFFlagType *, int        > CTFPlayer::ft_HasTheFlag                       ("CTFPlayer::HasTheFlag");
MemberFuncThunk<      CTFPlayer *, int, int                        > CTFPlayer::ft_GetAutoTeam                      ("CTFPlayer::GetAutoTeam");
MemberFuncThunk<      CTFPlayer *, float, CTFWeaponBase **         > CTFPlayer::ft_MedicGetChargeLevel              ("CTFPlayer::MedicGetChargeLevel");
MemberFuncThunk<      CTFPlayer *, float, bool                     > CTFPlayer::ft_TeamFortress_CalculateMaxSpeed   ("CTFPlayer::TeamFortress_CalculateMaxSpeed");
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
MemberFuncThunk<      CTFPlayer *, CBaseEntity *, int              > CTFPlayer::ft_GetEntityForLoadoutSlot            ("CTFPlayer::GetEntityForLoadoutSlot");

MemberFuncThunk<CTFPlayer *, CBaseEntity *, const char *, int, CEconItemView *, bool> CTFPlayer::vt_GiveNamedItem("CTFPlayer::GiveNamedItem");


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
	return rtti_cast<CTFWeaponBase *>(this->GetActiveWeapon());
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
	
	return TF_COND_INVALID;
}


StaticFuncThunk<int, CUtlVector<CTFPlayer *> *, int, bool, bool> ft_CollectPlayers_CTFPlayer("CollectPlayers<CTFPlayer>");
