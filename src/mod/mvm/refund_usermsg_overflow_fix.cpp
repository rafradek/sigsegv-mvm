#include "mod.h"

namespace Mod::MvM::UserMsg_Overflow
{

    class CTFPlayer;

	RefCount rc_CUpgrades_GrantOrRemoveAllUpgrades;
	DETOUR_DECL_MEMBER(void, CUpgrades_GrantOrRemoveAllUpgrades, CTFPlayer * player, bool remove, bool refund)
	{
		SCOPED_INCREMENT_IF(rc_CUpgrades_GrantOrRemoveAllUpgrades, refund);
		DETOUR_MEMBER_CALL(player, remove, refund);
	}

	DETOUR_DECL_MEMBER(void, CMannVsMachineStats_NotifyTargetPlayerEvent, CTFPlayer *player, int wave, int eType, int nCost)
	{
		if (rc_CUpgrades_GrantOrRemoveAllUpgrades == 0)
			DETOUR_MEMBER_CALL(player, wave, eType, nCost);
	}

	DETOUR_DECL_MEMBER(void, CMannVsMachineStats_PlayerEvent_Upgraded, CTFPlayer *player, uint16 nItemDef, uint16 nAttributeDef, uint8 nQuality, int16 nCost, bool bIsBottle)
	{
		if (rc_CUpgrades_GrantOrRemoveAllUpgrades == 0)
			DETOUR_MEMBER_CALL(player, nItemDef, nAttributeDef, nQuality, nCost, bIsBottle);
	}

    
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:UserMsg_Overflow")
		{
			MOD_ADD_DETOUR_MEMBER(CMannVsMachineStats_PlayerEvent_Upgraded, "CMannVsMachineStats::PlayerEvent_Upgraded");
			MOD_ADD_DETOUR_MEMBER(CMannVsMachineStats_NotifyTargetPlayerEvent, "CMannVsMachineStats::NotifyTargetPlayerEvent");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_GrantOrRemoveAllUpgrades, "CUpgrades::GrantOrRemoveAllUpgrades");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_usermsg_overflow_fix", "0", FCVAR_NOTIFY,
		"MvM: fix buffer overflow in net message",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

}