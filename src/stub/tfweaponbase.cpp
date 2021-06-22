#include "stub/tfweaponbase.h"
#include "mem/extract.h"

static constexpr uint8_t s_Buf_CTFWeaponBaseMelee_Holster[] = {
	0x55, 
	0x89, 0xE5 ,
	0x57, 
	0x56,
	0x53, 
	0x83, 0xEC, 0x7C, 
	0x8B, 0x7D, 0x08,
	0xC7, 0x87, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF //0c
};

struct CExtract_CTFWeaponBaseMelee_Holster : public IExtract<float *>
{
	using T = float *;
	
	CExtract_CTFWeaponBaseMelee_Holster() : IExtract<T>(sizeof(s_Buf_CTFWeaponBaseMelee_Holster)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFWeaponBaseMelee_Holster);
		
		//buf.SetDword(0x0c + 1, (uint32_t)AddrManager::GetAddr("gpGlobals"));
		
		mask.SetRange(0x0c + 2, 4, 0x00);
		//mask.SetRange(0x24 + 3, 4, 0x00);
		//mask.SetRange(0x2b + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFWeaponBaseMelee::Holster"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
	virtual uint32_t GetExtractOffset() const override { return 0x000c + 2; }
};

IMPL_SENDPROP(float,                         CBaseCombatWeapon, m_flNextPrimaryAttack,   CBaseCombatWeapon);
IMPL_SENDPROP(float,                         CBaseCombatWeapon, m_flNextSecondaryAttack, CBaseCombatWeapon);
IMPL_SENDPROP(float,                         CBaseCombatWeapon, m_flTimeWeaponIdle,      CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iState,                CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iPrimaryAmmoType,      CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iSecondaryAmmoType,    CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iClip1,                CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iClip2,                CBaseCombatWeapon);
IMPL_SENDPROP(CHandle<CBaseCombatCharacter>, CBaseCombatWeapon, m_hOwner,                CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iViewModelIndex,       CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iWorldModelIndex,      CBaseCombatWeapon);

MemberFuncThunk<const CBaseCombatWeapon *, bool> CBaseCombatWeapon::ft_IsMeleeWeapon("CBaseCombatWeapon::IsMeleeWeapon");

MemberVFuncThunk<const CBaseCombatWeapon *, int>                          CBaseCombatWeapon::vt_GetMaxClip1  (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetMaxClip1");
MemberVFuncThunk<const CBaseCombatWeapon *, int>                          CBaseCombatWeapon::vt_GetMaxClip2  (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetMaxClip2");
MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         CBaseCombatWeapon::vt_HasAmmo      (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::HasAmmo");
MemberVFuncThunk<      CBaseCombatWeapon *, void, CBaseCombatCharacter *> CBaseCombatWeapon::vt_Equip        (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::Equip");
MemberVFuncThunk<      CBaseCombatWeapon *, void, const Vector&>          CBaseCombatWeapon::vt_Drop         (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::Drop");
MemberVFuncThunk<const CBaseCombatWeapon *, const char *, int>            CBaseCombatWeapon::vt_GetViewModel (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetViewModel");
MemberVFuncThunk<const CBaseCombatWeapon *, const char *>                 CBaseCombatWeapon::vt_GetWorldModel(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetWorldModel");
MemberVFuncThunk<      CBaseCombatWeapon *, void>                         CBaseCombatWeapon::vt_SetViewModel (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::SetViewModel");
MemberVFuncThunk<      CBaseCombatWeapon *, void>                         CBaseCombatWeapon::vt_PrimaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::PrimaryAttack");
MemberVFuncThunk<      CBaseCombatWeapon *, void>                         CBaseCombatWeapon::vt_SecondaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::SecondaryAttack");
//MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         CBaseCombatWeapon::vt_CanPerformPrimaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::CanPerformPrimaryAttack");
MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         CBaseCombatWeapon::vt_CanPerformSecondaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::CanPerformSecondaryAttack");


IMPL_SENDPROP(float,                CTFWeaponBase, m_flLastFireTime,          CTFWeaponBase);
IMPL_SENDPROP(float,                CTFWeaponBase, m_flEffectBarRegenTime,    CTFWeaponBase);
IMPL_SENDPROP(float,                CTFWeaponBase, m_flEnergy,                CTFWeaponBase);
IMPL_SENDPROP(CHandle<CTFWearable>, CTFWeaponBase, m_hExtraWearable,          CTFWeaponBase);
IMPL_SENDPROP(CHandle<CTFWearable>, CTFWeaponBase, m_hExtraWearableViewModel, CTFWeaponBase);
IMPL_SENDPROP(bool                , CTFWeaponBase, m_bBeingRepurposedForTaunt,CTFWeaponBase);

MemberFuncThunk<CTFWeaponBase *, bool> CTFWeaponBase::ft_IsSilentKiller("CTFWeaponBase::IsSilentKiller");
MemberFuncThunk<const CTFWeaponBase *, CTFPlayer *> CTFWeaponBase::ft_GetTFPlayerOwner("CTFWeaponBase::GetTFPlayerOwner");

MemberVFuncThunk<const CTFWeaponBase *, int> CTFWeaponBase::vt_GetWeaponID(     TypeName<CTFBonesaw>(),     "CTFBonesaw::GetWeaponID");
MemberVFuncThunk<const CTFWeaponBase *, int> CTFWeaponBase::vt_GetPenetrateType(TypeName<CTFSniperRifle>(), "CTFSniperRifle::GetPenetrateType");
MemberVFuncThunk<CTFWeaponBase *, void, CTFPlayer *, Vector , Vector *, QAngle *, bool , float >   CTFWeaponBase::vt_GetProjectileFireSetup(TypeName<CTFWeaponBase>(),"CTFWeaponBase::GetProjectileFireSetup");
MemberVFuncThunk<const CTFWeaponBase *, bool> CTFWeaponBase::vt_ShouldRemoveInvisibilityOnPrimaryAttack(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::ShouldRemoveInvisibilityOnPrimaryAttack");

MemberVFuncThunk<CTFWeaponBaseGun *, float> CTFWeaponBaseGun::vt_GetProjectileGravity(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetProjectileGravity") ;
MemberVFuncThunk<CTFWeaponBaseGun *, float> CTFWeaponBaseGun::vt_GetProjectileSpeed(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetProjectileSpeed") ;
MemberVFuncThunk<CTFWeaponBaseGun *, float> CTFWeaponBaseGun::vt_GetProjectileDamage(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetProjectileDamage") ;
MemberVFuncThunk<const CTFWeaponBaseGun *, int> CTFWeaponBaseGun::vt_GetWeaponProjectileType(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetWeaponProjectileType") ;
MemberVFuncThunk<CTFWeaponBaseGun *, void, CBaseAnimating *> CTFWeaponBaseGun::vt_ModifyProjectile(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::ModifyProjectile") ;
MemberFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> CTFWeaponBaseGun::ft_UpdatePunchAngles("CTFWeaponBaseGun::UpdatePunchAngles");

MemberVFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> CTFWeaponBaseGun::vt_RemoveProjectileAmmo(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::RemoveProjectileAmmo");
MemberVFuncThunk<CTFWeaponBaseGun *, void>              CTFWeaponBaseGun::vt_DoFireEffects       (TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::DoFireEffects");
MemberVFuncThunk<CTFWeaponBaseGun *, bool>              CTFWeaponBaseGun::vt_ShouldPlayFireAnim  (TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::ShouldPlayFireAnim");

//MemberVFuncThunk<CTFCompoundBow *, bool>  CTFCompoundBow::vt_CanCharge(         TypeName<CTFCompoundBow>(), "CTFCompoundBow::CanCharge");
//MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetChargeBeginTime(TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetChargeBeginTime");
MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetChargeMaxTime(  TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetChargeMaxTime");
MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetCurrentCharge(  TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetCurrentCharge");


IMPL_SENDPROP(CTFMinigun::MinigunState_t, CTFMinigun, m_iWeaponState, CTFMinigun);

MemberFuncThunk<CTFSniperRifle *, void, CTFPlayer *, CTFPlayer *> CTFSniperRifle::ft_ExplosiveHeadShot            ("CTFSniperRifle::ExplosiveHeadShot");
MemberFuncThunk<CTFSniperRifle *, void, float &>                  CTFSniperRifle::ft_ApplyChargeSpeedModifications("CTFSniperRifle::ApplyChargeSpeedModifications");

MemberVFuncThunk<CTFSniperRifle *, float> CTFSniperRifle::vt_SniperRifleChargeRateMod(TypeName<CTFSniperRifle>(), "CTFSniperRifle::SniperRifleChargeRateMod");

IMPL_SENDPROP(float, CTFSniperRifle, m_flChargedDamage, CTFSniperRifle);


MemberFuncThunk<CTFSpellBook *, void, int>                  CTFSpellBook::ft_RollNewSpell("CTFSpellBook::RollNewSpell");

IMPL_SENDPROP(int, CTFSpellBook, m_iSelectedSpellIndex, CTFSpellBook);
IMPL_SENDPROP(int, CTFSpellBook, m_iSpellCharges, CTFSpellBook);

MemberFuncThunk<CTFSniperRifleDecap *, int> CTFSniperRifleDecap::ft_GetCount("CTFSniperRifleDecap::GetCount");

IMPL_EXTRACT(float, CTFWeaponBaseMelee, m_flSmackTime, new CExtract_CTFWeaponBaseMelee_Holster());

MemberVFuncThunk<CTFWeaponBaseMelee *, int>            CTFWeaponBaseMelee::vt_GetSwingRange(TypeName<CTFWeaponBaseMelee>(), "CTFWeaponBaseMelee::GetSwingRange");
MemberVFuncThunk<CTFWeaponBaseMelee *, bool, trace_t&> CTFWeaponBaseMelee::vt_DoSwingTrace (TypeName<CTFWeaponBaseMelee>(), "CTFWeaponBaseMelee::DoSwingTrace");


MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> CTFKnife::ft_CanPerformBackstabAgainstTarget("CTFKnife::CanPerformBackstabAgainstTarget");
MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> CTFKnife::ft_IsBehindAndFacingTarget        ("CTFKnife::IsBehindAndFacingTarget");


IMPL_SENDPROP(bool, CTFBottle, m_bBroken, CTFBottle);


IMPL_SENDPROP(CHandle<CTFWearableRobotArm>, CTFRobotArm, m_hRobotArm, CTFRobotArm);


MemberVFuncThunk<CWeaponMedigun *, float> CWeaponMedigun::vt_GetHealRate(TypeName<CWeaponMedigun>(), "CWeaponMedigun::GetHealRate");

IMPL_SENDPROP(CHandle<CBaseEntity>, CWeaponMedigun, m_hHealingTarget, CWeaponMedigun);
IMPL_SENDPROP(float, CWeaponMedigun, m_flChargeLevel, CWeaponMedigun);


MemberFuncThunk<CTFFlameThrower *, Vector, bool> CTFFlameThrower::ft_GetMuzzlePosHelper("CTFFlameThrower::GetMuzzlePosHelper");
MemberFuncThunk<CTFFlameThrower *, float> CTFFlameThrower::ft_GetDeflectionRadius("CTFFlameThrower::GetDeflectionRadius");


IMPL_SENDPROP(int,                        CBaseViewModel, m_nViewModelIndex, CBaseViewModel);
IMPL_SENDPROP(CHandle<CBaseEntity>,       CBaseViewModel, m_hOwner,          CBaseViewModel);
IMPL_SENDPROP(CHandle<CBaseCombatWeapon>, CBaseViewModel, m_hWeapon,         CBaseViewModel);


static StaticFuncThunk<bool, int> ft_WeaponID_IsSniperRifle("WeaponID_IsSniperRifle");
bool WeaponID_IsSniperRifle(int id) { return ft_WeaponID_IsSniperRifle(id); }

static StaticFuncThunk<bool, int> ft_WeaponID_IsSniperRifleOrBow("WeaponID_IsSniperRifleOrBow");
bool WeaponID_IsSniperRifleOrBow(int id) { return ft_WeaponID_IsSniperRifleOrBow(id); }


static StaticFuncThunk<int, const char *> ft_GetWeaponId("GetWeaponId");
int GetWeaponId(const char *name) { return ft_GetWeaponId(name); }

static StaticFuncThunk<const char *, int> ft_WeaponIdToAlias("WeaponIdToAlias");
const char *WeaponIdToAlias(int weapon_id) { return ft_WeaponIdToAlias(weapon_id); }

float CalculateProjectileSpeed(CTFWeaponBaseGun *weapon) {
	if (weapon == nullptr)
		return 0.0f;
		
	float speed = 0.0f;

	int weaponid = weapon->GetWeaponID();
	int projid = 0;
	CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, projid, override_projectile_type);

	if (projid == 0) {
		projid = weapon->GetWeaponProjectileType();
	}

	if (projid == TF_PROJECTILE_ROCKET) {
		speed = 1100.0f;
	}
	else if (projid == TF_PROJECTILE_FLARE) {
		speed = 2000.0f;
	}
	else if (projid == TF_PROJECTILE_SYRINGE) {
		speed = 1000.0f;
	}
	else if (projid == TF_PROJECTILE_ENERGY_RING) {
		int penetrate = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, penetrate, energy_weapon_penetration);

		speed = penetrate ? 840.0f : 1200.0f;
	}
	else if (projid == TF_PROJECTILE_BALLOFFIRE) {
		speed = 3000.0f;
	}
	else {
		speed = weapon->GetProjectileSpeed();
	}

	// Grenade launcher has mult_projectile_speed defined in getprojectilespeed
	if (weaponid != TF_WEAPON_GRENADELAUNCHER && weaponid != TF_WEAPON_CANNON
		&& weaponid != TF_WEAPON_CROSSBOW && weaponid != TF_WEAPON_COMPOUND_BOW
		&& weaponid != TF_WEAPON_GRAPPLINGHOOK && weaponid != TF_WEAPON_SHOTGUN_BUILDING_RESCUE){

		float mult_speed = 1.0f;
		CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(weapon, mult_speed, mult_projectile_speed);
		speed *= mult_speed;
	}

	if (projid == TF_PROJECTILE_ROCKET) {
		int specialist = 0;
		CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, specialist, rocket_specialist);
		speed *= RemapVal(specialist, 1.f, 4.f, 1.15f, 1.6f);
	}

	return speed;
}

const char *TranslateWeaponEntForClass_improved(const char *name, int classnum)
{
	if (strncasecmp(name, "tf_weapon_", 10) == 0)
	{
		if (strcasecmp(name+10, "shotgun") == 0) {
			DevMsg("is shotgun\n");
			switch (classnum) {
			case TF_CLASS_SOLDIER:
				return "tf_weapon_shotgun_soldier";
			case TF_CLASS_PYRO:
				return "tf_weapon_shotgun_pyro";
			case TF_CLASS_HEAVYWEAPONS:
				return "tf_weapon_shotgun_hwg";
			case TF_CLASS_ENGINEER:
				return "tf_weapon_shotgun_primary";
			default:
				return "tf_weapon_shotgun_pyro";
			}
		}
	
		if (strcasecmp(name+10, "throwable") == 0) {
			switch (classnum) {
			case TF_CLASS_MEDIC:
				return "tf_weapon_throwable_primary";
			default:
				return "tf_weapon_throwable_secondary";
			}
		}
		
		if (strcasecmp(name+10, "parachute") == 0) {
			switch (classnum) {
			case TF_CLASS_SOLDIER:
				return "tf_weapon_parachute_secondary";
			case TF_CLASS_DEMOMAN:
				return "tf_weapon_parachute_primary";
			default:
				return "tf_weapon_parachute";
			}
		}
		
		if (strcasecmp(name+10, "revolver") == 0) {
			switch (classnum) {
			case TF_CLASS_ENGINEER:
				return "tf_weapon_revolver_secondary";
			case TF_CLASS_SPY:
				return "tf_weapon_revolver";
			default:
				return "tf_weapon_revolver";
			}
		}
		if (strcasecmp(name+10, "pistol") == 0) {
			switch (classnum) {
			case TF_CLASS_SCOUT:
				return "tf_weapon_pistol_scout";
			case TF_CLASS_ENGINEER:
				return "tf_weapon_pistol";
			default:
				return "tf_weapon_pistol";
			}
		}
		
		if (strcasecmp(name+10, "shovel") == 0 || strcasecmp(name+10, "bottle") == 0) {
			switch (classnum) {
			case TF_CLASS_SOLDIER:
				return "tf_weapon_shovel";
			case TF_CLASS_DEMOMAN:
				return "tf_weapon_bottle";
			}
		}
	}
	else if (strcasecmp(name, "saxxy") == 0) {
		switch (classnum) {
		case TF_CLASS_SCOUT:
			return "tf_weapon_bat";
		case TF_CLASS_SOLDIER:
			return "tf_weapon_shovel";
		case TF_CLASS_PYRO:
			return "tf_weapon_fireaxe";
		case TF_CLASS_DEMOMAN:
			return "tf_weapon_bottle";
		case TF_CLASS_HEAVYWEAPONS:
			return "tf_weapon_fireaxe";
		case TF_CLASS_ENGINEER:
			return "tf_weapon_wrench";
		case TF_CLASS_MEDIC:
			return "tf_weapon_bonesaw";
		case TF_CLASS_SNIPER:
			return "tf_weapon_club";
		case TF_CLASS_SPY:
			return "tf_weapon_knife";
		}
	}
	
	/* if not handled: return original entity name, not an empty string */
	return name;
}