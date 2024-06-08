#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "stub/econ.h"
#include "stub/tf_shareddefs.h"


namespace Mod::Robot::No_Canteens
{
	constexpr int DEFIDX_CANTEEN_DEFAULT = 1163;
	
	
	DETOUR_DECL_MEMBER(CEconItemView *, CTFPlayer_GetLoadoutItem, int pclass, int slot, bool b1)
	{
		auto result = DETOUR_MEMBER_CALL(pclass, slot, b1);
		
		if (TFGameRules()->IsMannVsMachineMode() && result->GetItemDefIndex() == DEFIDX_CANTEEN_DEFAULT) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			if (player->GetTeamNumber() == TF_TEAM_BLUE && player->IsBot()) {
				return nullptr;
			}
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Robot:No_Canteens")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetLoadoutItem, "CTFPlayer::GetLoadoutItem");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_robot_no_canteens", "0", FCVAR_NOTIFY,
		"Mod: don't give stock canteens to robots, because that's idiotic",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
