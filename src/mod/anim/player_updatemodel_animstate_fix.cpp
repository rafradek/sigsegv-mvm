#include "mod.h"
#include "stub/tfplayer.h"


namespace Mod::Anim::Player_UpdateModel_AnimState_Fix
{
	ConVar cvar_setcycle("sig_anim_player_updatemodel_animstate_fix_1", "0", FCVAR_NOTIFY,
		"Mod: also call CBaseAnimating::SetCycle(0.0f) in CTFPlayer::UpdateModel");
	ConVar cvar_resetsequenceinfo("sig_anim_player_updatemodel_animstate_fix_2", "0", FCVAR_NOTIFY,
		"Mod: also call CBaseAnimating::ResetSequenceInfo() in CTFPlayer::UpdateModel");
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_UpdateModel)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		DETOUR_MEMBER_CALL(Model)();
		
		DevMsg("[Player #%d \"%s\"] In CTFPlayer::UpdateModel, calling m_PlayerAnimState->OnNewModel()\n", ENTINDEX(player), player->GetPlayerName());
		
		// doing the call in one step results in some kind of OO error
		CMultiplayerAnimState *m_PlayerAnimState = player->m_PlayerAnimState;
		m_PlayerAnimState->OnNewModel();
	//	player->m_PlayerAnimState->OnNewModel();
		
		if (cvar_setcycle.GetBool()) { // NOPE
			DevMsg("[Player #%d \"%s\"] In CTFPlayer::UpdateModel, also calling SetCycle(0.0f)\n", ENTINDEX(player), player->GetPlayerName());
			player->SetCycle(0.0f);
		}
		if (cvar_resetsequenceinfo.GetBool()) { // NOPE
			DevMsg("[Player #%d \"%s\"] In CTFPlayer::UpdateModel, also calling ResetSequenceInfo()\n", ENTINDEX(player), player->GetPlayerName());
			player->ResetSequenceInfo();
		}
		
		// FINDINGS:
		// it appears that client-side animation is at fault for the out-of-sync issue
		// (stuff like m_flCycle is specifically excluded from the sendtable;
		// grep for 'cs_playeranimstate and clientside animation takes care of these on the client'
		// in the Source2007 tree for more on that)
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Anim:Player_UpdateModel_AnimState_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_UpdateModel, "CTFPlayer::UpdateModel");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(1));
			if (player == nullptr) return;
			
			NDebugOverlay::ScreenText(0.90f, 0.13f, CFmtStr("Server Cycle: %.02f", player->GetCycle()), 0xff, 0xff, 0xff, 0xff, 0.0f);
		}
	};
	CMod s_Mod;
	
	
	CON_COMMAND(sig_anim_cycle_adjust_fwd, "")
	{
		CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(1));
		if (player == nullptr) return;
		
		float adj = abs(strtof(args[0], nullptr));
		float f_int; float f_frac = modf(adj, &f_int);
		
		float cycle = player->GetCycle();
		cycle += f_frac;
		
		while (cycle > 1.0f) cycle -= 1.0f;
		while (cycle < 0.0f) cycle += 1.0f;
		
		player->SetCycle(cycle);
	}
	
	CON_COMMAND(sig_anim_cycle_adjust_rev, "")
	{
		CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(1));
		if (player == nullptr) return;
		
		float adj = abs(strtof(args[0], nullptr));
		float f_int; float f_frac = modf(adj, &f_int);
		
		float cycle = player->GetCycle();
		cycle -= f_frac;
		
		while (cycle > 1.0f) cycle -= 1.0f;
		while (cycle < 0.0f) cycle += 1.0f;
		
		player->SetCycle(cycle);
	}
	
	
	ConVar cvar_enable("sig_anim_player_updatemodel_animstate_fix", "0", FCVAR_NOTIFY,
		"Mod: fix animation state issues caused by CTFPlayer::UpdateModel failing to call m_PlayerAnimState->OnNewModel()",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
