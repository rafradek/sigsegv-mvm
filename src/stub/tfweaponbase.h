#ifndef _INCLUDE_SIGSEGV_STUB_TFWEAPONBASE_H_
#define _INCLUDE_SIGSEGV_STUB_TFWEAPONBASE_H_


#include "stub/tfplayer.h"
#include "stub/entities.h"


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
	void CanPerformSecondaryAttack()                                 {        vt_CanPerformSecondaryAttack (this); }
	
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
};

class CTFWeaponBase : public CBaseCombatWeapon, public IHasGenericMeter
{
public:
	CTFPlayer *GetTFPlayerOwner() const { return ToTFPlayer(this->GetOwner()); }
	
	bool IsSilentKiller() { return ft_IsSilentKiller(this); }
	
	int GetWeaponID() const      { return vt_GetWeaponID     (this); }
	int GetPenetrateType() const { return vt_GetPenetrateType(this); }
	
	DECL_SENDPROP(float,                m_flLastFireTime);
	DECL_SENDPROP(float,                m_flEffectBarRegenTime);
	DECL_SENDPROP(float,                m_flEnergy);
	DECL_SENDPROP(CHandle<CTFWearable>, m_hExtraWearable);
	DECL_SENDPROP(CHandle<CTFWearable>, m_hExtraWearableViewModel);
	
private:
	static MemberFuncThunk<CTFWeaponBase *, bool> ft_IsSilentKiller;
	
	static MemberVFuncThunk<const CTFWeaponBase *, int> vt_GetWeaponID;
	static MemberVFuncThunk<const CTFWeaponBase *, int> vt_GetPenetrateType;
};

class CTFWeaponBaseGun : public CTFWeaponBase {};

class CTFPipebombLauncher : public CTFWeaponBaseGun {};

class CTFGrenadeLauncher : public CTFWeaponBaseGun {};

class CTFSpellBook : public CTFWeaponBaseGun {
public:
	DECL_SENDPROP(int, m_iSelectedSpellIndex);
	DECL_SENDPROP(int, m_iSpellCharges);
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
	DECL_SENDPROP(float, m_flChargedDamage);
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
	
private:
	DECL_SENDPROP(CHandle<CBaseEntity>, m_hHealingTarget);
};

class CTFFlameThrower : public CTFWeaponBaseGun
{
public:
	Vector GetVisualMuzzlePos() { return ft_GetMuzzlePosHelper(this, true);  }
	Vector GetFlameOriginPos()  { return ft_GetMuzzlePosHelper(this, false); }
	
private:
	static MemberFuncThunk<CTFFlameThrower *, Vector, bool> ft_GetMuzzlePosHelper;
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


bool WeaponID_IsSniperRifle(int id);
bool WeaponID_IsSniperRifleOrBow(int id);


int GetWeaponId(const char *name);
const char *WeaponIdToAlias(int weapon_id);


#endif
