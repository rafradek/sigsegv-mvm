#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/entities.h"
#include "util/scope.h"
#include "stub/tfweaponbase.h"

namespace Mod::MvM::UpgradeStation_Regen_Improved
{
	RefCount rc_CUpgrades_PlayerPurchasingUpgrade;
	ConVar cvar_only_creators("sig_mvm_upgradestation_creators", "0", FCVAR_NOTIFY,
		"The mod only affects creators.tf weapons");

	DETOUR_DECL_MEMBER(void, CUpgrades_PlayerPurchasingUpgrade, CTFPlayer *player, int slot, int tier, bool sell, bool free, bool b3)
	{
		SCOPED_INCREMENT(rc_CUpgrades_PlayerPurchasingUpgrade);
		DETOUR_MEMBER_CALL(CUpgrades_PlayerPurchasingUpgrade)(player, slot, tier, sell, free, b3);
		
	}
	
	/* disallow GiveDefaultItems, if we're in this function because of the upgrade station:
	 * - it removes YER disguises
	 * - it removes picked-up or generated weapons (i.e. anything not-from-loadout)
	 */
	DETOUR_DECL_MEMBER(void, CTFPlayer_GiveDefaultItems)
	{
		if (rc_CUpgrades_PlayerPurchasingUpgrade > 0) {
			if (cvar_only_creators.GetBool()) {
				// Creator tf items have item level set to -1
				bool found_creators_weapon = false;

				CTFPlayer *player = reinterpret_cast<CTFPlayer *>(this);
				for (int i = 0; i < player->GetNumWearables(); ++i) {
					CEconWearable *wearable = player->GetWearable(i);
					
					if (wearable == nullptr) continue;
					int level = wearable->GetItem()->m_iEntityLevel;
					
					if (level == 255) {
						found_creators_weapon = true;
						break;
					}
				}
				
				if (!found_creators_weapon) {
					for (int i = 0; i < player->WeaponCount(); ++i) {
						CBaseCombatWeapon *weapon = player->GetWeapon(i);
						if (weapon == nullptr) continue;
						int level = (int) (weapon->GetItem()->m_iEntityLevel);

						if (level == 255) {
							found_creators_weapon = true;
							break;
						} 
					}
				}
				if (found_creators_weapon)
					return;
			}
			else
				return;
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_GiveDefaultItems)();
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
		CMod() : IMod("MvM:UpgradeStation_Regen_Improved")
		{
			MOD_ADD_DETOUR_MEMBER(CUpgrades_PlayerPurchasingUpgrade, "CUpgrades::PlayerPurchasingUpgrade");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GiveDefaultItems,        "CTFPlayer::GiveDefaultItems");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_upgradestation_regen_improved", "0", FCVAR_NOTIFY,
		"Mod: fix annoying aspects of the health+ammo regen provided by the upgrade station",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
