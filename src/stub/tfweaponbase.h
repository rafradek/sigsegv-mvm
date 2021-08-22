#ifndef _INCLUDE_SIGSEGV_STUB_TFWEAPONBASE_H_
#define _INCLUDE_SIGSEGV_STUB_TFWEAPONBASE_H_


#include "stub/tfplayer.h"
#include "stub/entities.h"

typedef enum {
	EMPTY,
	SINGLE,
	SINGLE_NPC,
	WPN_DOUBLE,
	DOUBLE_NPC,
	BURST,
	RELOAD,
	RELOAD_NPC,
	MELEE_MISS,
	MELEE_HIT,
	MELEE_HIT_WORLD,
	SPECIAL1,
	SPECIAL2,
	SPECIAL3,
	TAUNT,
	DEPLOY,
} WeaponSound_t;

class CBaseCombatWeapon : public CEconEntity
{
public:
	CBaseCombatCharacter *GetOwner() const { return this->m_hOwner; }
	
	bool IsMeleeWeapon() const { return ft_IsMeleeWeapon(this); }
	
	int GetMaxClip1() const                                { return vt_GetMaxClip1  (this); }
	int GetMaxClip2() const                                { return vt_GetMaxClip2  (this); }
	bool HasAmmo()                                         { return vt_HasAmmo      (this); }
	void Equip(CBaseCombatCharacter *pOwner)               {        vt_Equip        (this, pOwner); }
	void Drop(const Vector& vecVelocity)                   {        vt_Drop         (this, vecVelocity); }
	const char *GetViewModel(int viewmodelindex = 0) const { return vt_GetViewModel (this, viewmodelindex); }
	const char *GetWorldModel() const                      { return vt_GetWorldModel(this); }
	void SetViewModel()                                    {        vt_SetViewModel (this); }
	void PrimaryAttack()                                   {        vt_PrimaryAttack (this); }
	void SecondaryAttack()                                 {        vt_SecondaryAttack (this); }
	//void CanPerformPrimaryAttack()                                   {        vt_CanPerformPrimaryAttack (this); }
	void CanPerformSecondaryAttack()                       {        vt_CanPerformSecondaryAttack (this); }
	char const *GetShootSound(int type)                    { return vt_GetShootSound (this, type); }
	
	
	DECL_SENDPROP(float, m_flNextPrimaryAttack);
	DECL_SENDPROP(float, m_flNextSecondaryAttack);
	DECL_SENDPROP(float, m_flTimeWeaponIdle);
	DECL_SENDPROP(int,   m_iState);
	DECL_SENDPROP(int,   m_iPrimaryAmmoType);
	DECL_SENDPROP(int,   m_iSecondaryAmmoType);
	DECL_SENDPROP(int,   m_iClip1);
	DECL_SENDPROP(int,   m_iClip2);
	DECL_SENDPROP(int,   m_iViewModelIndex);
	DECL_SENDPROP(int,   m_iWorldModelIndex);
	
private:
	DECL_SENDPROP(CHandle<CBaseCombatCharacter>, m_hOwner);
	
	static MemberFuncThunk<const CBaseCombatWeapon *, bool> ft_IsMeleeWeapon;
	
	static MemberVFuncThunk<const CBaseCombatWeapon *, int>                          vt_GetMaxClip1;
	static MemberVFuncThunk<const CBaseCombatWeapon *, int>                          vt_GetMaxClip2;
	static MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         vt_HasAmmo;
	static MemberVFuncThunk<      CBaseCombatWeapon *, void, CBaseCombatCharacter *> vt_Equip;
	static MemberVFuncThunk<      CBaseCombatWeapon *, void, const Vector&>          vt_Drop;
	static MemberVFuncThunk<const CBaseCombatWeapon *, const char *, int>            vt_GetViewModel;
	static MemberVFuncThunk<const CBaseCombatWeapon *, const char *>                 vt_GetWorldModel;
	static MemberVFuncThunk<      CBaseCombatWeapon *, void>                         vt_SetViewModel;
	static MemberVFuncThunk<      CBaseCombatWeapon *, void>                         vt_PrimaryAttack;
	static MemberVFuncThunk<      CBaseCombatWeapon *, void>                         vt_SecondaryAttack;
	//static MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         vt_CanPerformPrimaryAttack;
	static MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         vt_CanPerformSecondaryAttack;
	static MemberVFuncThunk<      CBaseCombatWeapon *, char const *, int>            vt_GetShootSound;
};

class CTFWeaponBase : public CBaseCombatWeapon, public IHasGenericMeter
{
public:
	CTFPlayer *GetTFPlayerOwner() const { return ft_GetTFPlayerOwner(this); }
	
	bool IsSilentKiller() { return ft_IsSilentKiller(this); }
	
	int GetWeaponID() const      { return vt_GetWeaponID     (this); }
	int GetPenetrateType() const { return vt_GetPenetrateType(this); }
	void GetProjectileFireSetup(CTFPlayer *player, Vector vecOffset, Vector *vecSrc, QAngle *angForward, bool bHitTeammaates, float flEndDist) {        vt_GetProjectileFireSetup (this, player, vecOffset, vecSrc, angForward, bHitTeammaates, flEndDist); }
	bool ShouldRemoveInvisibilityOnPrimaryAttack() const { return vt_ShouldRemoveInvisibilityOnPrimaryAttack(this); }

	DECL_SENDPROP(float,                m_flLastFireTime);
	DECL_SENDPROP(float,                m_flEffectBarRegenTime);
	DECL_SENDPROP(float,                m_flEnergy);
	DECL_SENDPROP(CHandle<CTFWearable>, m_hExtraWearable);
	DECL_SENDPROP(CHandle<CTFWearable>, m_hExtraWearableViewModel);
	DECL_SENDPROP(bool, m_bBeingRepurposedForTaunt);
	

	
private:
	static MemberFuncThunk<const CTFWeaponBase *, CTFPlayer *> ft_GetTFPlayerOwner;
	static MemberFuncThunk<CTFWeaponBase *, bool> ft_IsSilentKiller;
	
	static MemberVFuncThunk<const CTFWeaponBase *, int> vt_GetWeaponID;
	static MemberVFuncThunk<const CTFWeaponBase *, int> vt_GetPenetrateType;
	static MemberVFuncThunk<CTFWeaponBase *, void, CTFPlayer *, Vector , Vector *, QAngle *, bool , float >   vt_GetProjectileFireSetup;
	static MemberVFuncThunk<const CTFWeaponBase *, bool> vt_ShouldRemoveInvisibilityOnPrimaryAttack;
};

class CTFWeaponBaseGun : public CTFWeaponBase {
public:
	void UpdatePunchAngles(CTFPlayer *pPlayer) { ft_UpdatePunchAngles(this, pPlayer); }
	float GetProjectileGravity() {return vt_GetProjectileGravity(this);}
	float GetProjectileSpeed()  {return vt_GetProjectileSpeed(this);}
	int GetWeaponProjectileType() const {return vt_GetWeaponProjectileType(this);}
	float GetProjectileDamage() { return vt_GetProjectileDamage(this); }
	void ModifyProjectile(CBaseAnimating * anim) { return vt_ModifyProjectile(this, anim); }
	void RemoveProjectileAmmo(CTFPlayer *pPlayer) {        vt_RemoveProjectileAmmo(this, pPlayer); }
	void DoFireEffects()                          {        vt_DoFireEffects       (this); }
	bool ShouldPlayFireAnim()                     { return vt_ShouldPlayFireAnim  (this); }
private:
	static MemberVFuncThunk<CTFWeaponBaseGun *, float> vt_GetProjectileGravity;
	static MemberVFuncThunk<CTFWeaponBaseGun *, float> vt_GetProjectileSpeed;
	static MemberVFuncThunk<const CTFWeaponBaseGun *, int> vt_GetWeaponProjectileType;
	static MemberVFuncThunk<CTFWeaponBaseGun *, float> vt_GetProjectileDamage;
	static MemberVFuncThunk<CTFWeaponBaseGun *, void, CBaseAnimating *> vt_ModifyProjectile;
	static MemberFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> ft_UpdatePunchAngles;
	
	static MemberVFuncThunk<CTFWeaponBaseGun *, void, CTFPlayer *> vt_RemoveProjectileAmmo;
	static MemberVFuncThunk<CTFWeaponBaseGun *, void>              vt_DoFireEffects;
	static MemberVFuncThunk<CTFWeaponBaseGun *, bool>              vt_ShouldPlayFireAnim;
};

class CTFPipebombLauncher : public CTFWeaponBaseGun {};

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

class CTFLunchBox : public CTFWeaponBase {};
class CTFLunchBox_Drink : public CTFLunchBox {};

class CWeaponMedigun : public CTFWeaponBase
{
public:
	CBaseEntity *GetHealTarget() const { return this->m_hHealingTarget; }
	float GetHealRate() { return vt_GetHealRate(this); }
	float GetCharge() const { return this->m_flChargeLevel; }
	void SetCharge(float charge) { this->m_flChargeLevel = charge; }
	
private:
	static MemberVFuncThunk<CWeaponMedigun *, float> vt_GetHealRate;
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hHealingTarget);
	DECL_SENDPROP(float, m_flChargeLevel);
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

class CTFWeaponBuilder : public CTFWeaponBase {};
class CTFWeaponSapper : public CTFWeaponBuilder {};

class CTFWeaponInvis : public CTFWeaponBase {};


class CBaseViewModel : public CBaseAnimating
{
public:
	CBaseCombatWeapon *GetWeapon() const { return this->m_hWeapon; }
	
private:
	DECL_SENDPROP(int,                        m_nViewModelIndex);
	DECL_SENDPROP(CHandle<CBaseEntity>,       m_hOwner);
	DECL_SENDPROP(CHandle<CBaseCombatWeapon>, m_hWeapon);
};

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
	if (slot < 0)
		return nullptr;
	CEconEntity *item = player->Weapon_GetSlot(slot); 
	if (item == nullptr)
		return player->GetEquippedWearableForLoadoutSlot(slot);
	else
		return item;
}

const char *TranslateWeaponEntForClass_improved(const char *name, int classnum);
#endif
