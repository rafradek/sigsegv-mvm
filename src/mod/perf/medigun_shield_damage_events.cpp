#ifdef SE_IS_TF2
#include "mod.h"
#include "stub/tf_shareddefs.h"


namespace Mod::Perf::Medigun_Shield_Damage_Events
{
	constexpr bool BLOCK_PLAYER = true;
	constexpr bool BLOCK_NPC    = true;
	
	
	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		
		if (event != nullptr) {
			if (BLOCK_PLAYER && strcmp(event->GetName(), "player_hurt") == 0) {
				int userid   = event->GetInt("userid",   -1);
				int weaponid = event->GetInt("weaponid", -1);
				int custom   = event->GetInt("custom",   -1);
				
				if (userid != -1 && weaponid == TF_WEAPON_MEDIGUN && custom == TF_DMG_CUSTOM_PLASMA) {
					mgr->FreeEvent(event);
					return false;
				}
			}
			
			if (BLOCK_NPC && strcmp(event->GetName(), "npc_hurt") == 0) {
				int entindex = event->GetInt("entindex", -1);
				int weaponid = event->GetInt("weaponid", -1);
				
				if (entindex != -1 && weaponid == TF_WEAPON_MEDIGUN) {
					CBaseEntity *victim = UTIL_EntityByIndex(entindex);
					
					if (victim != nullptr && strcmp(victim->GetClassname(), "tank_boss") == 0) {
						mgr->FreeEvent(event);
						return false;
					}
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(IGameEventManager2_FireEvent)(event, bDontBroadcast);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Medigun_Shield_Damage_Events")
		{
			MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEvent, "IGameEventManager2::FireEvent");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_medigun_shield_damage_events", "0", FCVAR_NOTIFY,
		"Mod: prevent \"player_hurt\" and \"npc_hurt\" game events from firing due to medigun shield damage",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
#endif