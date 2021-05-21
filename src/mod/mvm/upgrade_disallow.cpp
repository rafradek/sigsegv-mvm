#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/entities.h"
#include "util/scope.h"
#include "stub/upgrades.h"
#include "stub/gamerules.h"

namespace Mod::MvM::Upgrade_Disallow
{
	ConVar cvar_explode_on_ignite("sig_mvm_upgrade_allow_explode_on_ignite", "1", FCVAR_NOTIFY,
		"Should explode on ignite be enabled");
	ConVar cvar_medigun_shield("sig_mvm_upgrade_allow_medigun_shield", "1", FCVAR_NOTIFY,
		"Should medigun shield be enabled");
	ConVar cvar_upgrade_over_cap("sig_mvm_upgrade_over_cap_exploit_fix", "0", FCVAR_NOTIFY,
		"Fix for upgrade over cap exploit");

	DETOUR_DECL_MEMBER(void, CUpgrades_PlayerPurchasingUpgrade, CTFPlayer *player, int itemslot, int upgradeslot, bool sell, bool free, bool b3)
	{
		if (!sell && !b3) {
			auto upgrade = reinterpret_cast<CUpgrades *>(this);
			
			if (upgradeslot >= 0 && upgradeslot < CMannVsMachineUpgradeManager::Upgrades().Count()) {
				DevMsg("bought %d %d\n",upgradeslot, itemslot);
				const char *upgradename = upgrade->GetUpgradeAttributeName(upgradeslot);
				
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
				else if (strcmp(upgradename,"engy sentry fire rate increased") == 0 &&
					(strcmp(TFGameRules()->GetCustomUpgradesFile(), "") == 0 || strcmp(TFGameRules()->GetCustomUpgradesFile(), "scripts/items/mvm_upgrades.txt") == 0)){
					//CTFWeaponBase* weapon = rtti_cast<CTFWeaponBase*>(player->GetWeapon(itemslot));
					float upgrade = 1.0f;
					CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, upgrade, mult_sentry_firerate);
					
					DevMsg("upgrade %f\n",upgrade);
					if (upgrade >= 0.79f && upgrade <= 0.81f) {
						gamehelpers->TextMsg(ENTINDEX(player), TEXTMSG_DEST_CENTER, "3rd sentry fire rate bonus upgrade is broken. Buy another upgrade");
						return;
					}
					
				}

			}
		}
		DETOUR_MEMBER_CALL(CUpgrades_PlayerPurchasingUpgrade)(player, itemslot, upgradeslot, sell, free, b3);
		
	}
	
	inline bool BIsAttributeValueWithDeltaOverCap(float flCurAttributeValue, float flAttrDeltaValue, float flCap)
	{
		return AlmostEqual( flCurAttributeValue, flCap )
			|| ( flAttrDeltaValue > 0 && flCurAttributeValue >= flCap )
			|| ( flAttrDeltaValue < 0 && flCurAttributeValue <= flCap );
	}

	#warning __gcc_regcall detours considered harmful!
	DETOUR_DECL_STATIC_CALL_CONVENTION(__gcc_regcall, attrib_definition_index_t, ApplyUpgrade_Default, const CMannVsMachineUpgrades& upgrade, CTFPlayer *pTFPlayer, CEconItemView *pEconItemView, int nCost, bool bDowngrade)
	{
		
		if (upgrade.m_iUIGroup == 0 && cvar_upgrade_over_cap.GetBool())
		{
			const CEconItemAttributeDefinition *pAttrDef = GetItemSchema()->GetAttributeDefinitionByName(upgrade.m_szAttribute);
			if (!pAttrDef)
				return INVALID_ATTRIB_DEF_INDEX;

			float fDefaultValue = FLT_MAX;

			// If we're trying to attach to an item and we don't have an item to attach to, give up.
			if ( !pEconItemView )
				return INVALID_ATTRIB_DEF_INDEX;

			FindAttribute<float>(pEconItemView->GetItemDefinition(), pAttrDef, &fDefaultValue);

			if (fDefaultValue != FLT_MAX && BIsAttributeValueWithDeltaOverCap(fDefaultValue, upgrade.m_flIncrement, upgrade.m_flCap))
			{
				return INVALID_ATTRIB_DEF_INDEX;
			}
		}

		auto result = DETOUR_STATIC_CALL(ApplyUpgrade_Default)(upgrade, pTFPlayer, pEconItemView, nCost, bDowngrade);
		DevMsg("defindex %d\n", result);
		return result;
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
			MOD_ADD_DETOUR_STATIC(ApplyUpgrade_Default, "ApplyUpgrade_Default");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_upgradedisallow", "0", FCVAR_NOTIFY,
		"Mod: Disallow buing certain upgrades",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

	
}
