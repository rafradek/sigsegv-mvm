#include "mod.h"
#include "util/scope.h"
#include "stub/tfbot.h"


namespace Mod::Robot::Spawn_SetName
{
	RefCount rc_CTFBotSpawner_Spawn;
	DETOUR_DECL_MEMBER(int, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		return DETOUR_MEMBER_CALL(where, ents);
	}
	
	DETOUR_DECL_MEMBER(void, IVEngineServer_SetFakeClientConVarValue, edict_t *pEntity, const char *cvar, const char *value)
	{
		if (rc_CTFBotSpawner_Spawn > 0 && strcmp(cvar, "name") == 0) {
			CTFBot *bot = ToTFBot(GetContainingEntity(pEntity));
			if (bot != nullptr) {
				bot->SetPlayerName(value);
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(pEntity, cvar, value);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Robot:Spawn_SetName")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Spawn,                     "CTFBotSpawner::Spawn");
			MOD_ADD_DETOUR_MEMBER(IVEngineServer_SetFakeClientConVarValue, "IVEngineServer::SetFakeClientConVarValue");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_robot_spawn_setname", "0", FCVAR_NOTIFY,
		"Mod: use SetPlayerName instead of cl_name in CTFBotSpawner::Spawn",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
