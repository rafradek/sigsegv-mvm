#include "mod.h"
#include "stub/tf_shareddefs.h"
#include "stub/baseplayer.h"
#include "util/iterate.h"


namespace Debug_MvM_Shield_FPS
{
	CBasePlayer *UTIL_PlayerByUserId(int userID)
	{
		CBasePlayer *match = nullptr;
		
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			edict_t *edict = INDEXENT(i);
			if (edict == nullptr) {
			//	Warning("UTIL_PlayerByUserId(%d): #%d: INDEXENT returned nullptr\n", userID, i);
				continue;
			}
			
			int id = engine->GetPlayerUserId(edict);
			if (id == userID) {
			//	Warning("UTIL_PlayerByUserId(%d): #%d: user ID matches!\n", userID, i);
				match = reinterpret_cast<CBasePlayer *>(GetContainingEntity(edict));
			} else {
			//	Warning("UTIL_PlayerByUserId(%d): #%d: non-matching ID: %d\n", userID, i, id);
			}
		}
		
		return match;
		
		// for some reason, the code below DOES NOT WORK, and the lambda is always
		// passed a different pointer that still maps to player #1 (?????)
#if 0
		CBasePlayer *match = nullptr;
		
		ForEachPlayer([&](CBasePlayer *player) {
			DevMsg("UTIL_PlayerByUserId(%d): %08x #%d \"%s\"\n",
				userID, (uintptr_t)player, ENTINDEX(player), player->GetPlayerName());
			DevMsg("  connected: %d  userid: %d\n", (int)player->IsConnected(), player->GetUserID());
			
			if (player->GetUserID() == userID) {
				match = player;
				return false;
			}
			
			return true;
		});
		
		return match;
#endif
	}
	
	
	[[gnu::format(printf, 1, 2)]]
	void DevMsg_Tick(const char *fmt, ...)
	{
		static char buf[0x1000];
		
		va_list va;
		va_start(va, fmt);
		V_vsprintf_safe(buf, fmt, va);
		va_end(va);
		
		static int last_tick = -1;
		
		bool new_tick = false;
		if (gpGlobals->tickcount != last_tick) {
			last_tick = gpGlobals->tickcount;
			new_tick = true;
		}
		
		DevMsg("%s[Tick %d] %s", (new_tick ? "\n" : ""), gpGlobals->tickcount, buf);
	}
	
	
	ConVar cvar_player("sig_debug_mvm_shield_fps_player", "0", FCVAR_NOTIFY,
		"Debug: filter out \"player_hurt\" game events due to damaging players");
//	ConVar cvar_object("sig_debug_mvm_shield_fps_object", "0", FCVAR_NOTIFY,
//		"Debug: filter out \"npc_hurt\" game events due to damaging buildings");
	ConVar cvar_tank("sig_debug_mvm_shield_fps_tank", "0", FCVAR_NOTIFY,
		"Debug: filter out \"npc_hurt\" game events due to damaging tanks");
	
	DETOUR_DECL_MEMBER(bool, IGameEventManager2_FireEvent, IGameEvent *event, bool bDontBroadcast)
	{
		auto mgr = reinterpret_cast<IGameEventManager2 *>(this);
		
		if (event != nullptr) {
			if (cvar_player.GetBool() && strcmp(event->GetName(), "player_hurt") == 0) {
				int userid   = event->GetInt("userid",   -1);
				int weaponid = event->GetInt("weaponid", -1);
				int custom   = event->GetInt("custom",   -1);
				
				DevMsg_Tick("Event \"player_hurt\": userid %d, weaponid %d, custom %d\n",
					userid, weaponid, custom);
				
				if (userid != -1 && weaponid == TF_WEAPON_MEDIGUN && custom == TF_DMG_CUSTOM_PLASMA) {
					CBasePlayer *victim = UTIL_PlayerByUserId(userid);
					
					if (victim != nullptr) {
						DevMsg_Tick("Blocking \"player_hurt\" event for medigun shield vs %s #%d \"%s\"\n",
							victim->GetClassname(), ENTINDEX(victim), victim->GetPlayerName());
						
						mgr->FreeEvent(event);
						return false;
					}
				}
			}
			
			if (cvar_tank.GetBool() && strcmp(event->GetName(), "npc_hurt") == 0) {
				int entindex = event->GetInt("entindex", -1);
				int weaponid = event->GetInt("weaponid", -1);
				
				DevMsg_Tick("Event \"npc_hurt\": entindex %d, weaponid %d\n",
					entindex, weaponid);
				
				if (entindex != -1 && weaponid == TF_WEAPON_MEDIGUN) {
					CBaseEntity *victim = UTIL_EntityByIndex(entindex);
					
					if (victim != nullptr && strcmp(victim->GetClassname(), "tank_boss") == 0) {
						DevMsg_Tick("Blocking \"npc_hurt\" event for medigun shield vs %s #%d\n",
							victim->GetClassname(), entindex);
						
						mgr->FreeEvent(event);
						return false;
					}
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(event, bDontBroadcast);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:MvM_Shield_FPS")
		{
			MOD_ADD_DETOUR_MEMBER(IGameEventManager2_FireEvent, "IGameEventManager2::FireEvent");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_mvm_shield_fps", "0", FCVAR_NOTIFY,
		"Debug: MvM projectile shield FPS issues",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


#if 0
players:
	Game event "player_hurt", Tick 859750:
	- "userid" = "561"
	- "health" = "27220"
	- "attacker" = "573"
	- "damageamount" = "1"
	- "custom" = "46"
	- "showdisguisedcrit" = "0"
	- "crit" = "0"
	- "minicrit" = "0"
	- "allseecrit" = "0"
	- "weaponid" = "50"
	- "bonuseffect" = "4"
buildings:
	nope
tanks:
	Game event "npc_hurt", Tick 876309:
	- "entindex" = "89"
	- "health" = "4426"
	- "attacker_player" = "573"
	- "weaponid" = "50"
	- "damageamount" = "2"
	- "crit" = "0"
	- "boss" = "0"
#endif
