#include "mod.h"
#include "stub/baseentity.h"
#include "stub/entities.h"


namespace Mod::Etc::Trigger_Filter_Fix
{
    DETOUR_DECL_MEMBER(void, CBaseTrigger_InputStartTouch, inputdata_t &data)
	{
		CBaseEntity *ent = data.pCaller;

		if (ent == nullptr) {
			return;// false;
		}

        DETOUR_MEMBER_CALL(CBaseTrigger_InputStartTouch)(data);
	}

	DETOUR_DECL_MEMBER(void, CBaseTrigger_InputEndTouch, inputdata_t &data)
	{
		CBaseEntity *ent = data.pCaller;

		if (ent == nullptr) {
			return;// false;
		}

        DETOUR_MEMBER_CALL(CBaseTrigger_InputEndTouch)(data);
	}

	DETOUR_DECL_MEMBER(void, CBaseFilter_InputTestActivator, inputdata_t &data)
	{
		CBaseEntity *ent = data.pActivator;

		if (ent == nullptr) {
			return;
		}

        DETOUR_MEMBER_CALL(CBaseFilter_InputTestActivator)(data);
	}

	DETOUR_DECL_MEMBER(void, CGameUI_Deactivate, CBaseEntity *pActivator)
	{
		CGameUI *ent = reinterpret_cast<CGameUI *>(this);

		if (ent->m_player == nullptr) return;

		if (pActivator == nullptr) {
			pActivator = UTIL_EntityByIndex(0);
		}

        DETOUR_MEMBER_CALL(CGameUI_Deactivate)(pActivator);
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Trigger_Filter_Fix")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseTrigger_InputStartTouch, "CBaseTrigger::InputStartTouch");
			MOD_ADD_DETOUR_MEMBER(CBaseTrigger_InputEndTouch, "CBaseTrigger::InputEndTouch");
			MOD_ADD_DETOUR_MEMBER(CBaseFilter_InputTestActivator, "CBaseFilter::InputTestActivator");
			MOD_ADD_DETOUR_MEMBER(CGameUI_Deactivate, "CGameUI::Deactivate");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_trigger_filter_fix", "0", FCVAR_NOTIFY,
		"Mod: Deleted entities trigger filtering crash fix",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}