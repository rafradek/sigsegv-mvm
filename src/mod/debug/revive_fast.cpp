#include "mod.h"
#include "stub/tfentities.h"


namespace Mod::Debug::Revive_Fast
{
	DETOUR_DECL_MEMBER(CTFReviveMarker *, CTFReviveMarker_Create)
	{
		CTFReviveMarker *marker = DETOUR_MEMBER_CALL();
		
		if (marker != nullptr) {
			DevMsg("[%8.3f] CTFReviveMarker::Create\n"
				"           [POST] m_iHealth    = %d\n"
				"           [POST] m_iMaxHealth = %d\n",
				gpGlobals->curtime, marker->GetHealth(), marker->GetMaxHealth());
		}
		
		return marker;
	}
	
	DETOUR_DECL_MEMBER(void, CTFReviveMarker_AddMarkerHealth, float amount)
	{
		auto marker = reinterpret_cast<CTFReviveMarker *>(this);
		if (marker != nullptr) {
			DevMsg("[%8.3f] CTFReviveMarker::AddMarkerHealth(%.1f)\n"
				"           [PRE]  m_iHealth    = %d\n"
				"           [PRE]  m_iMaxHealth = %d\n",
				gpGlobals->curtime, amount, marker->GetHealth(), marker->GetMaxHealth());
		}
		
		DETOUR_MEMBER_CALL(amount);
	}
	
	DETOUR_DECL_MEMBER(void, CTFReviveMarker_ReviveThink)
	{
		DETOUR_MEMBER_CALL();
		
		auto marker = reinterpret_cast<CTFReviveMarker *>(this);
		if (marker != nullptr) {
			DevMsg("[%8.3f] CTFReviveMarker::ReviveThink\n"
				"           [POST] m_iHealth    = %d\n"
				"           [POST] m_iMaxHealth = %d\n",
				gpGlobals->curtime, marker->GetHealth(), marker->GetMaxHealth());
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Revive_Fast")
		{
			MOD_ADD_DETOUR_MEMBER(CTFReviveMarker_Create,          "CTFReviveMarker::Create");
			MOD_ADD_DETOUR_MEMBER(CTFReviveMarker_AddMarkerHealth, "CTFReviveMarker::AddMarkerHealth");
			MOD_ADD_DETOUR_MEMBER(CTFReviveMarker_ReviveThink,     "CTFReviveMarker::ReviveThink");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_revive_fast", "0", FCVAR_NOTIFY,
		"Debug: diagnose instant revives",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
