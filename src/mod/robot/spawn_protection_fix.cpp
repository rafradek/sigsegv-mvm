#include "mod.h"
#include "stub/tfbot.h"
#include "util/scope.h"


namespace Mod::Robot::Spawn_Protection_Fix
{
	RefCount rc_CTFBotMainAction_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_CTFBotMainAction_Update);
		return DETOUR_MEMBER_CALL(actor, dt);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
		if (rc_CTFBotMainAction_Update > 0 &&
			(nCond == TF_COND_INVULNERABLE || nCond == TF_COND_INVULNERABLE_WEARINGOFF) &&
			flDuration == 0.5f && pProvider == nullptr) {
			
			auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
			if (FindCaseInsensitive(player->GetPlayerName(), "overclock") == nullptr && !ToTFBot(player)->ExtAttr()[CTFBot::ExtendedAttr::NO_SPAWN_PROTECTION_FIX]) {
				return;
			}
		}
		
	//	#warning REMOVE THIS DEBUG MESSAGE!
	//	DevMsg("[SpawnProtect] rc == %d; nCond == %d; flDuration %c= 0.5f; pProvider %c= nullptr\n",
	//		rc_CTFBotMainAction_Update, (int)nCond, (flDuration == 0.5f ? '=' : '!'), (pProvider == nullptr ? '=' : '!'));
		
		DETOUR_MEMBER_CALL(nCond, flDuration, pProvider);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Robot:Spawn_Protection_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update, "CTFBotMainAction::Update");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_AddCond, "CTFPlayerShared::AddCond");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_robot_spawn_protection_fix", "0", FCVAR_NOTIFY,
		"Mod: make robot spawn protection only involve TF_COND_INVULNERABLE_HIDE_UNLESS_DAMAGED",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
