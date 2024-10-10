#ifndef _INCLUDE_SIGSEGV_STUB_TFWEAPONBASE_H_
#define _INCLUDE_SIGSEGV_STUB_TFWEAPONBASE_H_

#include "stub/baseweapon.h"
#include "stub/tfplayer.h"
#include "stub/tfentities.h"

class CTFWeaponInfo : public FileWeaponInfo_t { 
public:
	int m_nDamage;
	int m_nBulletsPerShot;
	float m_flRange;
	float m_flSpread;
	float m_flPunchAngle;
	float m_flTimeFireDelay;
	float m_flTimeIdle;
	float m_flTimeIdleEmpty;
	float m_flTimeReloadStart;
	float m_flTimeReload;
};

class CTFWeaponBase : public CBaseCombatWeapon, public IHasGenericMeter
{
public:
	CTFPlayer *GetTFPlayerOwner() const { return ft_GetTFPlayerOwner(this); }
	
	bool IsSilentKiller() { return ft_IsSilentKiller(this); }
	float Energy_GetMaxEnergy() { return ft_Energy_GetMaxEnergy(this); }
	void CalcIsAttackCritical() { ft_CalcIsAttackCritical(this); }
	bool CalcIsAttackCriticalPoll(bool checkRandom);
	CTFWeaponInfo const& GetTFWpnData() const { return ft_GetTFWeaponData(this); }
	void StartEffectBarRegen()   { ft_StartEffectBarRegen(this); }
	bool DeflectProjectiles()    { return ft_DeflectProjectiles(this); }
	
	int GetWeaponID() const                  { return vt_GetWeaponID     (this); }
	int GetPenetrateType() const             { return vt_GetPenetrateType(this); }
	void GetProjectileFireSetup(CTFPlayer *player, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammaates, float flEndDist) {        vt_GetProjectileFireSetup (this, player, vecOffset, vecSrc, angForward, bHitTeammaates, flEndDist); }
	bool ShouldRemoveInvisibilityOnPrimaryAttack() const { return vt_ShouldRemoveInvisibilityOnPrimaryAttack(this); }
	bool IsEnergyWeapon() const              { return vt_IsEnergyWeapon(this); }
	float Energy_GetShotCost() const         { return vt_Energy_GetShotCost(this); }
	void Misfire()                           {        vt_Misfire(this); }
	Vector GetParticleColor(int color)       { return vt_GetParticleColor(this, color); }
	int GetMaxHealthMod()                    { return vt_GetMaxHealthMod(this); }
	float GetAfterburnRateOnHit()            { return vt_GetAfterburnRateOnHit(this); }
	float InternalGetEffectBarRechargeTime() { return vt_InternalGetEffectBarRechargeTime(this); }
	float GetEffectBarProgress()             { return vt_GetEffectBarProgress(this); }
	void ApplyOnHitAttributes(CBaseEntity *pVictimBaseEntity, CTFPlayer *pAttacker, const CTakeDamageInfo &info) { return vt_ApplyOnHitAttributes(this, pVictimBaseEntity, pAttacker, info); }
	int GetSkin()                            { return vt_GetSkin(this); }
	

	DECL_SENDPROP(float,                m_flLastFireTime);
	DECL_SENDPROP(float,                m_flEffectBarRegenTime);
	DECL_SENDPROP(float,                m_flEnergy);
	DECL_SENDPROP(CHandle<CTFWearable>, m_hExtraWearable);
	DECL_SENDPROP(CHandle<CTFWearable>, m_hExtraWearableViewModel);
	DECL_SENDPROP(bool, m_bBeingRepurposedForTaunt);
	DECL_SENDPROP(float, m_flReloadPriorNextFire);
	DECL_SENDPROP(bool,  m_bDisguiseWeapon);
	DECL_SENDPROP(int,   m_iReloadMode);
	DECL_RELATIVE(bool,  m_bCurrentAttackIsCrit);
	DECL_RELATIVE(bool,  m_bCurrentCritIsRandom);
	DECL_SENDPROP(float, m_flLastCritCheckTime);
	DECL_RELATIVE(float, m_flCritTime);
	DECL_RELATIVE(int,   m_iLastCritCheckFrame);
	

	
private:
	static MemberFuncThunk<const CTFWeaponBase *, CTFPlayer *> ft_GetTFPlayerOwner;
	static MemberFuncThunk<CTFWeaponBase *, bool> ft_IsSilentKiller;
	static MemberFuncThunk<CTFWeaponBase *, float> ft_Energy_GetMaxEnergy;
	static MemberFuncThunk<CTFWeaponBase *, void> ft_CalcIsAttackCritical;
	static MemberFuncThunk<const CTFWeaponBase *, CTFWeaponInfo const &> ft_GetTFWeaponData;
	static MemberFuncThunk<CTFWeaponBase *, void> ft_StartEffectBarRegen;
	static MemberFuncThunk<CTFWeaponBase *, bool> ft_DeflectProjectiles;
	
	static MemberVFuncThunk<const CTFWeaponBase *, int> vt_GetWeaponID;
	static MemberVFuncThunk<const CTFWeaponBase *, int> vt_GetPenetrateType;
	static MemberVFuncThunk<CTFWeaponBase *, void, CTFPlayer *, Vector , Vector *, QAngle *, bool , float >   vt_GetProjectileFireSetup;
	static MemberVFuncThunk<const CTFWeaponBase *, bool> vt_ShouldRemoveInvisibilityOnPrimaryAttack;
	static MemberVFuncThunk<const CTFWeaponBase *, bool> vt_IsEnergyWeapon;
	static MemberVFuncThunk<const CTFWeaponBase *, float> vt_Energy_GetShotCost;
	static MemberVFuncThunk<CTFWeaponBase *, void> vt_Misfire;
	static MemberVFuncThunk<CTFWeaponBase *, Vector, int> vt_GetParticleColor;
	static MemberVFuncThunk<CTFWeaponBase *, int> vt_GetMaxHealthMod;
	static MemberVFuncThunk<CTFWeaponBase *, float> vt_GetAfterburnRateOnHit;
	static MemberVFuncThunk<CTFWeaponBase *, float> vt_InternalGetEffectBarRechargeTime;
	static MemberVFuncThunk<CTFWeaponBase *, float> vt_GetEffectBarProgress;
	static MemberVFuncThunk<CTFWeaponBase *, void, CBaseEntity *, CTFPlayer *, const CTakeDamageInfo &> vt_ApplyOnHitAttributes;
	static MemberVFuncThunk<CTFWeaponBase *, int> vt_GetSkin;
};

class CTFWeaponBaseGun : public CTFWeaponBase {
public:
	void UpdatePunchAngles(CTFPlayer *pPlayer)                 {        ft_UpdatePunchAngles(this, pPlayer); }

	float GetProjectileGravity() {return vt_GetProjectileGravity(this);}
	float GetProjectileSpeed()  {return vt_GetProjectileSpeed(this);}
	int GetWeaponProjectileType() const {return vt_GetWeaponProjectileType(this);}
	float GetProjectileDamage() { return vt_GetProjectileDamage(this); }
	void ModifyProjectile(CBaseAnimating * anim) { return vt_ModifyProjectile(this, anim); }
	void RemoveProjectileAmmo(CTFPlayer *pPlayer) {        vt_RemoveProjectileAmmo(this, pPlayer); }
	void DoFireEffects()                          {        vt_DoFireEffects       (this); }
	bool ShouldPlayFireAnim()                     { return vt_ShouldPlayFireAnim  (this); }
	CBaseEntity *FireProjectile(CTFPlayer *pPlayer) { return vt_FireProjectile  (this, pPlayer); }
	int GetAmmoPerShot()                          { return vt_GetAmmoPerShot(this); }
	
private:
	static MemberFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> ft_UpdatePunchAngles;

	static MemberVFuncThunk<CTFWeaponBaseGun *, float> vt_GetProjectileGravity;
	static MemberVFuncThunk<CTFWeaponBaseGun *, float> vt_GetProjectileSpeed;
	static MemberVFuncThunk<const CTFWeaponBaseGun *, int> vt_GetWeaponProjectileType;
	static MemberVFuncThunk<CTFWeaponBaseGun *, float> vt_GetProjectileDamage;
	static MemberVFuncThunk<CTFWeaponBaseGun *, void, CBaseAnimating *> vt_ModifyProjectile;
	static MemberVFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> vt_RemoveProjectileAmmo;
	static MemberVFuncThunk<CTFWeaponBaseGun *, void>              vt_DoFireEffects;
	static MemberVFuncThunk<CTFWeaponBaseGun *, bool>              vt_ShouldPlayFireAnim;
	static MemberVFuncThunk<CTFWeaponBaseGun *, CBaseEntity *,CTFPlayer *> vt_FireProjectile;
	static MemberVFuncThunk<CTFWeaponBaseGun *, int >              vt_GetAmmoPerShot;
};

class CTFPipebombLauncher : public CTFWeaponBaseGun {
public:
	DECL_SENDPROP(int, m_iPipebombCount);
	DECL_RELATIVE(CUtlVector<EHANDLE>, m_Pipebombs);
};

class CTFGrenadeLauncher : public CTFWeaponBaseGun {};

class CTFSpellBook : public CTFWeaponBaseGun {
public:
	void RollNewSpell(int tier) { ft_RollNewSpell(this, tier); }
	
public:
	DECL_SENDPROP(int, m_iSelectedSpellIndex);
	DECL_SENDPROP(int, m_iSpellCharges);
private:
	static MemberFuncThunk<CTFSpellBook *, void, int> ft_RollNewSpell;
};

class CTFCompoundBow : public CTFPipebombLauncher
{
public:
	/* these 4 vfuncs really ought to be in a separate ITFChargeUpWeapon stub
	 * class, but reliably determining these vtable indexes at runtime is hard,
	 * plus all calls would have to do an rtti_cast from the derived type to
	 * ITFChargeUpWeapon before calling the thunk; incidentally, this means that
	 * ITFChargeUpWeapon would need to be a template class with a parameter
	 * telling it what the derived class is, so that it knows what source ptr
	 * type to pass to rtti_cast... what a mess */
//	bool CanCharge()           { return vt_CanCharge         (this); }
//	float GetChargeBeginTime() { return vt_GetChargeBeginTime(this); }
	float GetChargeMaxTime()   { return vt_GetChargeMaxTime  (this); }
	float GetCurrentCharge()   { return vt_GetCurrentCharge  (this); }

	DECL_SENDPROP(bool, m_bArrowAlight);
	
private:
//	static MemberVFuncThunk<CTFCompoundBow *, bool>  vt_CanCharge;
//	static MemberVFuncThunk<CTFCompoundBow *, float> vt_GetChargeBeginTime;
	static MemberVFuncThunk<CTFCompoundBow *, float> vt_GetChargeMaxTime;
	static MemberVFuncThunk<CTFCompoundBow *, float> vt_GetCurrentCharge;
};

class CTFMinigun : public CTFWeaponBaseGun
{
public:
	enum MinigunState_t : int32_t
	{
		AC_STATE_IDLE        = 0,
		AC_STATE_STARTFIRING = 1,
		AC_STATE_FIRING      = 2,
		AC_STATE_SPINNING    = 3,
		AC_STATE_DRYFIRE     = 4,
	};
	
	DECL_SENDPROP(MinigunState_t, m_iWeaponState);
	DECL_SENDPROP(bool, m_bCritShot);
};

class CTFSniperRifle : public CTFWeaponBaseGun
{
public:
	void ExplosiveHeadShot(CTFPlayer *attacker, CTFPlayer *victim) { ft_ExplosiveHeadShot(this, attacker, victim); }
	void ApplyChargeSpeedModifications(float &value)               { ft_ApplyChargeSpeedModifications(this, value); }

	float SniperRifleChargeRateMod() { return vt_SniperRifleChargeRateMod(this); }
	
	DECL_SENDPROP(float, m_flChargedDamage);

private:
	static MemberFuncThunk<CTFSniperRifle *, void, CTFPlayer *, CTFPlayer *> ft_ExplosiveHeadShot;
	static MemberFuncThunk<CTFSniperRifle *, void, float &> ft_ApplyChargeSpeedModifications;

	static MemberVFuncThunk<CTFSniperRifle *, float> vt_SniperRifleChargeRateMod;
};

class CTFSniperRifleClassic : public CTFSniperRifle {};

class CTFSniperRifleDecap : public CTFSniperRifle
{
public:
	int GetCount() { return ft_GetCount(this); }
	
private:
	static MemberFuncThunk<CTFSniperRifleDecap *, int> ft_GetCount;
};

class CTFSMG : public CTFWeaponBaseGun {};
class CTFChargedSMG : public CTFSMG {
public:
	DECL_SENDPROP(float, m_flMinicritCharge);
};

class CTFRocketLauncher : public CTFWeaponBaseGun {};
class CTFRocketLauncher_AirStrike : public CTFRocketLauncher {};
class CTFParticleCannon : public CTFRocketLauncher 
{
public:
	DECL_SENDPROP(float, m_flChargeBeginTime);
};

class CTFGrapplingHook : public CTFRocketLauncher 
{
public:
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hProjectile);
};

class CTFWeaponBaseMelee : public CTFWeaponBase
{
public:

	DECL_EXTRACT(float, m_flSmackTime);
	int GetSwingRange()            { return vt_GetSwingRange(this); }
	bool DoSwingTrace(trace_t& tr) { return vt_DoSwingTrace (this, tr); }
	
private:
	static MemberVFuncThunk<CTFWeaponBaseMelee *, int>            vt_GetSwingRange;
	static MemberVFuncThunk<CTFWeaponBaseMelee *, bool, trace_t&> vt_DoSwingTrace;
};

class CTFKnife : public CTFWeaponBaseMelee
{
public:
	bool CanPerformBackstabAgainstTarget(CTFPlayer *player) { return ft_CanPerformBackstabAgainstTarget(this, player); }
	bool IsBehindAndFacingTarget(CTFPlayer *player)         { return ft_IsBehindAndFacingTarget        (this, player); }
	
private:
	static MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> ft_CanPerformBackstabAgainstTarget;
	static MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> ft_IsBehindAndFacingTarget;
};

class CTFBottle : public CTFWeaponBaseMelee
{
public:
	DECL_SENDPROP(bool, m_bBroken);
};

class CTFBonesaw : public CTFWeaponBaseMelee {};

class CTFWrench : public CTFWeaponBaseMelee {};

class CTFRobotArm : public CTFWrench
{
public:
	/* this is a hacky mess for now */
	
	int GetPunchNumber() const            { return *reinterpret_cast<int   *>((uintptr_t)&this->m_hRobotArm + 0x04); }
	float GetLastPunchTime() const        { return *reinterpret_cast<float *>((uintptr_t)&this->m_hRobotArm + 0x08); }
	bool ShouldInflictComboDamage() const { return *reinterpret_cast<bool  *>((uintptr_t)&this->m_hRobotArm + 0x0c); }
	bool ShouldImpartMaxForce() const     { return *reinterpret_cast<bool  *>((uintptr_t)&this->m_hRobotArm + 0x0d); }
	
	// 20151007a:
	// CTFRobotArm +0x800 CHandle<CTFWearableRobotArm> m_hRobotArm
	// CTFRobotArm +0x804 int                          m_iPunchNumber
	// CTFRobotArm +0x808 float                        m_flTimeLastPunch
	// CTFRobotArm +0x80c bool                         m_bComboPunch
	// CTFRobotArm +0x80d bool                         m_bMaxForce
	
private:
	DECL_SENDPROP(CHandle<CTFWearableRobotArm>, m_hRobotArm);
};

class CTFBuffItem : public CTFWeaponBaseMelee {};

class CTFDecapitationMeleeWeaponBase : public CTFWeaponBaseMelee {};

class CTFSword : public CTFDecapitationMeleeWeaponBase
{
public:
	float GetSwordSpeedMod() { return ft_GetSwordSpeedMod(this); }
	int GetSwordHealthMod() { return ft_GetSwordHealthMod(this); }
	
private:
	static MemberFuncThunk<CTFSword *, float> ft_GetSwordSpeedMod;
	static MemberFuncThunk<CTFSword *, int> ft_GetSwordHealthMod;
};

class CTFLunchBox : public CTFWeaponBase {};
class CTFLunchBox_Drink : public CTFLunchBox {};

class CTFJar : public CTFWeaponBaseGun {};

class CTFShotgun : public CTFWeaponBaseGun {};
class CTFScatterGun : public CTFShotgun {};
class CTFSodaPopper : public CTFScatterGun {};
class CTFPEPBrawlerBlaster : public CTFScatterGun {};

class CWeaponMedigun : public CTFWeaponBase
{
public:
	CBaseEntity *GetHealTarget() const { return this->m_hHealingTarget; }
	void SetHealTarget(CBaseEntity *target) { this->m_hHealingTarget = target; }
	float GetHealRate() { return vt_GetHealRate(this); }
	float GetCharge() const { return this->m_flChargeLevel; }
	void SetCharge(float charge) { this->m_flChargeLevel = charge; }
	bool IsChargeReleased() const { return this->m_bChargeRelease; }
	
private:
	static MemberVFuncThunk<CWeaponMedigun *, float> vt_GetHealRate;
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hHealingTarget);
	DECL_SENDPROP(float, m_flChargeLevel);
	DECL_SENDPROP(bool, m_bChargeRelease);
	
};

class CTFFlameThrower : public CTFWeaponBaseGun
{
public:
	Vector GetVisualMuzzlePos() { return ft_GetMuzzlePosHelper(this, true);  }
	Vector GetFlameOriginPos()  { return ft_GetMuzzlePosHelper(this, false); }
	float GetDeflectionRadius()  { return ft_GetDeflectionRadius(this); }
	
	DECL_SENDPROP(int, m_iWeaponState);
private:
	static MemberFuncThunk<CTFFlameThrower *, Vector, bool> ft_GetMuzzlePosHelper;
	static MemberFuncThunk<CTFFlameThrower *, float> ft_GetDeflectionRadius;
};

class CTFWeaponBuilder : public CTFWeaponBase {
public:
	DECL_SENDPROP(int, m_iObjectType);
	DECL_SENDPROP(int, m_iObjectMode);
	DECL_SENDPROP_RW(bool[4], m_aBuildableObjectTypes);
};
class CTFWeaponSapper : public CTFWeaponBuilder {};

class CTFWeaponInvis : public CTFWeaponBase {};

class CTFViewModel : public CBaseViewModel {};

inline CBaseCombatWeapon *ToBaseCombatWeapon(CBaseEntity *pEntity)
{
	if (pEntity == nullptr)   return nullptr;
	
	return pEntity->MyCombatWeaponPointer();
}

bool WeaponID_IsSniperRifle(int id);
bool WeaponID_IsSniperRifleOrBow(int id);


int GetWeaponId(const char *name);
const char *WeaponIdToAlias(int weapon_id);

float CalculateProjectileSpeed(CTFWeaponBaseGun *weapon);

inline CEconEntity *GetEconEntityAtLoadoutSlot(CTFPlayer *player, int slot) {
	if (slot < 0 || player == nullptr) return nullptr;
	return (CEconEntity *)player->GetEntityForLoadoutSlot(slot);
}

const char *TranslateWeaponEntForClass_improved(const char *name, int classnum);

#endif
