#include "stub/tfweaponbase.h"
#include "mem/extract.h"

static constexpr uint8_t s_Buf_CTFWeaponBaseMelee_Holster[] = {
#ifdef PLATFORM_64BITS
	0x55,                                                        // +0x0000 push    rbp
	0x48, 0x89, 0xe5,                                            // +0x0001 mov     rbp, rsp
	0x41, 0x57,                                                  // +0x0004 push    r15
	0x41, 0x56,                                                  // +0x0006 push    r14
	0x41, 0x55,                                                  // +0x0008 push    r13
	0x49, 0x89, 0xf5,                                            // +0x000a mov     r13, rsi
	0x41, 0x54,                                                  // +0x000d push    r12
	0x49, 0x89, 0xfc,                                            // +0x000f mov     r12, rdi
	0x53,                                                        // +0x0012 push    rbx
	0x48, 0x83, 0xec, 0x28,                                      // +0x0013 sub     rsp, 28h
	0xc7, 0x87, 0x10, 0x0a, 0x00, 0x00, 0x00, 0x00, 0x80, 0xbf,  // +0x0017 mov     dword ptr [rdi+0A10h], 0BF800000h
#else
	0x55, 
	0x89, 0xE5 ,
	0x57, 
	0x56,
	0x53, 
	0x83, 0xEC, 0x48, 
	0x8B, 0x5D, 0x08,
	0x8B, 0x7D, 0x0c,
	0xC7, 0x83, 0xE0, 0x07, 0x00, 0x00, 0x00, 0x00, 0x80, 0xBF //0f
#endif
};

struct CExtract_CTFWeaponBaseMelee_Holster : public IExtract<uint32_t>
{
	using T = uint32_t;
	
	CExtract_CTFWeaponBaseMelee_Holster() : IExtract<T>(sizeof(s_Buf_CTFWeaponBaseMelee_Holster)) {}
	
	virtual bool GetExtractInfo(ByteBuf& buf, ByteBuf& mask) const override
	{
		buf.CopyFrom(s_Buf_CTFWeaponBaseMelee_Holster);
		
		//buf.SetDword(0x0c + 1, (uint32_t)AddrManager::GetAddr("gpGlobals"));
		
#ifdef PLATFORM_64BITS
		mask.SetRange(0x17 + 2, 4, 0x00);
		mask.SetRange(0x13 + 3, 1, 0x00);
		mask.SetRange(0x17 + 2, 1, 0x00);
#else
		mask.SetRange(0x0f + 2, 4, 0x00);
		mask.SetRange(0x06 + 2, 1, 0x00);
		mask.SetRange(0x0f + 2, 1, 0x00);
#endif
		//mask.SetRange(0x24 + 3, 4, 0x00);
		//mask.SetRange(0x2b + 2, 4, 0x00);
		
		return true;
	}
	
	virtual const char *GetFuncName() const override   { return "CTFWeaponBaseMelee::Holster"; }
	virtual uint32_t GetFuncOffMin() const override    { return 0x0000; }
	virtual uint32_t GetFuncOffMax() const override    { return 0x0000; }
#ifdef PLATFORM_64BITS
	virtual uint32_t GetExtractOffset() const override { return 0x0017 + 2; }
#else
	virtual uint32_t GetExtractOffset() const override { return 0x000f + 2; }
#endif
};

IMPL_SENDPROP(float,                CTFWeaponBase, m_flLastFireTime,          CTFWeaponBase);
IMPL_SENDPROP(float,                CTFWeaponBase, m_flEffectBarRegenTime,    CTFWeaponBase);
IMPL_SENDPROP(float,                CTFWeaponBase, m_flEnergy,                CTFWeaponBase);
IMPL_SENDPROP(CHandle<CTFWearable>, CTFWeaponBase, m_hExtraWearable,          CTFWeaponBase);
IMPL_SENDPROP(CHandle<CTFWearable>, CTFWeaponBase, m_hExtraWearableViewModel, CTFWeaponBase);
IMPL_SENDPROP(bool                , CTFWeaponBase, m_bBeingRepurposedForTaunt,CTFWeaponBase);
IMPL_SENDPROP(float               , CTFWeaponBase, m_flReloadPriorNextFire,   CTFWeaponBase);
IMPL_SENDPROP(int,                  CTFWeaponBase, m_bDisguiseWeapon,         CTFWeaponBase);
IMPL_SENDPROP(int,                  CTFWeaponBase, m_iReloadMode,             CTFWeaponBase);
IMPL_REL_AFTER(bool               , CTFWeaponBase, m_bCurrentAttackIsCrit, m_flReloadPriorNextFire, void *, bool, bool);
IMPL_REL_AFTER(bool               , CTFWeaponBase, m_bCurrentCritIsRandom, m_bCurrentAttackIsCrit);
IMPL_SENDPROP(float,                CTFWeaponBase, m_flLastCritCheckTime,     CTFWeaponBase);
IMPL_REL_BEFORE(float,              CTFWeaponBase, m_flCritTime, m_flLastCritCheckTime, 0);
IMPL_REL_AFTER(int,                 CTFWeaponBase, m_iLastCritCheckFrame, m_flLastCritCheckTime);

MemberFuncThunk<CTFWeaponBase *, bool> CTFWeaponBase::ft_IsSilentKiller("CTFWeaponBase::IsSilentKiller");
MemberFuncThunk<const CTFWeaponBase *, CTFPlayer *> CTFWeaponBase::ft_GetTFPlayerOwner("CTFWeaponBase::GetTFPlayerOwner");
MemberFuncThunk<CTFWeaponBase *, float> CTFWeaponBase::ft_Energy_GetMaxEnergy("CTFWeaponBase::Energy_GetMaxEnergy");
MemberFuncThunk<CTFWeaponBase *, void> CTFWeaponBase::ft_CalcIsAttackCritical("CTFWeaponBase::CalcIsAttackCritical");
MemberFuncThunk<const CTFWeaponBase *, CTFWeaponInfo const &> CTFWeaponBase::ft_GetTFWeaponData("CTFWeaponBase::GetTFWpnData");
MemberFuncThunk<CTFWeaponBase *, void> CTFWeaponBase::ft_StartEffectBarRegen("CTFWeaponBase::StartEffectBarRegen");
MemberFuncThunk<CTFWeaponBase *, bool> CTFWeaponBase::ft_DeflectProjectiles("CTFWeaponBase::DeflectProjectiles");

MemberVFuncThunk<const CTFWeaponBase *, int> CTFWeaponBase::vt_GetWeaponID(     TypeName<CTFBonesaw>(),     "CTFBonesaw::GetWeaponID");
MemberVFuncThunk<const CTFWeaponBase *, int> CTFWeaponBase::vt_GetPenetrateType(TypeName<CTFSniperRifle>(), "CTFSniperRifle::GetPenetrateType");
MemberVFuncThunk<CTFWeaponBase *, void, CTFPlayer *, Vector , Vector *, QAngle *, bool , float >   CTFWeaponBase::vt_GetProjectileFireSetup(TypeName<CTFWeaponBase>(),"CTFWeaponBase::GetProjectileFireSetup");
MemberVFuncThunk<const CTFWeaponBase *, bool> CTFWeaponBase::vt_ShouldRemoveInvisibilityOnPrimaryAttack(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::ShouldRemoveInvisibilityOnPrimaryAttack");
MemberVFuncThunk<const CTFWeaponBase *, bool> CTFWeaponBase::vt_IsEnergyWeapon(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::IsEnergyWeapon");
MemberVFuncThunk<const CTFWeaponBase *, float> CTFWeaponBase::vt_Energy_GetShotCost(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::Energy_GetShotCost");
MemberVFuncThunk<CTFWeaponBase *, void>  CTFWeaponBase::vt_Misfire(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::Misfire");
MemberVFuncThunk<CTFWeaponBase *, Vector, int> CTFWeaponBase::vt_GetParticleColor(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::GetParticleColor");
MemberVFuncThunk<CTFWeaponBase *, int> CTFWeaponBase::vt_GetMaxHealthMod(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::GetMaxHealthMod");
MemberVFuncThunk<CTFWeaponBase *, float> CTFWeaponBase::vt_GetAfterburnRateOnHit(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::GetAfterburnRateOnHit");
MemberVFuncThunk<CTFWeaponBase *, float> CTFWeaponBase::vt_InternalGetEffectBarRechargeTime(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::InternalGetEffectBarRechargeTime");
MemberVFuncThunk<CTFWeaponBase *, float> CTFWeaponBase::vt_GetEffectBarProgress(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::GetEffectBarProgress");
MemberVFuncThunk<CTFWeaponBase *, void, CBaseEntity *, CTFPlayer *, const CTakeDamageInfo &> CTFWeaponBase::vt_ApplyOnHitAttributes(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::ApplyOnHitAttributes");
MemberVFuncThunk<CTFWeaponBase *, int> CTFWeaponBase::vt_GetSkin(TypeName<CTFWeaponBase>(),  "CTFWeaponBase::GetSkin");


MemberFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> CTFWeaponBaseGun::ft_UpdatePunchAngles("CTFWeaponBaseGun::UpdatePunchAngles");

MemberVFuncThunk<CTFWeaponBaseGun *, float> CTFWeaponBaseGun::vt_GetProjectileGravity(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetProjectileGravity") ;
MemberVFuncThunk<CTFWeaponBaseGun *, float> CTFWeaponBaseGun::vt_GetProjectileSpeed(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetProjectileSpeed") ;
MemberVFuncThunk<CTFWeaponBaseGun *, float> CTFWeaponBaseGun::vt_GetProjectileDamage(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetProjectileDamage") ;
MemberVFuncThunk<const CTFWeaponBaseGun *, int> CTFWeaponBaseGun::vt_GetWeaponProjectileType(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetWeaponProjectileType") ;
MemberVFuncThunk<CTFWeaponBaseGun *, void, CBaseAnimating *> CTFWeaponBaseGun::vt_ModifyProjectile(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::ModifyProjectile") ;
MemberVFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> CTFWeaponBaseGun::vt_RemoveProjectileAmmo(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::RemoveProjectileAmmo");
MemberVFuncThunk<CTFWeaponBaseGun *, void>              CTFWeaponBaseGun::vt_DoFireEffects       (TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::DoFireEffects");
MemberVFuncThunk<CTFWeaponBaseGun *, bool>              CTFWeaponBaseGun::vt_ShouldPlayFireAnim  (TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::ShouldPlayFireAnim");
MemberVFuncThunk<CTFWeaponBaseGun *, CBaseEntity *,CTFPlayer *> CTFWeaponBaseGun::vt_FireProjectile(TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::FireProjectile");
MemberVFuncThunk<CTFWeaponBaseGun *, int >              CTFWeaponBaseGun::vt_GetAmmoPerShot      (TypeName<CTFWeaponBaseGun>(), "CTFWeaponBaseGun::GetAmmoPerShot");


IMPL_SENDPROP(CHandle<CBaseEntity>, CTFGrapplingHook, m_hProjectile, CTFGrapplingHook);


IMPL_SENDPROP(float, CTFParticleCannon, m_flChargeBeginTime, CTFParticleCannon);


//MemberVFuncThunk<CTFCompoundBow *, bool>  CTFCompoundBow::vt_CanCharge(         TypeName<CTFCompoundBow>(), "CTFCompoundBow::CanCharge");
//MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetChargeBeginTime(TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetChargeBeginTime");
MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetChargeMaxTime(  TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetChargeMaxTime");
MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetCurrentCharge(  TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetCurrentCharge");

IMPL_SENDPROP(bool, CTFCompoundBow, m_bArrowAlight, CTFCompoundBow);


IMPL_SENDPROP(CTFMinigun::MinigunState_t, CTFMinigun, m_iWeaponState, CTFMinigun);
IMPL_SENDPROP(bool, CTFMinigun, m_bCritShot, CTFMinigun);

MemberFuncThunk<CTFSniperRifle *, void, CTFPlayer *, CTFPlayer *> CTFSniperRifle::ft_ExplosiveHeadShot            ("CTFSniperRifle::ExplosiveHeadShot");
MemberFuncThunk<CTFSniperRifle *, void, float &>                  CTFSniperRifle::ft_ApplyChargeSpeedModifications("CTFSniperRifle::ApplyChargeSpeedModifications");

MemberVFuncThunk<CTFSniperRifle *, float> CTFSniperRifle::vt_SniperRifleChargeRateMod(TypeName<CTFSniperRifle>(), "CTFSniperRifle::SniperRifleChargeRateMod");

IMPL_SENDPROP(float, CTFSniperRifle, m_flChargedDamage, CTFSniperRifle);

IMPL_SENDPROP(int, CTFPipebombLauncher, m_iPipebombCount, CTFPipebombLauncher);
IMPL_REL_AFTER(CUtlVector<EHANDLE>, CTFPipebombLauncher, m_Pipebombs, m_iPipebombCount);

MemberFuncThunk<CTFSpellBook *, void, int>                  CTFSpellBook::ft_RollNewSpell("CTFSpellBook::RollNewSpell");

IMPL_SENDPROP(int, CTFSpellBook, m_iSelectedSpellIndex, CTFSpellBook);
IMPL_SENDPROP(int, CTFSpellBook, m_iSpellCharges, CTFSpellBook);

MemberFuncThunk<CTFSniperRifleDecap *, int> CTFSniperRifleDecap::ft_GetCount("CTFSniperRifleDecap::GetCount");

IMPL_SENDPROP(float, CTFChargedSMG, m_flMinicritCharge, CTFChargedSMG);

IMPL_EXTRACT(float, CTFWeaponBaseMelee, m_flSmackTime, new CExtract_CTFWeaponBaseMelee_Holster());

MemberVFuncThunk<CTFWeaponBaseMelee *, int>            CTFWeaponBaseMelee::vt_GetSwingRange(TypeName<CTFWeaponBaseMelee>(), "CTFWeaponBaseMelee::GetSwingRange");
MemberVFuncThunk<CTFWeaponBaseMelee *, bool, trace_t&> CTFWeaponBaseMelee::vt_DoSwingTrace (TypeName<CTFWeaponBaseMelee>(), "CTFWeaponBaseMelee::DoSwingTrace");


MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> CTFKnife::ft_CanPerformBackstabAgainstTarget("CTFKnife::CanPerformBackstabAgainstTarget");
MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> CTFKnife::ft_IsBehindAndFacingTarget        ("CTFKnife::IsBehindAndFacingTarget");


MemberFuncThunk<CTFSword *, float> CTFSword::ft_GetSwordSpeedMod("CTFSword::GetSwordSpeedMod");
MemberFuncThunk<CTFSword *, int> CTFSword::ft_GetSwordHealthMod("CTFSword::GetSwordHealthMod");


IMPL_SENDPROP(bool, CTFBottle, m_bBroken, CTFBottle);


IMPL_SENDPROP(CHandle<CTFWearableRobotArm>, CTFRobotArm, m_hRobotArm, CTFRobotArm);


MemberVFuncThunk<CWeaponMedigun *, float> CWeaponMedigun::vt_GetHealRate(TypeName<CWeaponMedigun>(), "CWeaponMedigun::GetHealRate");

IMPL_SENDPROP(CHandle<CBaseEntity>, CWeaponMedigun, m_hHealingTarget, CWeaponMedigun);
IMPL_SENDPROP(float, CWeaponMedigun, m_flChargeLevel, CWeaponMedigun);
IMPL_SENDPROP(bool, CWeaponMedigun, m_bChargeRelease, CWeaponMedigun);

IMPL_SENDPROP(int, CTFWeaponBuilder, m_iObjectType, CTFWeaponBuilder);
IMPL_SENDPROP(int, CTFWeaponBuilder, m_iObjectMode, CTFWeaponBuilder);
IMPL_SENDPROP(bool[4], CTFWeaponBuilder, m_aBuildableObjectTypes, CTFWeaponBuilder);

MemberFuncThunk<CTFFlameThrower *, Vector, bool> CTFFlameThrower::ft_GetMuzzlePosHelper("CTFFlameThrower::GetMuzzlePosHelper");
MemberFuncThunk<CTFFlameThrower *, float> CTFFlameThrower::ft_GetDeflectionRadius("CTFFlameThrower::GetDeflectionRadius");

IMPL_SENDPROP(int, CTFFlameThrower, m_iWeaponState, CTFFlameThrower);


StaticFuncThunk<WEAPON_FILE_INFO_HANDLE, const char *> ft_LookupWeaponInfoSlot("LookupWeaponInfoSlot");
WEAPON_FILE_INFO_HANDLE LookupWeaponInfoSlot(const char * className) { return ft_LookupWeaponInfoSlot(className); }

StaticFuncThunk<FileWeaponInfo_t *, WEAPON_FILE_INFO_HANDLE> ft_GetFileWeaponInfoFromHandle("GetFileWeaponInfoFromHandle");
FileWeaponInfo_t *GetFileWeaponInfoFromHandle(WEAPON_FILE_INFO_HANDLE handle) { return ft_GetFileWeaponInfoFromHandle(handle); }

static StaticFuncThunk<bool, int> ft_WeaponID_IsSniperRifle("WeaponID_IsSniperRifle");
bool WeaponID_IsSniperRifle(int id) { return ft_WeaponID_IsSniperRifle(id); }

static StaticFuncThunk<bool, int> ft_WeaponID_IsSniperRifleOrBow("WeaponID_IsSniperRifleOrBow");
bool WeaponID_IsSniperRifleOrBow(int id) { return ft_WeaponID_IsSniperRifleOrBow(id); }


static StaticFuncThunk<int, const char *> ft_GetWeaponId("GetWeaponId");
int GetWeaponId(const char *name) { return ft_GetWeaponId(name); }

static StaticFuncThunk<const char *, int> ft_WeaponIdToAlias("WeaponIdToAlias");
const char *WeaponIdToAlias(int weapon_id) { return ft_WeaponIdToAlias(weapon_id); }


bool CTFWeaponBase::CalcIsAttackCriticalPoll(bool checkRandom) {
	bool oldIsCritical = this->m_bCurrentAttackIsCrit;
	bool oldIsCriticalRandom = this->m_bCurrentCritIsRandom;

	float oldCritBucket = this->m_flCritTokenBucket;
	int oldCritChecks = this->m_nCritChecks;
	int oldCritSeedRequests = this->m_nCritSeedRequests;
	float oldLastCritCheckTime = this->m_flLastCritCheckTime;
	float oldCritTime = this->m_flCritTime;
	int oldLastCritCheckFrame = this->m_iLastCritCheckFrame;
	this->CalcIsAttackCritical();
	
	bool isCrit = checkRandom ? m_bCurrentCritIsRandom : m_bCurrentAttackIsCrit;
	this->m_flCritTokenBucket = oldCritBucket;
	this->m_nCritChecks = oldCritChecks;
	this->m_nCritSeedRequests = oldCritSeedRequests;
	this->m_bCurrentAttackIsCrit = oldIsCritical;
	this->m_bCurrentCritIsRandom = oldIsCriticalRandom;
	this->m_flLastCritCheckTime = oldLastCritCheckTime;
	this->m_flCritTime = oldCritTime;
	this->m_iLastCritCheckFrame = oldLastCritCheckFrame;
	return isCrit;
}

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
		case TF_CLASS_CIVILIAN:
			return "tf_weapon_fireaxe";
		}
	}
	
	/* if not handled: return original entity name, not an empty string */
	return name;
}