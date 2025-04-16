#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"


// FINDINGS:
// 
// Direct hits from normal flamethrowers have these damage bits:
//   0x01000800 = (DMG_IGNITE|DMG_PREVENT_PHYSICS_FORCE)
// Direct hits from the Dragon's Fury have these damage bits:
//   0x00220008 = (DMG_BURN|DMG_USEDISTANCEMOD|DMG_NOCLOSEDISTANCEMOD)
// 
// (The DF's damage bits are set manually in CTFProjectile_BallOfFire::Burn.)
// 
// DMG_BURN is clearly only meant to indicate afterburn damage.
// DMG_IGNITE is the damage bit which should be used for direct fire damage.
// 
// CTFWeaponBase::ApplyOnHitAttributes skips *numerous* on-hit effects if the
// damage bits include DMG_BURN.
// 
// Complete list of on-hit effects which are skipped in the DMG_BURN case:
// - Attribute charge_meter_on_hit (Persian Persuader)
// - Attribute speed_boost_on_hit (Eviction Notice; but not the Hot Hand: it uses a new attribute to do the exact same thing)
// + Condition TF_COND_MAD_MILK (Mad Milk heal-on-damage-done)
// + Condition TF_COND_REGENONDAMAGEBUFF (Concheror buff heal-on-damage-done)
// - [Medic] Attribute add_onhit_ubercharge (Ubersaw)
// - [Soldier and Pyro] Attribute rage_on_hit (currently unused)
// - Condition TF_COND_RUNE_SUPERNOVA ("rune charge"; some kind of Mannpower shit)
// - Attribute boost_on_damage (Baby Face's Blaster)
// - Attribute aoe_heal_chance (undocumented; grants TF_COND_RADIUSHEAL_ON_DAMAGE)
// - Attribute crits_on_damage (undocumented; grants TF_COND_CRITBOOSTED_CARD_EFFECT)
// - Attribute stun_on_damage (undocumented; stuns victim)
// - Attribute aoe_blast_on_damage (undocumented; does something very similar to explosive headshot)
// - [Heavy] Attribute generate_rage_on_dmg (specifically, the part of Knockback Rage that inflicts a slowdown stun on the victim; not the pushback itself)
// - Attribute mult_onhit_enemyspeed (Natascha)
// - Attribute mult_onhit_enemyspeed_major (unused)
// - Attribute mark_for_death (Fan-o-War)
// - Attribute stun_waist_high_airborne (unused)
// 
// Notably present in the list (marked with +) are Mad Milk and Concheror buff.


namespace Mod::Debug::DragonsFury_HealOnHit
{
	static CFmtStr ModuloTime()
	{
		float t = gpGlobals->curtime;
		if (t >= 1000.0f) t = fmod(t, 1000.0f);
		
		return CFmtStr("%6.2f", t);
	}
	
	static CFmtStr FormatEntity(CBaseEntity *ent)
	{
		if (ent == nullptr) return "<null>";
		
		CTFPlayer *player = ToTFPlayer(ent);
		if (player != nullptr) {
			return CFmtStr("[#%d %s \"%s\"]", ENTINDEX(ent), ent->GetClassname(), player->GetPlayerName());
		} else {
			auto weapon = rtti_cast<CTFWeaponBase *>(ent);
			if (weapon != nullptr) {
				return CFmtStr("[#%d %s %s]", ENTINDEX(ent), ent->GetClassname(), WeaponIdToAlias(weapon->GetWeaponID()));
			} else {
				return CFmtStr("[#%d %s]", ENTINDEX(ent), ent->GetClassname());
			}
		}
	}
	
	
	RefCount rc_CTFPlayer_OnTakeDamage;
	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, const CTakeDamageInfo& info)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_OnTakeDamage);
		
		auto victim = reinterpret_cast<CTFPlayer *>(this);
		
		ConColorMsg(Color(0xff, 0xff, 0x00, 0xff),
			"[%s CTFPlayer::OnTakeDamage]\n"
			"  victim: %s\n"
			"  dmginfo.inflictor: %s\n"
			"  dmginfo.attacker: %s\n"
			"  dmginfo.weapon: %s\n"
			"  dmginfo.custom: %s\n"
			"  dmginfo.dmgbits: %08x\n",
			ModuloTime().Get(),
			FormatEntity(victim).Get(),
			FormatEntity(info.GetInflictor()).Get(),
			FormatEntity(info.GetAttacker()).Get(),
			FormatEntity(info.GetWeapon()).Get(),
			GetCustomDamageName((ETFDmgCustom)info.GetDamageCustom()),
			info.GetDamageType());
		
		return DETOUR_MEMBER_CALL(CTFPlayer_OnTakeDamage)(info);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ApplyOnHitAttributes, CBaseEntity *ent, CTFPlayer *attacker, const CTakeDamageInfo& info)
	{
		auto weapon = reinterpret_cast<CTFWeaponBase *>(this);
		
		if (rc_CTFPlayer_OnTakeDamage > 0) {
			ConColorMsg(Color(0xff, 0xff, 0x00, 0xff),
				"[%s CTFWeaponBase::ApplyOnHitAttributes]\n"
				"  weapon: %s\n"
				"  ent: %s\n"
				"  attacker: %s\n"
				"  dmginfo.inflictor: %s\n"
				"  dmginfo.attacker: %s\n"
				"  dmginfo.weapon: %s\n"
				"  dmginfo.custom: %s\n"
				"  dmginfo.dmgbits: %08x\n",
				ModuloTime().Get(),
				FormatEntity(weapon).Get(),
				FormatEntity(ent).Get(),
				FormatEntity(attacker).Get(),
				FormatEntity(info.GetInflictor()).Get(),
				FormatEntity(info.GetAttacker()).Get(),
				FormatEntity(info.GetWeapon()).Get(),
				GetCustomDamageName((ETFDmgCustom)info.GetDamageCustom()),
				info.GetDamageType());
		}
		
		DETOUR_MEMBER_CALL(CTFWeaponBase_ApplyOnHitAttributes)(ent, attacker, info);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:DragonsFury_HealOnHit")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage, "CTFPlayer::OnTakeDamage");
			
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyOnHitAttributes, "CTFWeaponBase::ApplyOnHitAttributes");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_dragonsfury_healonhit", "0", FCVAR_NOTIFY,
		"Debug: figure out DF's lack of heal-on-hit",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
