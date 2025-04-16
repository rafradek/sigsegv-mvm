#include "stub/gamerules.h"


[[gnu::noinline]] bool IsNullptr(uintptr_t ptr)
{
	return (ptr == 0);
}


#if defined _LINUX

static constexpr uint8_t s_Buf_CTeamplayRoundBasedRules_SetTeamRespawnWaveTime[] = {
	0x8d, 0x1c, 0x90,                               // +0000  lea ebx,[eax+edx*4]
	0x0f, 0x2f, 0x83, 0x98, 0x02, 0x00, 0x00,       // +0003  comiss xmm0,dword ptr [ebx+m_flOriginalTeamRespawnWaveTime]
	0x76, 0x0d,                                     // +000A  jbe +0xXX
	0xf3, 0x0f, 0x10, 0x45, 0x10,                   // +000C  movss xmm0,dword ptr [ebp+0x10]
	0xf3, 0x0f, 0x11, 0x83, 0x98, 0x02, 0x00, 0x00, // +0011  movss dword ptr [ebx+m_flOriginalTeamRespawnWaveTime],xmm0
};

struct CExtract_CTeamplayRoundBasedRules_SetTeamRespawnWaveTime : public IExtract<float (*)[MAX_TEAMS]>
{
	using T = float (*)[MAX_TEAMS];
	
	CExtract_CTeamplayRoundBasedRules_SetTeamRespawnWaveTime() : IExtract<T>(sizeof(s_Buf_CTeamplayRoundBasedRules_SetTeamRespawnWaveTime)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTeamplayRoundBasedRules_SetTeamRespawnWaveTime);
		
		mask.SetRange(0x03 + 3, 2, 0x00);
		mask.SetRange(0x0a + 1, 1, 0x00);
		mask.SetRange(0x11 + 4, 2, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTeamplayRoundBasedRules::SetTeamRespawnWaveTime"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0030; } // @ 0x0016
	virtual uint32_t GetExtractOffset() const override { return 0x0003 + 3; } // also: 0x0011 + 4
	
	/* we have two places where our desired offset pops up in the snippet, so ensure that they match */
	virtual bool Validate(const uint8_t *ptr) const override
	{
		uint32_t off1 = *(uint32_t *)(ptr + 0x03 + 3);
		uint32_t off2 = *(uint32_t *)(ptr + 0x11 + 4);
		
		return (off1 == off2);
	}
};

#elif defined _WINDOWS

using CExtract_CTeamplayRoundBasedRules_SetTeamRespawnWaveTime = IExtractStub;

#endif


GlobalThunk<CGameRulesProxy *> CGameRulesProxy::s_pGameRulesProxy("CGameRulesProxy::s_pGameRulesProxy");


MemberVFuncThunk<const CGameRules *, const CViewVectors *> CGameRules::vt_GetViewVectors(TypeName<CGameRules>(), "CGameRules::GetViewVectors");
MemberVFuncThunk<      CGameRules *, bool, int, int>       CGameRules::vt_ShouldCollide (TypeName<CGameRules>(), "CGameRules::ShouldCollide");
MemberVFuncThunk<      CGameRules *, bool, CBasePlayer *>  CGameRules::vt_IsConnectedUserInfoChangeAllowed(TypeName<CGameRules>(), "CGameRules::IsConnectedUserInfoChangeAllowed");


MemberVFuncThunk<CMultiplayRules *, VoiceCommandMenuItem_t *, CBaseMultiplayerPlayer *, int, int> CMultiplayRules::vt_VoiceCommand(TypeName<CMultiplayRules>(), "CMultiplayRules::VoiceCommand");


MemberVFuncThunk<CTeamplayRules *, void, int, int, bool, bool, bool, bool> CTeamplayRules::vt_SetWinningTeam(TypeName<CTeamplayRules>(), "CTeamplayRules::SetWinningTeam");
MemberVFuncThunk<CTeamplayRules *, void, int, bool, bool>                  CTeamplayRules::vt_SetStalemate  (TypeName<CTeamplayRules>(), "CTeamplayRules::SetStalemate");


IMPL_SENDPROP(gamerules_roundstate_t, CTeamplayRoundBasedRules, m_iRoundState,  CTeamplayRoundBasedRulesProxy);
IMPL_SENDPROP(int,                    CTeamplayRoundBasedRules, m_iWinningTeam, CTeamplayRoundBasedRulesProxy);
IMPL_SENDPROP(bool[MAX_PLAYERS],               CTeamplayRoundBasedRules, m_bPlayerReady, CTeamplayRoundBasedRulesProxy);
IMPL_SENDPROP(bool[MAX_PLAYERS],               CTeamplayRoundBasedRules, m_bTeamReady,   CTeamplayRoundBasedRulesProxy);
IMPL_SENDPROP(bool,                   CTeamplayRoundBasedRules, m_bInSetup,     CTeamplayRoundBasedRulesProxy);

#ifdef SE_IS_TF2
MemberFuncThunk<CTeamplayRoundBasedRules *, void, int, const char *, int, CBasePlayer *> CTeamplayRoundBasedRules::ft_BroadcastSound("CTeamplayRoundBasedRules::BroadcastSound");
#else
MemberFuncThunk<CTeamplayRoundBasedRules *, void, int, const char *, int> CTeamplayRoundBasedRules::ft_BroadcastSound("CTeamplayRoundBasedRules::BroadcastSound");
#endif
MemberFuncThunk<CTeamplayRoundBasedRules *, float, CBasePlayer *>         CTeamplayRoundBasedRules::ft_GetMinTimeWhenPlayerMaySpawn("CTeamplayRoundBasedRules::GetMinTimeWhenPlayerMaySpawn");
MemberFuncThunk<CTeamplayRoundBasedRules *, void, gamerules_roundstate_t> CTeamplayRoundBasedRules::ft_State_Transition            ("CTeamplayRoundBasedRules::State_Transition");
MemberFuncThunk<CTeamplayRoundBasedRules *, void, bool>                   CTeamplayRoundBasedRules::ft_SetForceMapReset            ("CTeamplayRoundBasedRules::SetForceMapReset");
MemberFuncThunk<CTeamplayRoundBasedRules *, void, int, float>             CTeamplayRoundBasedRules::ft_SetTeamRespawnWaveTime      ("CTeamplayRoundBasedRules::SetTeamRespawnWaveTime");
MemberVFuncThunk<CTeamplayRoundBasedRules *, float, int, CBasePlayer *>   CTeamplayRoundBasedRules::vt_GetNextRespawnWave(TypeName<CTeamplayRoundBasedRules>(), "CTeamplayRoundBasedRules::GetNextRespawnWave");


#ifdef SE_IS_TF2
IMPL_SENDPROP(bool,           CTFGameRules, m_bPlayingMedieval,      CTFGameRulesProxy);
IMPL_SENDPROP(bool,           CTFGameRules, m_bPowerupMode,          CTFGameRulesProxy);
IMPL_SENDPROP(bool,           CTFGameRules, m_bPlayingMannVsMachine, CTFGameRulesProxy);
IMPL_SENDPROP(char[MAX_PATH], CTFGameRules, m_pszCustomUpgradesFile, CTFGameRulesProxy);
IMPL_SENDPROP(float,           CTFGameRules, m_flRestartRoundTime, CTFGameRulesProxy);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],         CTFGameRules, m_ePlayerWantsRematch, CTFGameRulesProxy);

MemberFuncThunk<CTFGameRules *, bool, CTFPlayer *, int, unsigned short, CMannVsMachineUpgrades *> CTFGameRules::ft_CanUpgradeWithAttrib               ("CTFGameRules::CanUpgradeWithAttrib");
MemberFuncThunk<CTFGameRules *, int, CMannVsMachineUpgrades *, int, int, CTFPlayer *>             CTFGameRules::ft_GetCostForUpgrade                  ("CTFGameRules::GetCostForUpgrade");
MemberFuncThunk<CTFGameRules *, int, int>                                                         CTFGameRules::ft_GetUpgradeTier                     ("CTFGameRules::GetUpgradeTier");
MemberFuncThunk<CTFGameRules *, bool, CTFPlayer *, int, int>                                      CTFGameRules::ft_IsUpgradeTierEnabled               ("CTFGameRules::IsUpgradeTierEnabled");
MemberFuncThunk<CTFGameRules *, void, CTFPlayer *, bool>                                          CTFGameRules::ft_PlayerReadyStatus_UpdatePlayerState("CTFGameRules::PlayerReadyStatus_UpdatePlayerState");
MemberFuncThunk<CTFGameRules *, void, int, CTFPlayer *, bool, bool, bool>                         CTFGameRules::ft_DistributeCurrencyAmount           ("CTFGameRules::DistributeCurrencyAmount");
MemberFuncThunk<CTFGameRules *, void, inputdata_t&>                                               CTFGameRules::ft_SetCustomUpgradesFile              ("CTFGameRules::SetCustomUpgradesFile");
MemberFuncThunk<CTFGameRules *, void, CTFRadiusDamageInfo&>                                       CTFGameRules::ft_RadiusDamage                       ("CTFGameRules::RadiusDamage");
MemberFuncThunk<CTFGameRules *, bool, CBaseEntity *>                                              CTFGameRules::ft_IsPVEModeControlled                ("CTFGameRules::IsPVEModeControlled");
MemberFuncThunk<CTFGameRules *, void, const Vector &, int>                                        CTFGameRules::ft_DropSpellPickup                    ("CTFGameRules::DropSpellPickup");
MemberFuncThunk<CTFGameRules *, bool, CTFPlayer *>                                                CTFGameRules::ft_CanPlayerUseRespec                 ("CTFGameRules::CanPlayerUseRespec");
MemberFuncThunk<CTFGameRules *, bool>                                                             CTFGameRules::ft_IsCompetitiveMode                  ("CTFGameRules::IsCompetitiveMode");
MemberFuncThunk<CTFGameRules *, CBasePlayer *, CBasePlayer *, CBasePlayer *, CBaseEntity *>       CTFGameRules::ft_GetAssister                        ("CTFGameRules::GetAssister");
MemberFuncThunk<CTFGameRules *, bool, CTakeDamageInfo &, CBaseEntity *, bool>                     CTFGameRules::ft_ApplyOnDamageModifyRules           ("CTFGameRules::ApplyOnDamageModifyRules");
MemberFuncThunk<CTFGameRules *, float, const CTakeDamageInfo &, CBaseEntity *, CTFGameRules::DamageModifyExtras_t &> CTFGameRules::ft_ApplyOnDamageAliveModifyRules ("CTFGameRules::ApplyOnDamageAliveModifyRules");

MemberVFuncThunk<CTFGameRules *, bool> CTFGameRules::vt_FlagsMayBeCapped(TypeName<CTFGameRules>(), "CTFGameRules::FlagsMayBeCapped");
#endif


GlobalThunk<CGameRules *> g_pGameRules("g_pGameRules");


const char *GetRoundStateName(gamerules_roundstate_t state)
{
	switch (state) {
	case GR_STATE_INIT:         return "INIT";
	case GR_STATE_PREGAME:      return "PREGAME";
	case GR_STATE_STARTGAME:    return "STARTGAME";
	case GR_STATE_PREROUND:     return "PREROUND";
	case GR_STATE_RND_RUNNING:  return "RND_RUNNING";
	case GR_STATE_TEAM_WIN:     return "TEAM_WIN";
	case GR_STATE_RESTART:      return "RESTART";
	case GR_STATE_STALEMATE:    return "STALEMATE";
	case GR_STATE_GAME_OVER:    return "GAME_OVER";
	case GR_STATE_BONUS:        return "BONUS";
	case GR_STATE_BETWEEN_RNDS: return "BETWEEN_RNDS";
	default:                    return "???";
	}
}
