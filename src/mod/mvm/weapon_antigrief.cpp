#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "util/scope.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"


namespace Mod::MvM::Weapon_AntiGrief
{
	ConVar cvar_scorchshot  ("sig_mvm_weapon_antigrief_scorchshot",   "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Scorch Shot");
	ConVar cvar_loosecannon ("sig_mvm_weapon_antigrief_loosecannon",  "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Loose Cannon");
	ConVar cvar_forceanature("sig_mvm_weapon_antigrief_forceanature", "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Force-A-Nature");
	ConVar cvar_moonshot    ("sig_mvm_weapon_antigrief_moonshot", "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Moonshot");
	ConVar cvar_eh_nerf    ("sig_mvm_explosiveheadshot_nerf", "1", FCVAR_NOTIFY, "Nerf from explosive headshots");
	ConVar cvar_stunball    ("sig_mvm_stunball_stun", "1", FCVAR_NOTIFY, "Balls now stun players");
	
	
	static inline bool BotIsAGiant(const CTFPlayer *player)
	{
		return (player->IsMiniBoss() || player->GetModelScale() > 1.0f);
	}
	
	
	RefCount rc_ScorchShot;
	DETOUR_DECL_MEMBER(void, CTFProjectile_Flare_Explode, CGameTrace *tr, CBaseEntity *ent)
	{
		SCOPED_INCREMENT(rc_ScorchShot);
		DETOUR_MEMBER_CALL(CTFProjectile_Flare_Explode)(tr, ent);
	}
	
	
	RefCount rc_LooseCannon;
	DETOUR_DECL_MEMBER(void, CTFGrenadePipebombProjectile_PipebombTouch, CBaseEntity *ent)
	{
		SCOPED_INCREMENT(rc_LooseCannon);
		DETOUR_MEMBER_CALL(CTFGrenadePipebombProjectile_PipebombTouch)(ent);
	}
	
	
	RefCount rc_ForceANature1;
	DETOUR_DECL_MEMBER(void, CTFScatterGun_ApplyPostHitEffects, const CTakeDamageInfo& info, CTFPlayer *victim)
	{
		SCOPED_INCREMENT_IF(rc_ForceANature1, BotIsAGiant(victim));
		DETOUR_MEMBER_CALL(CTFScatterGun_ApplyPostHitEffects)(info, victim);
	}
	RefCount rc_ForceANature2;
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyPushFromDamage, const CTakeDamageInfo& info, Vector vec)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		SCOPED_INCREMENT_IF(rc_ForceANature2, BotIsAGiant(player));
		DETOUR_MEMBER_CALL(CTFPlayer_ApplyPushFromDamage)(info, vec);
	}
	
	DETOUR_DECL_STATIC(bool, CanScatterGunKnockBack, CTFWeaponBase *scattergun, float damage, float distsqr)
	{
		if (cvar_forceanature.GetBool() && TFGameRules()->IsMannVsMachineMode()) {
			if (rc_ForceANature1 > 0 || rc_ForceANature2 > 0) {
				return false;
			}
		}
		
		return DETOUR_STATIC_CALL(CanScatterGunKnockBack)(scattergun, damage, distsqr);
	}
	
	
	static inline bool ShouldBlock_ScorchShot()  { return (cvar_scorchshot .GetBool() && rc_ScorchShot  > 0); }
	static inline bool ShouldBlock_LooseCannon() { return (cvar_loosecannon.GetBool() && rc_LooseCannon > 0); }
	static inline bool ShouldBlock_Moonshot(int flags) { return (flags == 11 && cvar_moonshot.GetBool()); }
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyGenericPushbackImpulse, const Vector& impulse)
	{
		if (ShouldBlock_ScorchShot() || ShouldBlock_LooseCannon()) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			
			if (BotIsAGiant(player) && TFGameRules()->IsMannVsMachineMode()) {
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_ApplyGenericPushbackImpulse)(impulse);
	}
	
	bool HasStunAttribute(CTFPlayer *attacker)
	{
		CTFWeaponBase *weapon = attacker->GetActiveTFWeapon();
		if (weapon != nullptr) {
			CAttributeList &attrlist = weapon->GetItem()->GetAttributeList();
			auto attr = attrlist.GetAttributeByName("mod bat launches balls");
			if (attr != nullptr) {
				float value = attr->GetValuePtr()->m_Float;
				return value >= 1.f;
			}
		}
		return false;
	}
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_StunPlayer, float duration, float slowdown, int flags, CTFPlayer *attacker)
	{
		//DevMsg("Stun ball %f %f %d\n",duration, slowdown, flags);
		if (flags == 257 && slowdown == 0.5f && duration != 5.0f && (cvar_stunball.GetBool() || HasStunAttribute(attacker))) {
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			auto player = shared->GetOuter();
			if (!(BotIsAGiant(player) || (player->IsBot() && player->HasTheFlag())))
				flags = TF_STUNFLAGS_SMALLBONK;
		}
		if (ShouldBlock_ScorchShot() || ShouldBlock_LooseCannon() || ShouldBlock_Moonshot(flags)) {
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			auto player = shared->GetOuter();
			
			if (BotIsAGiant(player) && TFGameRules()->IsMannVsMachineMode()) {
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayerShared_StunPlayer)(duration, slowdown, flags, attacker);
	}
	
	RefCount rc_CTFSniperRifle_ExplosiveHeadShot;
	CTFSniperRifle *sniperrifle = nullptr;
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_ExplosiveHeadShot, CTFPlayer *player1, CTFPlayer *player2)
	{
		SCOPED_INCREMENT_IF(rc_CTFSniperRifle_ExplosiveHeadShot,cvar_eh_nerf.GetBool());
		sniperrifle = reinterpret_cast<CTFSniperRifle *>(this);
		DETOUR_MEMBER_CALL(CTFSniperRifle_ExplosiveHeadShot)(player1, player2);
	}
	
	/* UTIL_EntitiesInSphere forwards call to partition->EnumerateElementsInSphere */
	RefCount rc_EnumerateElementsInSphere;
	float radius_eh = 0.f;
	float radius_eh_sqr = 0.f;
	const Vector *origin_eh = nullptr;
	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInSphere, SpatialPartitionListMask_t listMask, const Vector& origin, float radius, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		SCOPED_INCREMENT(rc_EnumerateElementsInSphere);
		if (rc_CTFSniperRifle_ExplosiveHeadShot > 0 && rc_EnumerateElementsInSphere <= 1) {
			//DevMsg("he radius: %f\n",radius);
			float chargedmg = sniperrifle->m_flChargedDamage;
			chargedmg -= 50.f;
			chargedmg *= 0.01f;
			radius_eh = radius*(0.64f+chargedmg*0.23f);
			radius_eh_sqr = radius_eh*0.7f;
			radius_eh_sqr *= radius_eh_sqr;
			origin_eh = &origin;
			
			return DETOUR_MEMBER_CALL(ISpatialPartition_EnumerateElementsInSphere)(listMask, origin, radius_eh, coarseTest, pIterator);
		}
		
		return DETOUR_MEMBER_CALL(ISpatialPartition_EnumerateElementsInSphere)(listMask, origin, radius, coarseTest, pIterator);
	}

	ConVar healing_arrow_headshot    ("sig_healing_arrow_headshot", "0", FCVAR_NOTIFY, "Healing arrow can headshot if no headshot attribute is disabled");

	DETOUR_DECL_MEMBER(bool, CTFProjectile_HealingBolt_CanHeadshot)
	{
		auto arrow = reinterpret_cast<CTFProjectile_HealingBolt *>(this);
		CBaseEntity *shooter = arrow->GetOriginalLauncher();

		if (shooter != nullptr) {
			int headshot = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(shooter, headshot, set_weapon_mode);
			if (headshot == 0)
				return true;
			DevMsg("Headshot bolt: %d\n",headshot);
		}
		return DETOUR_MEMBER_CALL(CTFProjectile_HealingBolt_CanHeadshot)();
	}
	/*DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& inputInfo)
	{
		DevMsg("takedamege\n");
		if (rc_CTFSniperRifle_ExplosiveHeadShot > 0){
			auto ent = reinterpret_cast<CBaseEntity *>(this);
			float distanceSq = origin_eh->DistToSqr(ent->GetAbsOrigin());
			CTakeDamageInfo newInfo = inputInfo;
			if (distanceSq >= radius_eh_sqr)
				newInfo.ScaleDamage(0.6f);
			DevMsg("Damage after: %f %f %f %d %d\n",radius_eh,distanceSq, inputInfo.GetDamage(), inputInfo.GetDamageCustom(), inputInfo.GetDamageType());
			return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(newInfo);
		}
		return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(inputInfo);
	}*/
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Weapon_AntiGrief")
		{
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_Explode, "CTFProjectile_Flare::Explode");
			
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_PipebombTouch, "CTFGrenadePipebombProjectile::PipebombTouch");
			
			MOD_ADD_DETOUR_MEMBER(CTFScatterGun_ApplyPostHitEffects, "CTFScatterGun::ApplyPostHitEffects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyPushFromDamage,     "CTFPlayer::ApplyPushFromDamage");
			MOD_ADD_DETOUR_STATIC(CanScatterGunKnockBack,            "CanScatterGunKnockBack");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyGenericPushbackImpulse, "CTFPlayer::ApplyGenericPushbackImpulse");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_StunPlayer,            "CTFPlayerShared::StunPlayer");
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_ExplosiveHeadShot,            "CTFSniperRifle::ExplosiveHeadShot");
			MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInSphere, "ISpatialPartition::EnumerateElementsInSphere");
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_HealingBolt_CanHeadshot, "CTFProjectile_HealingBolt::CanHeadshot");
			//MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage, "CBaseEntity::TakeDamage");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_weapon_antigrief", "0", FCVAR_NOTIFY,
		"Mod: disable some obnoxious weapon effects in MvM (primarily knockback stuff)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
