#ifndef _INCLUDE_SIGSEGV_STUB_GAMERULES_H_
#define _INCLUDE_SIGSEGV_STUB_GAMERULES_H_


#include "prop.h"
#include "link/link.h"
#include "stub/baseentity.h"


enum gamerules_roundstate_t
{
	// initialize the game, create teams
	GR_STATE_INIT = 0,

	//Before players have joined the game. Periodically checks to see if enough players are ready
	//to start a game. Also reverts to this when there are no active players
	GR_STATE_PREGAME,

	//The game is about to start, wait a bit and spawn everyone
	GR_STATE_STARTGAME,

	//All players are respawned, frozen in place
	GR_STATE_PREROUND,

	//Round is on, playing normally
	GR_STATE_RND_RUNNING,

	//Someone has won the round
	GR_STATE_TEAM_WIN,

	//Noone has won, manually restart the game, reset scores
	GR_STATE_RESTART,

	//Noone has won, restart the game
	GR_STATE_STALEMATE,

	//Game is over, showing the scoreboard etc
	GR_STATE_GAME_OVER,

	//Game is in a bonus state, transitioned to after a round ends
	GR_STATE_BONUS,

	//Game is awaiting the next wave/round of a multi round experience
	GR_STATE_BETWEEN_RNDS,

	GR_NUM_ROUND_STATES
};


enum
{
	WINREASON_NONE                    =  0,
	WINREASON_ALL_POINTS_CAPTURED     =  1,
	WINREASON_OPPONENTS_DEAD          =  2,
	WINREASON_FLAG_CAPTURE_LIMIT      =  3,
	WINREASON_DEFEND_UNTIL_TIME_LIMIT =  4,
	WINREASON_STALEMATE               =  5,
	WINREASON_TIMELIMIT               =  6,
	WINREASON_WINLIMIT                =  7,
	WINREASON_WINDIFFLIMIT            =  8,
	WINREASON_RD_REACTOR_CAPTURED     =  9,
	WINREASON_RD_CORES_COLLECTED      = 10,
	WINREASON_RD_REACTOR_RETURNED     = 11,
};


class CBaseMultiplayerPlayer;
class CTFPlayer;
struct VoiceCommandMenuItem_t;
struct CMannVsMachineUpgrades;

struct CTFRadiusDamageInfo
{
	CTakeDamageInfo *m_DmgInfo;   // +0x00
	Vector m_vecOrigin;           // +0x04: blast origin
	float m_flRadius;             // +0x10: blast radius
	CBaseEntity *m_pEntityIgnore = nullptr; // +0x14
	float m_unknown_18 = 0.0f;           // +0x18, default 0.0f, ?
	float m_unknown_1c = 1.0f;           // +0x1c, default 1.0f, scales damage force or something
	CBaseEntity *target = nullptr;             // +0x20, default 0, ?
	float m_flFalloff = 0.5f;            // +0x24: how much the damage is decreased by at the edge of the blast
};


class CGameRulesProxy : public CBaseEntity
{
public:
	static void NotifyNetworkStateChanged();
	
private:
	static GlobalThunk<CGameRulesProxy *> s_pGameRulesProxy;
};

class CTeamplayRoundBasedRulesProxy : public CGameRulesProxy {};
class CTFGameRulesProxy : public CTeamplayRoundBasedRulesProxy {};


/* g_pGameRules is sometimes nullptr (e.g. at engine shutdown time), so we have
 * to be able to gracefully handle those cases, ideally without crashing;
 * annoyingly, 'this' is guaranteed to be non-nullptr in well-formed C++ code,
 * so we have to do stupid workarounds like this to avoid compilers optimizing
 * out the check we do */
[[gnu::noinline]] bool IsNullptr(uintptr_t ptr);
#define IF_NULL if (IsNullptr((uintptr_t)static_cast<const void *>(this)))
#define NULL_RET(...) IF_NULL { return __VA_ARGS__; }


class CGameRules
{
public:
	void NetworkStateChanged(void *pVar) { NULL_RET(); CGameRulesProxy::NotifyNetworkStateChanged(); }
	void NetworkStateChanged()           { NULL_RET(); CGameRulesProxy::NotifyNetworkStateChanged(); }
	
	const CViewVectors *GetViewVectors() const;
	bool ShouldCollide(int collisionGroup0, int collisionGroup1) { NULL_RET(true); return vt_ShouldCollide (this, collisionGroup0, collisionGroup1); }
	
private:
	static MemberVFuncThunk<const CGameRules *, const CViewVectors *> vt_GetViewVectors;
	static MemberVFuncThunk<      CGameRules *, bool, int, int>       vt_ShouldCollide;
};

class CMultiplayRules : public CGameRules
{
public:
	VoiceCommandMenuItem_t *VoiceCommand(CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem) { NULL_RET(nullptr); return vt_VoiceCommand(this, pPlayer, iMenu, iItem); }
	
private:
	static MemberVFuncThunk<CMultiplayRules *, VoiceCommandMenuItem_t *, CBaseMultiplayerPlayer *, int, int> vt_VoiceCommand;
};

class CTeamplayRules : public CMultiplayRules
{
public:
	void SetWinningTeam(int team, int iWinReason, bool bForceMapReset = true, bool bSwitchTeams = false, bool bDontAddScore = false, bool bFinal = false) { NULL_RET(); vt_SetWinningTeam(this, team, iWinReason, bForceMapReset, bSwitchTeams, bDontAddScore, bFinal); }
	void SetStalemate(int iReason, bool bForceMapReset = true, bool bSwitchTeams = false)                                                                 { NULL_RET(); vt_SetStalemate  (this, iReason, bForceMapReset, bSwitchTeams); }
	
private:
	static MemberVFuncThunk<CTeamplayRules *, void, int, int, bool, bool, bool, bool> vt_SetWinningTeam;
	static MemberVFuncThunk<CTeamplayRules *, void, int, bool, bool>                  vt_SetStalemate;
};

class CTeamplayRoundBasedRules : public CTeamplayRules
{
public:
	gamerules_roundstate_t State_Get() { NULL_RET(GR_STATE_INIT); return this->m_iRoundState; }
	int GetWinningTeam()               { NULL_RET(TEAM_INVALID ); return this->m_iWinningTeam; }
	bool IsPlayerReady(int iIndex)     { NULL_RET(false        ); return this->m_bPlayerReady[iIndex]; }
	bool IsTeamReady(int iIndex)       { NULL_RET(false        ); return this->m_bTeamReady[iIndex]; }
	void SetTeamReady(int iIndex, bool ready)       { NULL_RET( ); this->m_bTeamReady.SetIndex(ready, iIndex); }
	bool InSetup()                     { NULL_RET(false        ); return this->m_bInSetup; }
	void SetInSetup(bool setup)        { NULL_RET( );             m_bInSetup = setup; }
	
	void BroadcastSound(int iTeam, const char *sound, int iAdditionalSoundFlags = 0, CBasePlayer *player = nullptr) { NULL_RET(    );        ft_BroadcastSound              (this, iTeam, sound, iAdditionalSoundFlags, player); }
	float GetMinTimeWhenPlayerMaySpawn(CBasePlayer *pPlayer)                         { NULL_RET(0.0f); return ft_GetMinTimeWhenPlayerMaySpawn(this, pPlayer); }
	void State_Transition(gamerules_roundstate_t newState)                           { NULL_RET(    );        ft_State_Transition            (this, newState); }
	void SetForceMapReset(bool reset)                                                { NULL_RET(    );        ft_SetForceMapReset            (this, reset); }
	void SetTeamRespawnWaveTime(int team, float time)                                { NULL_RET(    );        ft_SetTeamRespawnWaveTime      (this, team, time); }
	
	float GetNextRespawnWave(int iTeam, CBasePlayer *pPlayer) { NULL_RET(0.0f); return vt_GetNextRespawnWave(this, iTeam, pPlayer); }
	
	DECL_SENDPROP(bool[33],               m_bPlayerReady);
private:

	DECL_SENDPROP(gamerules_roundstate_t, m_iRoundState);
	DECL_SENDPROP(int,                    m_iWinningTeam);
	DECL_SENDPROP_RW(bool[33],            m_bTeamReady);
	DECL_SENDPROP(bool,                   m_bInSetup);
	
	static MemberFuncThunk<CTeamplayRoundBasedRules *, void, int, const char *, int, CBasePlayer *> ft_BroadcastSound;
	static MemberFuncThunk<CTeamplayRoundBasedRules *, float, CBasePlayer *>         ft_GetMinTimeWhenPlayerMaySpawn;
	static MemberFuncThunk<CTeamplayRoundBasedRules *, void, gamerules_roundstate_t> ft_State_Transition;
	static MemberFuncThunk<CTeamplayRoundBasedRules *, void, bool>                   ft_SetForceMapReset;
	static MemberFuncThunk<CTeamplayRoundBasedRules *, void, int, float>             ft_SetTeamRespawnWaveTime;
	static MemberVFuncThunk<CTeamplayRoundBasedRules *, float, int, CBasePlayer *> vt_GetNextRespawnWave;
};

class CTFGameRules : public CTeamplayRoundBasedRules
{
public:
	bool IsInMedievalMode() const                { NULL_RET(false); return this->m_bPlayingMedieval; }
	bool IsMannVsMachineMode() const             { NULL_RET(false); return this->m_bPlayingMannVsMachine; }
	const char *GetCustomUpgradesFile() const    { NULL_RET(""   ); return this->m_pszCustomUpgradesFile; }
	float GetRestartRoundTime()                  { NULL_RET(    0); return this->m_flRestartRoundTime; }
	bool IsCompetitive() const                { NULL_RET(false); return this->m_bPlayingMedieval; }
	void SetCustomUpgradesFile(const char *path);
	
	bool CanUpgradeWithAttrib(CTFPlayer *player, int slot, unsigned short attr, CMannVsMachineUpgrades *upgrade) { NULL_RET(false); return ft_CanUpgradeWithAttrib               (this, player, slot, attr, upgrade); }
	int GetCostForUpgrade(CMannVsMachineUpgrades *upgrade, int slot, int pclass, CTFPlayer *player)              { NULL_RET(    0); return ft_GetCostForUpgrade                  (this, upgrade, slot, pclass, player); }
	int GetUpgradeTier(int index)                                                                                { NULL_RET(    0); return ft_GetUpgradeTier                     (this, index); }
	bool IsUpgradeTierEnabled(CTFPlayer *player, int slot, int tier)                                             { NULL_RET(false); return ft_IsUpgradeTierEnabled               (this, player, slot, tier); }
	void PlayerReadyStatus_UpdatePlayerState(CTFPlayer *player, bool state)                                      { NULL_RET(     ); return ft_PlayerReadyStatus_UpdatePlayerState(this, player, state); }
	void DistributeCurrencyAmount(int amount, CTFPlayer *player, bool b1, bool b2, bool b3)                      { NULL_RET(     );        ft_DistributeCurrencyAmount           (this, amount, player, b1, b2, b3); }
	void SetCustomUpgradesFile(inputdata_t& inputdata)                                                           { NULL_RET(     );        ft_SetCustomUpgradesFile              (this, inputdata); }
	void RadiusDamage(CTFRadiusDamageInfo& radiusdamage)                                                         { NULL_RET(     );        ft_RadiusDamage                       (this, radiusdamage); }
	bool IsPVEModeControlled(CBaseEntity *entity)                                                                { NULL_RET(false); return ft_IsPVEModeControlled                (this, entity); }
	void DropSpellPickup(const Vector &pos, int tier)                                                            { NULL_RET(     );        ft_DropSpellPickup                    (this, pos, tier); }
	bool CanPlayerUseRespec(CTFPlayer *player)                                                                   { NULL_RET(false); return ft_CanPlayerUseRespec                 (this, player); }
	bool IsCompetitiveMode()                                                                                     { NULL_RET(false); return ft_IsCompetitiveMode                  (this); }

	bool FlagsMayBeCapped() { NULL_RET(false); return vt_FlagsMayBeCapped(this); }
	
	void Set_m_bPlayingMedieval(bool val)      { NULL_RET(); this->m_bPlayingMedieval      = val; }
	void Set_m_bPlayingMannVsMachine(bool val) { NULL_RET(); this->m_bPlayingMannVsMachine = val; }
	void SetRestartRoundTime(float time)       { NULL_RET(); this->m_flRestartRoundTime    = time; }
	
	DECL_SENDPROP(int[34],         m_ePlayerWantsRematch);
private:
	DECL_SENDPROP(bool,           m_bPlayingMedieval);
	DECL_SENDPROP(bool,           m_bPlayingMannVsMachine);
	DECL_SENDPROP(char[MAX_PATH], m_pszCustomUpgradesFile);
	DECL_SENDPROP(float,           m_flRestartRoundTime);
	
	
	static MemberFuncThunk<CTFGameRules *, bool, CTFPlayer *, int, unsigned short, CMannVsMachineUpgrades *> ft_CanUpgradeWithAttrib;
	static MemberFuncThunk<CTFGameRules *, int, CMannVsMachineUpgrades *, int, int, CTFPlayer *>             ft_GetCostForUpgrade;
	static MemberFuncThunk<CTFGameRules *, int, int>                                                         ft_GetUpgradeTier;
	static MemberFuncThunk<CTFGameRules *, bool, CTFPlayer *, int, int>                                      ft_IsUpgradeTierEnabled;
	static MemberFuncThunk<CTFGameRules *, void, CTFPlayer *, bool>                                          ft_PlayerReadyStatus_UpdatePlayerState;
	static MemberFuncThunk<CTFGameRules *, void, int, CTFPlayer *, bool, bool, bool>                         ft_DistributeCurrencyAmount;
	static MemberFuncThunk<CTFGameRules *, void, inputdata_t&>                                               ft_SetCustomUpgradesFile;
	static MemberFuncThunk<CTFGameRules *, void, CTFRadiusDamageInfo&>                                       ft_RadiusDamage;
	static MemberFuncThunk<CTFGameRules *, bool, CBaseEntity *>                                              ft_IsPVEModeControlled;
	static MemberFuncThunk<CTFGameRules *, void, const Vector &, int>                                        ft_DropSpellPickup;
	static MemberFuncThunk<CTFGameRules *, bool, CTFPlayer *>                                                ft_CanPlayerUseRespec;
	static MemberFuncThunk<CTFGameRules *, bool>                                                             ft_IsCompetitiveMode;
	
	static MemberVFuncThunk<CTFGameRules *, bool> vt_FlagsMayBeCapped;
};


inline void CGameRulesProxy::NotifyNetworkStateChanged()
{
	if (s_pGameRulesProxy != nullptr) {
		s_pGameRulesProxy->NetworkStateChanged();
	}
}


inline const CViewVectors *CGameRules::GetViewVectors() const
{
	IF_NULL {
		static const CViewVectors s_PleaseDontCrash;
		return &s_PleaseDontCrash;
	}
	
	return vt_GetViewVectors(this);
}


inline void CTFGameRules::SetCustomUpgradesFile(const char *path)
{
	NULL_RET();
	
	variant_t variant;
	variant.SetString(MAKE_STRING(path));
	
	inputdata_t inputdata;
	inputdata.pActivator = UTIL_EntityByIndex(0);
	inputdata.pCaller    = UTIL_EntityByIndex(0);
	inputdata.value      = variant;
	inputdata.nOutputID  = -1;
	
	this->SetCustomUpgradesFile(inputdata);
}


extern GlobalThunk<CGameRules *> g_pGameRules;
inline CGameRules *GameRules()
{
	CGameRules *gamerules = g_pGameRules;
	if (gamerules == nullptr) {
		Warning("WARNING: g_pGameRules == nullptr!\n");
	}
	return gamerules;
}

inline CMultiplayRules          *MultiplayRules()          { return reinterpret_cast<CMultiplayRules          *>(GameRules()); }
inline CTeamplayRules           *TeamplayGameRules()       { return reinterpret_cast<CTeamplayRules           *>(GameRules()); }
inline CTeamplayRoundBasedRules *TeamplayRoundBasedRules() { return reinterpret_cast<CTeamplayRoundBasedRules *>(GameRules()); }
inline CTFGameRules             *TFGameRules()             { return reinterpret_cast<CTFGameRules             *>(GameRules()); }


const char *GetRoundStateName(gamerules_roundstate_t state);


#undef IF_NULL
#undef NULL_RET


#endif
