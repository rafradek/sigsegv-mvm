#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
#include "stub/tfweaponbase.h"
#include "re/nextbot.h"
#include "re/path.h"

namespace Mod::Pop::Teleporter_Aggro
{
    //Allow bots to target teleporters, if they are not melee
	DETOUR_DECL_MEMBER(void, CTFBotVision_UpdatePotentiallyVisibleNPCVector)
	{
		if (!TFGameRules()->IsMannVsMachineMode()) {
			DETOUR_MEMBER_CALL(CTFBotVision_UpdatePotentiallyVisibleNPCVector)();
			return;
		}

		auto vision = reinterpret_cast<IVision *>(this);
		CBaseEntity *ent_bot = vision->GetBot()->GetEntity();
		bool allow = false;
		if (ent_bot->GetTeamNumber() == TF_TEAM_BLUE) {
			CTFBot *bot = ToTFBot(ent_bot);
			allow = bot != nullptr && rtti_cast<CTFWeaponBaseMelee *>(bot->GetActiveWeapon()) == nullptr;
		}

		if (allow)
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);

		DETOUR_MEMBER_CALL(CTFBotVision_UpdatePotentiallyVisibleNPCVector)();

		if (allow)
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Teleporter_Aggro")
		{
            MOD_ADD_DETOUR_MEMBER(CTFBotVision_UpdatePotentiallyVisibleNPCVector, "CTFBotVision::UpdatePotentiallyVisibleNPCVector");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_teleporter_aggro", "0", FCVAR_NOTIFY,
		"Mod: Make robots target red teleporters",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}