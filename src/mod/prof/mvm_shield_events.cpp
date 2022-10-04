#include "mod.h"
#include "stub/tfplayer.h"
#include "util/stringpool.h"


namespace vgui
{
	class Panel;
}


namespace Prof_MvM_Shield_Events
{
	StringPool pool(true);
	
	
	#define VPROF_CUSTOM(name) \
		VPROF_BUDGET(pool(name), pool("CUSTOM"))
	
	
	ConVar cvar_trace("sig_prof_mvm_shield_events_trace", "0", FCVAR_NOTIFY);
	
	
	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEventClientSide, IGameEvent *event)
	{
		const char *name = "IGameEventManager2::FireEventClientSide(nullptr)";
		if (event != nullptr) {
			name = CFmtStr("IGameEventManager2::FireEventClientSide(\"%s\")", event->GetName());
		}
		
		if (cvar_trace.GetBool()) {
			DevMsg("ENTER IGameEventManager2::FireEventClientSide(\"%s\")\n", name);
		}
		
		VPROF_CUSTOM(name);
		auto result = DETOUR_MEMBER_CALL(IGameEventManager2_FireEventClientSide)(event);
		
		if (cvar_trace.GetBool()) {
			DevMsg("EXIT  IGameEventManager2::FireEventClientSide\n");
		}
		
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CDamageAccountPanel_FireGameEvent, IGameEvent *event)
	{
		const char *name = "CDamageAccountPanel::FireGameEvent(nullptr)";
		if (event != nullptr) {
			name = CFmtStr("CDamageAccountPanel::FireGameEvent(\"%s\")", event->GetName());
		}
		
		if (cvar_trace.GetBool()) {
			DevMsg("ENTER CDamageAccountPanel::FireGameEvent(\"%s\")\n", name);
		}
		
		VPROF_CUSTOM(name);
		DETOUR_MEMBER_CALL(CDamageAccountPanel_FireGameEvent)(event);
		
		if (cvar_trace.GetBool()) {
			DevMsg("EXIT  CDamageAccountPanel::FireGameEvent\n");
		}
	}
	
	DETOUR_DECL_MEMBER(void, CDamageAccountPanel_DisplayDamageFeedback, CTFPlayer *pPlayer, CBaseCombatCharacter *pVictim, int iDamageAmount, int iHealth, bool bCrit)
	{
		if (cvar_trace.GetBool()) {
			DevMsg("ENTER CDamageAccountPanel::DisplayDamageFeedback(%08x, %08x, %d, %d, %s)\n",
				(uintptr_t)pPlayer, (uintptr_t)pVictim, iDamageAmount, iHealth, (bCrit ? "true" : "false"));
		}
		
		VPROF_CUSTOM("CDamageAccountPanel::DisplayDamageFeedback");
		DETOUR_MEMBER_CALL(CDamageAccountPanel_DisplayDamageFeedback)(pPlayer, pVictim, iDamageAmount, iHealth, bCrit);
		
		if (cvar_trace.GetBool()) {
			DevMsg("EXIT  CDamageAccountPanel::DisplayDamageFeedback\n");
		}
	}
	
#if 0
	// MSVC __cdecl?
	// access violation @ client.dll 20161002a .text:105512A9
	DETOUR_DECL_MEMBER(bool, vgui_AnimationController_StartAnimationSequence, vgui::Panel *pWithinParent, const char *sequenceName)
	{
		const char *name = CFmtStr("vgui::AnimationController::StartAnimationSequence(\"%s\")", sequenceName);
		
		VPROF_CUSTOM(name);
		return DETOUR_MEMBER_CALL(vgui_AnimationController_StartAnimationSequence)(pWithinParent, sequenceName);
	}
#endif
	
	DETOUR_DECL_MEMBER(bool, vgui_AnimationController_StartAnimationSequence, const char *sequenceName)
	{
		const char *name = CFmtStr("vgui::AnimationController::StartAnimationSequence(\"%s\")", sequenceName);
		
		if (cvar_trace.GetBool()) {
			DevMsg("ENTER vgui::AnimationController::StartAnimationSequence(\"%s\")\n", sequenceName);
		}
		
		VPROF_CUSTOM(name);
		auto result = DETOUR_MEMBER_CALL(vgui_AnimationController_StartAnimationSequence)(sequenceName);
		
		if (cvar_trace.GetBool()) {
			DevMsg("EXIT  vgui::AnimationController::StartAnimationSequence(\"%s\")\n", sequenceName);
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Prof:MvM_Shield_Events")
		{
			MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEventClientSide,          "IGameEventManager2::FireEventClientSide");
			MOD_ADD_DETOUR_MEMBER(CDamageAccountPanel_FireGameEvent,               "[client] CDamageAccountPanel::FireGameEvent");
			MOD_ADD_DETOUR_MEMBER(CDamageAccountPanel_DisplayDamageFeedback,       "[client] CDamageAccountPanel::DisplayDamageFeedback");
		//	MOD_ADD_DETOUR_MEMBER(vgui_AnimationController_StartAnimationSequence, "[client] vgui::AnimationController::StartAnimationSequence");
			
			
			// TODO: CDamageAccountPanel::FireGameEvent
			// client
			// ebpprologue unistr
			//   "attacker_player"
			//   "patient"
			
			// TODO: CDamageAccountPanel::DisplayDamageFeedback
			// client
			// ebpprologue unistr
			//   "DamagedPlayer"
			
			// TODO: vgui::AnimationController::StartAnimationSequence
			// client
			// addr 863be0
			// version 3634831
			
			// TODO: C_BaseEntity::EmitSound
			// NOPE, already covered, I *think*
		}

		virtual bool IsClientSide() override { return true; }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_prof_mvm_shield_events", "0", FCVAR_NOTIFY,
		"Mod: profile excessive game events due to MvM medigun shield damage",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

// player_hurt:
// CDamageAccountPanel::FireGameEvent
// C_CTFGameStats::FireGameEvent
// CAchievementTFDemoman_BounceAndKill::FireGameEvent_Internal
// CAchievementTFSoldier_BounceThenShotgun::FireGameEvent_Internal

// npc_hurt:
// CDamageAccountPanel::FireGameEvent
