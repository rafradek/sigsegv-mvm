#include "mod.h"
#include "stub/tfplayer.h"


#if 0


// sizeof: 0x15c
struct CMannVsMachineUpgrades
{
	char m_szAttribute[0x40]; // +0x000
	char m_szIcon[MAX_PATH];  // +0x040
	float m_flIncrement;      // +0x144
	float m_flCap;            // +0x148
	int m_nCost;              // +0x14c
	int m_iUIGroup;           // +0x150
	int m_iQuality;           // +0x154
	int m_iTier;              // +0x158
};


namespace Mod::Debug::Upgrade_Tiers
{
	DETOUR_DECL_STATIC(int, GetUpgradeStepData, CTFPlayer *player, int slot, int tier, int& count, bool& b1)
	{
		
		
		auto result = DETOUR_STATIC_CALL(player, slot, tier, count, b1);
		
		
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Upgrade_Tiers")
		{
			#error need addrs?
			#error need addr for client win if possible
			
			MOD_ADD_DETOUR_STATIC(GetUpgradeStepData, "GetUpgradeStepData");
			
		//	MOD_ADD_DETOUR_MEMBER(CTFGameRules_GetUpgradeTier,       "CTFGameRules::GetUpgradeTier");
		//	MOD_ADD_DETOUR_MEMBER(CTFGameRules_IsUpgradeTierEnabled, "CTFGameRules::IsUpgradeTierEnabled");
			
			
			// int GetUpgradeStepData(CTFPlayer *player, int slot, int tier, int& count, bool& b1)
			// - b1 -> client CUpgradeBuyPanel->bool_0x1cc  => 
			// - return value: ?
			
			// const CMannVsMachineUpgrades* CTFGameRules::GetUpgradeTier(int idx)
			// - idx is the ID number of the block from the mvm_upgrades.txt KV file
			
			// bool CTFGameRules::IsUpgradeTierEnabled(CTFPlayer *player, int slot, int idx)
			// - slot is the loadout slot
			// - idx is the ID number of the block from the mvm_upgrades.txt KV file
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_upgrade_tiers", "0", FCVAR_NOTIFY,
		"Debug: MvM upgrade tiers",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif
