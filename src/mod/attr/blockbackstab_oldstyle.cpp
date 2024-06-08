#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfbot.h"
#include "stub/tfentities.h"
#include "util/scope.h"


namespace Mod::Attr::BlockBackstab_OldStyle
{
	// Differences between CTFPlayer::CheckBlockBackstab pre-JungleInferno vs post-JungleInferno:
	// - Removal of unused-return-value (i.e. useless) call to CBaseEntity::IsBaseCombatWeapon
	// - In CBaseEntity::IsWearable() block:
	//   - Removal of CEconWearable::RemoveFrom(this) call (item with attr is no longer removed)
	//   - Addition of CBaseEntity::AddEffects(EF_NODRAW) call (item with attr is now made invisible)
	//   - Addition of CTFPlayerShared::SetItemChargeMeter(LOADOUT_POSITION_SECONDARY, 0.0f) call (secondary slot charge meter is now reset)
	// - Immediately after CBaseEntity::IsWearable() block:
	//   - Removal of UTIL_Remove call (item with attr is no longer removed)
	
	// Plan to restore it to its original behavior:
	// - Don't do any modifications if the item with the attr is a CTFWearableRazorback!
	// - Restore the CEconWearable::RemoveFrom call
	// - Don't worry about the EF_NODRAW stuff; we're REMOVING the entity so it doesn't matter
	// - Make the CTFPlayerShared::SetItemChargeMeter call ineffectual
	// - Restore the UTIL_Remove call
	
	
	RefCount rc_CTFPlayer_CheckBlockBackstab;
	CBaseEntity *the_ent = nullptr;
	DETOUR_DECL_MEMBER(bool, CTFPlayer_CheckBlockBackstab, CTFPlayer *attacker)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		SCOPED_INCREMENT(rc_CTFPlayer_CheckBlockBackstab);
		the_ent = nullptr;
		
		bool ret = DETOUR_MEMBER_CALL(attacker);
		
	//	{
	//		ConColorMsg(Color(0xff, 0xff, 0x00), "> CTFPlayer::CheckBlockBackstab: ret %d\n", (int)ret);
	//		
	//		ConColorMsg(Color(0xff, 0xff, 0x00), "> CTFPlayer::CheckBlockBackstab: the_ent #%d '%s'\n",
	//			ENTINDEX(the_ent), (the_ent != nullptr ? the_ent->GetClassname() : ""));
	//		
	//		auto razorback = rtti_cast<CTFWearableRazorback *>(the_ent);
	//		ConColorMsg(Color(0xff, 0xff, 0x00), "> CTFPlayer::CheckBlockBackstab: razorback #%d '%s'\n",
	//			ENTINDEX(razorback), (razorback != nullptr ? razorback->GetClassname() : ""));
	//		
	//		auto wearable = rtti_cast<CTFWearable  *>(the_ent);
	//		ConColorMsg(Color(0xff, 0xff, 0x00), "> CTFPlayer::CheckBlockBackstab: wearable #%d '%s'\n",
	//			ENTINDEX(wearable), (wearable != nullptr ? wearable->GetClassname() : ""));
	//	}
		
		if (ret && the_ent != nullptr && rtti_cast<CTFWearableRazorback *>(the_ent) == nullptr) {
			auto wearable = rtti_cast<CTFWearable *>(the_ent);
			auto weapon = rtti_cast<CTFWeaponBase *>(the_ent);
			
			bool do_bot_weapon_switch = false;
			
			if (wearable != nullptr) {
				wearable->RemoveFrom(player);
			} else if (weapon != nullptr) {
				if (weapon == player->GetActiveTFWeapon()) {
					do_bot_weapon_switch = true;
				}
			}
			
			UTIL_Remove(the_ent);
			
			/* a bit of a hack: if we just removed a weapon from a bot, then coax it into switching to the next-best
			 * weapon it has available to it, instead of staying locked in useless T-pose mode */
			if (player->IsBot()) {
				CTFBot *bot = ToTFBot(player);
				if (bot != nullptr) {
					const CKnownEntity *threat = bot->GetVisionInterface()->GetPrimaryKnownThreat(false);
					if (threat != nullptr) {
						bot->EquipBestWeaponForThreat(threat);
					} else {
						bot->EquipRequiredWeapon();
					}
				}
			}
		}
		
		return ret;
	}
	
	DETOUR_DECL_MEMBER(float, CAttributeManager_ApplyAttributeFloatWrapper, float val, CBaseEntity *ent, string_t name, CUtlVector<CBaseEntity *> *vec)
	{
		float ret = DETOUR_MEMBER_CALL(val, ent, name, vec);
		
	//	ConColorMsg(Color(0x00, 0xff, 0xff), "  > CAttributeManager::ApplyAttributeFloatWrapper(%.1f, #%d, \"%s\", 0x%08X)\n",
	//		val, ENTINDEX(ent), STRING(name), (uintptr_t)vec);
		
		if (rc_CTFPlayer_CheckBlockBackstab > 0) {
			if (ret != 0.0f && ent != nullptr && ent->IsPlayer() && strcmp(STRING(name), "set_blockbackstab_once") == 0 && vec != nullptr && !vec->IsEmpty()) {
				the_ent = vec->Head();
				
			//	ConColorMsg(Color(0x00, 0xff, 0xff), "  > CAttributeManager::ApplyAttributeFloatWrapper(%.1f, #%d, \"%s\", 0x%08X): the_ent := #%d '%s'\n",
			//		val, ENTINDEX(ent), STRING(name), (uintptr_t)vec, ENTINDEX(the_ent), (the_ent != nullptr ? the_ent->GetClassname() : ""));
			}
		}
		
		return ret;
	}
	
//	DETOUR_DECL_MEMBER(void, CBaseEntity_AddEffects, int nEffects)
//	{
//		ConColorMsg(Color(0x00, 0xff, 0xff), "  > CBaseEntity::AddEffects(0x%08X)\n",
//			nEffects);
//		
//		if (rc_CTFPlayer_CheckBlockBackstab > 0) {
//			the_ent = reinterpret_cast<CBaseEntity *>(this);
//			
//			ConColorMsg(Color(0x00, 0xff, 0xff), "  > CBaseEntity::AddEffects(0x%08X): the_ent := #%d '%s'\n",
//				nEffects, ENTINDEX(the_ent), (the_ent != nullptr ? the_ent->GetClassname() : ""));
//		}
//		
//		DETOUR_MEMBER_CALL(nEffects);
//	}
	
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, void, CTFPlayerShared_SetItemChargeMeter, loadout_positions_t slot, float value)
	{
	//	ConColorMsg(Color(0xff, 0x00, 0xff), "  > CTFPlayerShared::SetItemChargeMeter(%d, %.2f): the_ent #%d '%s'\n",
	//		(int)slot, value, ENTINDEX(the_ent), (the_ent != nullptr ? the_ent->GetClassname() : ""));
		
		if (rc_CTFPlayer_CheckBlockBackstab > 0) {
			if (slot == LOADOUT_POSITION_SECONDARY && value == 0.0f) {
				if (the_ent != nullptr && the_ent->IsWearable() && rtti_cast<CTFWearableRazorback *>(the_ent) == nullptr) {
				//	ConColorMsg(Color(0xff, 0x00, 0xff), "  > CTFPlayerShared::SetItemChargeMeter(%d, %.2f): bypassing meter modification\n",
				//		(int)slot, value);
					return;
				}
			}
		}
		
		DETOUR_MEMBER_CALL(slot, value);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Attr:BlockBackstab_OldStyle")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CheckBlockBackstab,                 "CTFPlayer::CheckBlockBackstab");
			MOD_ADD_DETOUR_MEMBER(CAttributeManager_ApplyAttributeFloatWrapper, "CAttributeManager::ApplyAttributeFloatWrapper");
		//	MOD_ADD_DETOUR_MEMBER(CBaseEntity_AddEffects,                       "CBaseEntity::AddEffects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_SetItemChargeMeter,           "CTFPlayerShared::SetItemChargeMeter [clone]");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_attr_blockbackstab_oldstyle", "0", FCVAR_NOTIFY,
		"Mod: make attribute \"set_blockbackstab_once\" on non-Razorback items behave as it did before the Jungle Inferno update (i.e. remove the item when stabbed)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
