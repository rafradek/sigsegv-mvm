#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "util/scope.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"


namespace Mod::MvM::Weapon_AntiGrief
{
	ConVar cvar_scorchshot   ("sig_mvm_weapon_antigrief_scorchshot",   "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Scorch Shot");
	ConVar cvar_loosecannon  ("sig_mvm_weapon_antigrief_loosecannon",  "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Loose Cannon");
	ConVar cvar_forceanature ("sig_mvm_weapon_antigrief_forceanature", "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Force-A-Nature");
	ConVar cvar_shortstop    ("sig_mvm_weapon_antigrief_shortstop", "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Shortstop");
	ConVar cvar_moonshot     ("sig_mvm_weapon_antigrief_moonshot", "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from the Moonshot");
	ConVar cvar_stunonhit    ("sig_mvm_weapon_antigrief_stunonhit", "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from stun on hit attributes");
	ConVar cvar_airborne_rage("sig_mvm_weapon_antigrief_airborne_rage", "1", FCVAR_NOTIFY, "Disable knockback and stun effects vs giant robots from minigun rage when airborne");
	ConVar cvar_stunball     ("sig_mvm_stunball_stun", "1", FCVAR_NOTIFY, "Balls now stun players");
	
	
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
	
	RefCount rc_ShortStop;
	DETOUR_DECL_MEMBER(void, CTFPistol_ScoutPrimary_Push)
	{
		SCOPED_INCREMENT(rc_ShortStop);
		DETOUR_MEMBER_CALL(CTFPistol_ScoutPrimary_Push)();
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

		if (cvar_airborne_rage.GetBool() && BotIsAGiant(player) && !(player->GetFlags() & FL_ONGROUND))
			return;
			
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
	
	RefCount rc_StunOnHit;
	CTFWeaponBase *weapon = nullptr;
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ApplyOnHitAttributes, CBaseEntity *ent, CTFPlayer *player, const CTakeDamageInfo& info)
	{
		SCOPED_INCREMENT(rc_StunOnHit);
		weapon = reinterpret_cast<CTFWeaponBase *>(this);
		DETOUR_MEMBER_CALL(CTFWeaponBase_ApplyOnHitAttributes)(ent, player, info);
	}
	static inline bool ShouldBlock_ScorchShot()  { return (cvar_scorchshot .GetBool() && rc_ScorchShot  > 0); }
	static inline bool ShouldBlock_LooseCannon() { return (cvar_loosecannon.GetBool() && rc_LooseCannon > 0); }
	static inline bool ShouldBlock_ShortStop()   { return (cvar_shortstop.GetBool()   && rc_ShortStop > 0); }
	static inline bool ShouldBlock_StunOnHit()   
	{ 
		if (cvar_stunonhit.GetBool() && rc_StunOnHit > 0 && weapon != nullptr) {
			float stundmg = 0.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, stundmg, stun_on_damage);
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, stundmg, aoe_blast_on_damage);
			if (stundmg != 0.0f) {
				return true;
			}
		}
		return false;
	}
	static inline bool ShouldBlock_Moonshot(int flags) { return ((flags & 9) == 9 && cvar_moonshot.GetBool()); }
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyGenericPushbackImpulse, const Vector& impulse)
	{
		if (ShouldBlock_ScorchShot() && TFGameRules()->IsMannVsMachineMode())
			return;
			
		if (ShouldBlock_LooseCannon() || ShouldBlock_ShortStop()) {
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
		DevMsg("Stun ball %f %f %d\n",duration, slowdown, flags);
	
		if (flags == 257 && slowdown == 0.5f && duration != 5.0f && (cvar_stunball.GetBool() || HasStunAttribute(attacker))) {
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			auto player = shared->GetOuter();
			if (!(BotIsAGiant(player) || (player->IsBot() && player->HasTheFlag())))
				flags = TF_STUNFLAGS_SMALLBONK;
		}
		if (ShouldBlock_ScorchShot() || ShouldBlock_LooseCannon() || ShouldBlock_StunOnHit() || ShouldBlock_Moonshot(flags)) {
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			auto player = shared->GetOuter();
			
			if (BotIsAGiant(player) && TFGameRules()->IsMannVsMachineMode()) {
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayerShared_StunPlayer)(duration, slowdown, flags, attacker);
	}
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Weapon_AntiGrief")
		{
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Flare_Explode, "CTFProjectile_Flare::Explode");
			
			MOD_ADD_DETOUR_MEMBER(CTFGrenadePipebombProjectile_PipebombTouch, "CTFGrenadePipebombProjectile::PipebombTouch");
			MOD_ADD_DETOUR_MEMBER(CTFPistol_ScoutPrimary_Push,                "CTFPistol_ScoutPrimary::Push");
			
			MOD_ADD_DETOUR_MEMBER(CTFScatterGun_ApplyPostHitEffects, "CTFScatterGun::ApplyPostHitEffects");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyPushFromDamage,     "CTFPlayer::ApplyPushFromDamage");
			MOD_ADD_DETOUR_STATIC(CanScatterGunKnockBack,            "CanScatterGunKnockBack");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyOnHitAttributes,          "CTFWeaponBase::ApplyOnHitAttributes");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyGenericPushbackImpulse,       "CTFPlayer::ApplyGenericPushbackImpulse");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_StunPlayer,                  "CTFPlayerShared::StunPlayer");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_weapon_antigrief", "0", FCVAR_NOTIFY,
		"Mod: disable some obnoxious weapon effects in MvM (primarily knockback stuff)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
