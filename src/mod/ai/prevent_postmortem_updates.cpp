#include "mod.h"
#include "re/nextbot.h"
#include "util/rtti.h"
#include "stub/tfbot_behavior.h"


namespace Mod::AI::Prevent_Postmortem_Updates
{
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, Action_CTFBot_OnKilled, CTFBot *actor, const CTakeDamageInfo& info)
	{
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		if (rtti_cast<CTFBotMainAction *>(action) != nullptr) {
			return EventDesiredResult<CTFBot>::ChangeTo(CTFBotDead::New(), "I died!");
		}
		
		return DETOUR_MEMBER_CALL(actor, info);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:Prevent_Postmortem_Updates")
		{
			MOD_ADD_DETOUR_MEMBER(Action_CTFBot_OnKilled, "Action<CTFBot>::OnKilled");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_prevent_postmortem_updates", "0", FCVAR_NOTIFY,
		"Mod: fix bugs caused by CTFBot AI Update functions being called after bots have died",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
