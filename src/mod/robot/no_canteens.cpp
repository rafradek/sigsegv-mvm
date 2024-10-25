#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "stub/econ.h"
#include "stub/tf_shareddefs.h"


namespace Mod::Robot::No_Canteens
{
	RefCount rc_isBot;
	DETOUR_DECL_MEMBER(CEconItemView *, CTFPlayer_GetLoadoutItem, int pclass, int slot, bool b1)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		SCOPED_INCREMENT_IF(rc_isBot, slot == LOADOUT_POSITION_ACTION && TFGameRules()->IsMannVsMachineMode() && player->IsBot());
		
		return DETOUR_MEMBER_CALL(pclass, slot, b1);
	}

	DETOUR_DECL_MEMBER(CEconItemView *, CTFInventoryManager_GetBaseItemForClass, int pclass, int slot)
	{
		static auto defView = CEconItemView::Create();
		if (rc_isBot) {
			return defView;
		}
		return DETOUR_MEMBER_CALL(pclass, slot);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Robot:No_Canteens")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetLoadoutItem, "CTFPlayer::GetLoadoutItem");
			MOD_ADD_DETOUR_MEMBER(CTFInventoryManager_GetBaseItemForClass, "CTFInventoryManager::GetBaseItemForClass");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_robot_no_canteens", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: don't give stock canteens or spellbooks or grapples to robots, because that's idiotic",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
