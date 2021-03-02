#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "util/scope.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"


namespace Mod::MvM::Weapon_Balancing
{
	ConVar cvar_eh_nerf          ("sig_mvm_explosiveheadshot_nerf", "1", FCVAR_NOTIFY, "Explosive headshots nerf (30% less range with no charge, damage and stun is reduced with distance)");
	ConVar cvar_backstab_nerf    ("sig_mvm_backstab_nerf", "1", FCVAR_NOTIFY, "Nerf for backstab (20% less damage for each successive backstab down to 60%)");
	ConVar cvar_madmilk_nerf     ("sig_mvm_madmilk_nerf", "1", FCVAR_NOTIFY, "Nerf for mad milk (reduce duration by 1 second for every 100 health restored)");
	ConVar cvar_beggar_stun_nerf ("sig_mvm_beggar_stun_nerf", "1", FCVAR_NOTIFY, "Nerf for beggar's bazooka (reduce stun slow from 85% to 60%, reduce duration by 15%)");
	
	ConVar cvar_heal_rage_from_upgrades_ratio("sig_heal_rage_from_upgrades_ratio", "1", FCVAR_NOTIFY,
		"Heal rage from upgrades ratio");

	ConVar cvar_heal_rage_ratio("sig_heal_rage_ratio", "1", FCVAR_NOTIFY,
		"Heal rage ratio");

	ConVar cvar_ubersaw_uber_decrease("sig_ubersaw_uber_decrease", "0", FCVAR_NOTIFY,
		"Ubersaw uber on hit decrease if rage scale attribute is present");
		
	RefCount rc_CTFSniperRifle_ExplosiveHeadShot;
	RefCount rc_CTFBaseRocket_CheckForStunOnImpact;
	float radius_eh = 0.f;
	float radius_eh_sqr = 0.f;
	const Vector *origin_eh = nullptr;

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_StunPlayer, float duration, float slowdown, int flags, CTFPlayer *attacker)
	{
		if (rc_CTFSniperRifle_ExplosiveHeadShot > 0) {
			slowdown *= 0.95f;
			auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
			if (origin_eh != nullptr) {
				float distance = origin_eh->DistTo(player->GetAbsOrigin());
				slowdown *= RemapValClamped(distance, 0.0f, radius_eh, 1.0f, 0.8f);
			}
		}
		else if (rc_CTFBaseRocket_CheckForStunOnImpact > 0) {
			slowdown -= 0.02f;

			int iCanOverload = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(attacker->GetActiveTFWeapon(), iCanOverload, can_overload);
			if (iCanOverload)
			{
				slowdown = 0.59f;
				duration *= 0.85f;
			}
		}

		DETOUR_MEMBER_CALL(CTFPlayerShared_StunPlayer)(duration, slowdown, flags, attacker);
	}
	
	CTFSniperRifle *sniperrifle = nullptr;
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_ExplosiveHeadShot, CTFPlayer *player1, CTFPlayer *player2)
	{
		SCOPED_INCREMENT_IF(rc_CTFSniperRifle_ExplosiveHeadShot, cvar_eh_nerf.GetBool());
		DevMsg("eh: %d\n",cvar_eh_nerf.GetBool());
		origin_eh = nullptr;
		sniperrifle = reinterpret_cast<CTFSniperRifle *>(this);
		if (!WeaponID_IsSniperRifle(sniperrifle->GetWeaponID()))
			sniperrifle = nullptr;

		DETOUR_MEMBER_CALL(CTFSniperRifle_ExplosiveHeadShot)(player1, player2);
	}
	
	/* UTIL_EntitiesInSphere forwards call to partition->EnumerateElementsInSphere */
	RefCount rc_EnumerateElementsInSphere;
	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInSphere, SpatialPartitionListMask_t listMask, const Vector& origin, float radius, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		SCOPED_INCREMENT(rc_EnumerateElementsInSphere);
		if (rc_CTFSniperRifle_ExplosiveHeadShot > 0 && rc_EnumerateElementsInSphere <= 1) {
			
			float chargedmg = sniperrifle == nullptr ? 50 : sniperrifle->m_flChargedDamage;
			chargedmg -= 50.f;
			chargedmg *= 0.01f;
			radius_eh = radius*(0.75f+chargedmg*0.3f);
			origin_eh = &origin;
			
			return DETOUR_MEMBER_CALL(ISpatialPartition_EnumerateElementsInSphere)(listMask, origin, radius_eh, coarseTest, pIterator);
		}
		
		return DETOUR_MEMBER_CALL(ISpatialPartition_EnumerateElementsInSphere)(listMask, origin, radius, coarseTest, pIterator);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_MakeBleed, CTFPlayer *attacker, CTFWeaponBase *weapon, float bleedTime, int bleeddmg, bool perm, int val)
	{
		if (rc_CTFSniperRifle_ExplosiveHeadShot > 0 && origin_eh != nullptr){
			auto player = reinterpret_cast<CTFPlayerShared *>(this)->GetOuter();
			float distance = origin_eh->DistTo(player->GetAbsOrigin());

			DevMsg("Damage before: %d\n", bleeddmg);

			float bleeddmgchanged = bleeddmg * RemapValClamped(distance, 0.0f, radius_eh, 1.0f, 0.5f);

			if (player->GetHealth() > bleeddmgchanged && player->GetHealth() <= bleeddmgchanged + 50 && player->GetHealth() >= bleeddmg / 2 + 50 ) {
				
				CWeaponMedigun *medigun = rtti_cast<CWeaponMedigun *>(player->GetActiveWeapon());
				if (medigun != nullptr && medigun->GetCharge() >= 1.0f) {
					bleeddmgchanged = bleeddmg / 2;
					//float chargerate = 1.0f;
					//CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(player, chargerate, mult_medigun_uberchargerate);
					//medigun->SetCharge(medigun->GetCharge() - chargerate * 0.06f);
					//DevMsg("Adding anti uber measure \n");
				}
			}
			bleeddmg = bleeddmgchanged;

			DevMsg("Damage after: %f %f %d\n",radius_eh,distance, bleeddmg);
		}
		DETOUR_MEMBER_CALL(CTFPlayerShared_MakeBleed)(attacker, weapon, bleedTime, bleeddmg, perm, val);
	}

	//std::map<CHandle<CBaseEntity>, int> backstab_count;
	DETOUR_DECL_MEMBER(float, CTFKnife_GetMeleeDamage, CBaseEntity *pTarget, int* piDamageType, int* piCustomDamage)
	{

		float ret = DETOUR_MEMBER_CALL(CTFKnife_GetMeleeDamage)(pTarget, piDamageType, piCustomDamage);
		auto knife = reinterpret_cast<CTFKnife *>(this);

		if (cvar_backstab_nerf.GetBool() && *piCustomDamage == TF_DMG_CUSTOM_BACKSTAB && !knife->GetTFPlayerOwner()->IsBot() && pTarget->IsPlayer() && ToTFPlayer(pTarget)->IsMiniBoss() ) {
			
			//int backstabs = backstab_count[pTarget];

			float armor_piercing_attr = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( knife, armor_piercing_attr, armor_piercing );
			float armor_piercing_dmg_mult = RemapValClamped(armor_piercing_attr, 0.0f, 100.0f, 1.0f, 5.0f);

			ret /= armor_piercing_dmg_mult;
			
			ret *= 1.0f + (armor_piercing_dmg_mult - 1.0f) * 0.818f;//RemapValClamped(backstabs, 0.0f, 2.0f, 1.0f, 0.6f);

			// backstab_count[pTarget] = backstabs + 1;

			
		}
		return ret;
	}

    DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond nCond, float flDuration, CBaseEntity *pProvider)
	{
		if (nCond == TF_COND_MAD_MILK && cvar_madmilk_nerf.GetBool())
        {
            if (flDuration > 5.0f)
                flDuration += 1.0f;
            else
                flDuration += 0.5f;
        }
		DETOUR_MEMBER_CALL(CTFPlayerShared_AddCond)(nCond, flDuration, pProvider);
	}

	RefCount rc_CTFWeaponBase_ApplyOnHitAttributes;
	CBaseEntity *victim_onhit;

	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ApplyOnHitAttributes, CBaseEntity *ent, CTFPlayer *player, const CTakeDamageInfo& info)
	{
		SCOPED_INCREMENT_IF(rc_CTFWeaponBase_ApplyOnHitAttributes, cvar_ubersaw_uber_decrease.GetBool());
		victim_onhit = ent;
        if (cvar_madmilk_nerf.GetBool()) {
            CTFPlayer *victim = ToTFPlayer(ent);

            if (victim != nullptr && victim->m_Shared->InCond(TF_COND_MAD_MILK)) {

                CBaseEntity *provider = victim->m_Shared->GetConditionProvider(TF_COND_MAD_MILK);

                int heal = std::min(ent->GetMaxHealth() - ent->GetHealth(), (int) (info.GetDamage() * 0.6f));

                if (heal > 0 && provider != nullptr)
                {
                    float duration = victim->m_Shared->GetConditionDuration(TF_COND_MAD_MILK);
                    victim->m_Shared->RemoveCond(TF_COND_MAD_MILK);
                    duration -= heal / 100.0f;
                    if (duration > 0.0f) {
                        victim->m_Shared->AddCond(TF_COND_MAD_MILK, duration, provider);
                    }
                }
            }
        }
		DETOUR_MEMBER_CALL(CTFWeaponBase_ApplyOnHitAttributes)(ent, player, info);
		victim_onhit = nullptr;
	}

	DETOUR_DECL_MEMBER(void, CTFBaseRocket_CheckForStunOnImpact, CTFPlayer* pTarget)
	{
        SCOPED_INCREMENT_IF(rc_CTFBaseRocket_CheckForStunOnImpact, cvar_beggar_stun_nerf.GetBool());
		DETOUR_MEMBER_CALL(CTFBaseRocket_CheckForStunOnImpact)(pTarget);
	}

	RefCount rc_HandleRageGain;
	DETOUR_DECL_STATIC(void, HandleRageGain, CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale)
	{
		SCOPED_INCREMENT_IF(rc_HandleRageGain, (0x10 /*kRageBuffFlag_OnHeal*/ & iRequiredBuffFlags) && !pPlayer->IsBot() && pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && 
			(cvar_heal_rage_from_upgrades_ratio.GetFloat() != 1.0f || cvar_heal_rage_ratio.GetFloat() != -1.0f));
		DETOUR_STATIC_CALL(HandleRageGain)(pPlayer, iRequiredBuffFlags, flDamage, fInverseRageGainScale);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_ModifyRage, float delta)
	{
		if (rc_HandleRageGain > 0) {
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			CTFPlayer *player = shared->GetOuter();
			
			float healrate = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( player, healrate, mult_medigun_healrate );
			float bonus = Max(1.0f, healrate - 0.5f);

			int iHealingMastery = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( player, iHealingMastery, healing_mastery );
			if ( iHealingMastery )
			{
				bonus *= RemapValClamped( (float)iHealingMastery, 1.f, 4.f, 1.25f, 2.0f );
			}

			delta = (delta / bonus) * (1.0f + ((bonus - 1.0f) * cvar_heal_rage_from_upgrades_ratio.GetFloat())) * cvar_heal_rage_ratio.GetFloat();
		}
		
		DETOUR_MEMBER_CALL(CTFPlayerShared_ModifyRage)(delta);
	}
	

	DETOUR_DECL_MEMBER(void, CWeaponMedigun_AddCharge, float val)
	{
		if (rc_CTFWeaponBase_ApplyOnHitAttributes && victim_onhit->IsPlayer()) {
			float ragescale = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( victim_onhit, ragescale, rage_giving_scale );
			val *= ragescale;
		}
		DETOUR_MEMBER_CALL(CWeaponMedigun_AddCharge)(val);
	}

	class CMod : public IMod //, public IModCallbackListener
	{
	public:
		CMod() : IMod("MvM:Weapon_Balancing")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_StunPlayer,                  "CTFPlayerShared::StunPlayer");
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_ExplosiveHeadShot,            "CTFSniperRifle::ExplosiveHeadShot");
			MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInSphere, "ISpatialPartition::EnumerateElementsInSphere");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_MakeBleed,                   "CTFPlayerShared::MakeBleed");
			MOD_ADD_DETOUR_MEMBER(CTFKnife_GetMeleeDamage,                     "CTFKnife::GetMeleeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_AddCond,                     "CTFPlayerShared::AddCond");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyOnHitAttributes,          "CTFWeaponBase::ApplyOnHitAttributes");
			MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_CheckForStunOnImpact,          "CTFBaseRocket::CheckForStunOnImpact");
			MOD_ADD_DETOUR_STATIC(HandleRageGain,                              "HandleRageGain");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_ModifyRage,                  "CTFPlayerShared::ModifyRage");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_AddCharge,                    "CWeaponMedigun::AddCharge");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_weapon_balancing", "0", FCVAR_NOTIFY,
		"Mod: change some weapon behavior",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
