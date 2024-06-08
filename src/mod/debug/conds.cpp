#include "mod.h"
#include "stub/tfplayer.h"
#include "util/clientmsg.h"


namespace Mod::Debug::Conds
{
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnConditionAdded, ETFCond cond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		auto player = shared->GetOuter();
		
		ClientMsgAll("[CondDebug] #%d \"%s\" +++ %d %s\n", ENTINDEX(player), player->GetPlayerName(), (int)cond, GetTFConditionName(cond));
		
		DETOUR_MEMBER_CALL(cond);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnConditionRemoved, ETFCond cond)
	{
		auto shared = reinterpret_cast<CTFPlayerShared *>(this);
		auto player = shared->GetOuter();
		
		ClientMsgAll("[CondDebug] #%d \"%s\" --- %d %s\n", ENTINDEX(player), player->GetPlayerName(), (int)cond, GetTFConditionName(cond));
		
		DETOUR_MEMBER_CALL(cond);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Conds")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionAdded,   "CTFPlayerShared::OnConditionAdded");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnConditionRemoved, "CTFPlayerShared::OnConditionRemoved");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_conds", "0", FCVAR_NOTIFY,
		"Debug: display player condition transitions",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
