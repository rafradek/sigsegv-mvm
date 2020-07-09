#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/entities.h"
#include "util/scope.h"
#include "stub/upgrades.h"


namespace Mod::MvM::Upgrade_Disallow
{
	ConVar cvar_explode_on_ignite("sig_mvm_upgrade_allow_explode_on_ignite", "1", FCVAR_NOTIFY,
		"Should explode on ignite be enabled");
	ConVar cvar_medigun_shield("sig_mvm_upgrade_allow_medigun_shield", "1", FCVAR_NOTIFY,
		"Should medigun shield be enabled");

	DETOUR_DECL_MEMBER(void, CUpgrades_PlayerPurchasingUpgrade, CTFPlayer *player, int slot, int tier, bool sell, bool free, bool b3)
	{
		if (!sell && !b3) {
			auto upgrade = reinterpret_cast<CUpgrades *>(this);
			
			if (tier > 0 && tier < CMannVsMachineUpgradeManager::Upgrades().Count()) {
				DevMsg("bought %d\n",tier);
				const char *upgradename = upgrade->GetUpgradeAttributeName(tier);
				if (!cvar_explode_on_ignite.GetBool() && strcmp(upgradename,"explode_on_ignite") == 0){
					gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "Explode on ignite is not allowed on this server");
					return;
				}
				else if (!cvar_medigun_shield.GetBool() && strcmp(upgradename,"generate rage on heal") == 0){
					gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "Projectile shield is not allowed on this server");
					return;
				}
				else if (strcmp(upgradename,"weapon burn time increased") == 0){
					gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "Burn time bonus upgrade is broken. Buy another upgrade");
					return;
				}
			}
		}
		DETOUR_MEMBER_CALL(CUpgrades_PlayerPurchasingUpgrade)(player, slot, tier, sell, free, b3);
		
	}
	
	
	/* NOTE: GiveDefaultItems also does this:
		CTFPlayerShared::RemoveCond(&this->m_Shared, TF_COND_OFFENSEBUFF, 0);
		CTFPlayerShared::RemoveCond(&this->m_Shared, TF_COND_DEFENSEBUFF, 0);
		CTFPlayerShared::RemoveCond(&this->m_Shared, TF_COND_REGENONDAMAGEBUFF, 0);
		CTFPlayerShared::RemoveCond(&this->m_Shared, TF_COND_NOHEALINGDAMAGEBUFF, 0);
		CTFPlayerShared::RemoveCond(&this->m_Shared, TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK, 0);
		CTFPlayerShared::RemoveCond(&this->m_Shared, TF_COND_DEFENSEBUFF_HIGH, 0);
	*/
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Upgrade_Disallow")
		{
			MOD_ADD_DETOUR_MEMBER(CUpgrades_PlayerPurchasingUpgrade, "CUpgrades::PlayerPurchasingUpgrade");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_upgradedisallow", "0", FCVAR_NOTIFY,
		"Mod: Disallow buing certain upgrades",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

	
}
