#include "mod.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "stub/misc.h"


namespace Mod::MvM::ChangeBotAttributes_Crash_Fix
{
	RefCount rc_CPointPopulatorInterface_InputChangeBotAttributes;
	DETOUR_DECL_MEMBER(void, CPointPopulatorInterface_InputChangeBotAttributes, inputdata_t& inputdata)
	{
		SCOPED_INCREMENT(rc_CPointPopulatorInterface_InputChangeBotAttributes);
		DETOUR_MEMBER_CALL(CPointPopulatorInterface_InputChangeBotAttributes)(inputdata);
	}
	
	DETOUR_DECL_MEMBER(CTFBot::EventChangeAttributes_t *, CTFBot_GetEventChangeAttributes, const char *name)
	{
		auto player = reinterpret_cast<CBasePlayer *>(this);
		if (ToTFBot(player) == nullptr) {
			return nullptr;
		}
		else if (name == nullptr) {
			PrintToChatAll("Invalid changebotattributes name");
			return nullptr;
		}
		return DETOUR_MEMBER_CALL(CTFBot_GetEventChangeAttributes)(name);
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_AddEventChangeAttributes, CTFBot::EventChangeAttributes_t * ecattr)
	{
	 	if (ecattr == nullptr || ecattr->m_strName == nullptr ) {
			PrintToChatAll("Bot spawned with invalid event change attributes");
			return;
		}

	 	DETOUR_MEMBER_CALL(CTFBot_AddEventChangeAttributes)(ecattr);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:ChangeBotAttributes_Crash_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CPointPopulatorInterface_InputChangeBotAttributes, "CPointPopulatorInterface::InputChangeBotAttributes");
			MOD_ADD_DETOUR_MEMBER(CTFBot_GetEventChangeAttributes,                   "CTFBot::GetEventChangeAttributes");
			MOD_ADD_DETOUR_MEMBER(CTFBot_AddEventChangeAttributes,        "CTFBot::AddEventChangeAttributes");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_changebotattributes_crash_fix", "0", FCVAR_NOTIFY,
		"Mod: fix crash in which CPointPopulatorInterface::InputChangeBotAttributes assumes that all blue players are TFBots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
