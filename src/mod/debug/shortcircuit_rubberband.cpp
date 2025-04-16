#include "mod.h"
#include "stub/baseplayer.h"
#include "stub/lagcompensation.h"
#include "util/clientmsg.h"


namespace Mod::Debug::ShortCircuit_RubberBand
{
	// 1: convar to disable lag compensation; see if that makes a difference
	// 2: see if sv_clockcorrection_msecs makes a difference
	// 3: see if net_fakelag/net_fakejitter make a difference
	// 4: watch for console messages
	
	
	// is there just some circumstance under which it starts lag compensation
	// and then returns and forgets to end it?
	
	// ...well, holy shit, that's exactly what they did <sigh>
	
	
	ConVar cvar_spew("sig_debug_shortcircuit_rubberband_spew", "0", FCVAR_NOTIFY,
		"Debug: spew a bunch of developer crap to client console");
	
	int last_tick = -1;
	
	template<typename... ARGS>
	void ClientMsgAllTick(const char *fmt, ARGS&&... args)
	{
		if (!cvar_spew.GetBool()) return;
		
		if (last_tick != gpGlobals->tickcount) {
			last_tick = gpGlobals->tickcount;
			ClientMsgAll("-------------\n");
		}
		
		size_t fmt2_len = strlen(fmt) + 20;
		char *fmt2 = new char[fmt2_len];
		snprintf(fmt2, fmt2_len, "[tick %6d] %s", gpGlobals->tickcount, fmt);
		
		ClientMsgAll(fmt2, std::forward<ARGS>(args)...);
		
		delete[] fmt2;
	}
	
	
//	CBasePlayer *lagcomp_currentplayer = nullptr;
	
	DETOUR_DECL_MEMBER(void, CLagCompensationManager_StartLagCompensation, CBasePlayer *player, CUserCmd *cmd)
	{
//		lagcomp_currentplayer = player;
		ClientMsgAllTick("- lagcompensation->StartLagCompensation\n");
		DETOUR_MEMBER_CALL(player, cmd);
	}
	
	DETOUR_DECL_MEMBER(void, CLagCompensationManager_FinishLagCompensation, CBasePlayer *player)
	{
		DETOUR_MEMBER_CALL(player);
		ClientMsgAllTick("- lagcompensation->FinishLagCompensation\n");
//		lagcomp_currentplayer = nullptr;
	}
	
	
	ConVar cvar_fix("sig_debug_shortcircuit_rubberband_fix", "0", FCVAR_NOTIFY,
		"Debug: call lagcompensation->FinishLagCompensation() at the end of every ShockAttack");
	
	DETOUR_DECL_MEMBER(bool, CTFMechanicalArm_ShockAttack)
	{
		ClientMsgAllTick("CTFMechanicalArm::ShockAttack BEGIN\n");
		auto result = DETOUR_MEMBER_CALL();
		ClientMsgAllTick("CTFMechanicalArm::ShockAttack END\n");
		
		if (lagcompensation->IsCurrentlyDoingLagCompensation()) {
			ClientMsgAllTick(">> BUG: still in lag compensation after CTFMechanicalArm::ShockAttack!\n");
			
			if (cvar_fix.GetBool()) {
				ClientMsgAllTick(">> FIX: resolving this by calling lagcompensation->FinishLagCompensation.\n");
				lagcompensation->FinishLagCompensation(nullptr);
			}
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:ShortCircuit_RubberBand")
		{
			MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_StartLagCompensation,  "CLagCompensationManager::StartLagCompensation");
			MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_FinishLagCompensation, "CLagCompensationManager::FinishLagCompensation");
			
			MOD_ADD_DETOUR_MEMBER(CTFMechanicalArm_ShockAttack, "CTFMechanicalArm::ShockAttack");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_shortcircuit_rubberband", "0", FCVAR_NOTIFY,
		"Debug: investigate Short Circuit alt-fire causing players to rubber-band",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
