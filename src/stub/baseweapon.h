#ifndef _INCLUDE_SIGSEGV_STUB_BASEWEAPON_H_
#define _INCLUDE_SIGSEGV_STUB_BASEWEAPON_H_

#include "stub/baseplayer.h"
#include "stub/entities.h"
#include "weapon_parse.h"

typedef struct
{
	int			baseAct;
	int			weaponAct;
	bool		required;
} acttable_t;

class CBaseCombatWeapon : 
#ifdef SE_IS_TF2 
public CEconEntity
#else
public CBaseAnimating
#endif
{
public:
	CBaseCombatCharacter *GetOwner() const { return this->m_hOwner; }
	
	bool IsMeleeWeapon() const { return ft_IsMeleeWeapon(this); }
	void SetOwner(CBaseCombatCharacter *owner) { return ft_SetOwner(this, owner); }
	FileWeaponInfo_t const &GetWpnData() const       { return ft_GetWpnData(this); }
	bool SetIdealActivity(Activity act) const        { return ft_SetIdealActivity(this, act); }
#ifdef SE_IS_TF2
	void SetCustomViewModel(const char *model) {        ft_SetCustomViewModel(this, model); }
#endif
	
	int GetMaxClip1() const                                { return vt_GetMaxClip1  (this); }
	int GetMaxClip2() const                                { return vt_GetMaxClip2  (this); }
	bool HasAmmo()                                         { return vt_HasAmmo      (this); }
	bool HasPrimaryAmmo()                                  { return vt_HasPrimaryAmmo(this); }
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
	int GetPrimaryAmmoType()                               { return vt_GetPrimaryAmmoType (this); }
	void SetSubType(int type)                              {        vt_SetSubType (this, type); }
	void CheckReload()                                     {        vt_CheckReload (this); }
	Activity ActivityOverride(Activity base, bool *required){return vt_ActivityOverride (this, base, required); }
	
	
	DECL_SENDPROP(float, m_flNextPrimaryAttack);
	DECL_SENDPROP(float, m_flNextSecondaryAttack);
	DECL_SENDPROP(float, m_flTimeWeaponIdle);
	DECL_SENDPROP(int,   m_iState);
	DECL_SENDPROP(int,   m_iPrimaryAmmoType);
	DECL_SENDPROP(int,   m_iSecondaryAmmoType);
#ifdef SE_IS_TF2
	DECL_SENDPROP(short,   m_nCustomViewmodelModelIndex);

	DECL_RELATIVE(float,   m_flCritTokenBucket);
	DECL_RELATIVE(int,   m_nCritChecks);
	DECL_RELATIVE(int,   m_nCritSeedRequests);
#endif
	DECL_SENDPROP(int,   m_iClip1);
	DECL_SENDPROP(int,   m_iClip2);
	DECL_SENDPROP(int,   m_iViewModelIndex);
	DECL_SENDPROP(int,   m_nViewModelIndex);
	DECL_SENDPROP(int,   m_iWorldModelIndex);
	DECL_SENDPROP(bool,  m_bFlipViewModel);
	DECL_DATAMAP(bool,   m_bReloadsSingly);
	DECL_DATAMAP(bool,   m_bInReload);
	DECL_DATAMAP(int,    m_nIdealSequence);
	DECL_DATAMAP(Activity,m_IdealActivity);
	
	
private:
	DECL_SENDPROP(CHandle<CBaseCombatCharacter>, m_hOwner);
	
	static MemberFuncThunk<const CBaseCombatWeapon *, bool> ft_IsMeleeWeapon;
	static MemberFuncThunk<CBaseCombatWeapon *, void, CBaseCombatCharacter *> ft_SetOwner;
	static MemberFuncThunk<const CBaseCombatWeapon *, FileWeaponInfo_t const &> ft_GetWpnData;
	static MemberFuncThunk<const CBaseCombatWeapon *, bool, Activity> ft_SetIdealActivity;
#ifdef SE_IS_TF2
	static MemberFuncThunk<CBaseCombatWeapon *, void, const char *> ft_SetCustomViewModel;
#endif
	
	static MemberVFuncThunk<const CBaseCombatWeapon *, int>                          vt_GetMaxClip1;
	static MemberVFuncThunk<const CBaseCombatWeapon *, int>                          vt_GetMaxClip2;
	static MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         vt_HasAmmo;
	static MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         vt_HasPrimaryAmmo;
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
	static MemberVFuncThunk<      CBaseCombatWeapon *, int>                          vt_GetPrimaryAmmoType;
	static MemberVFuncThunk<      CBaseCombatWeapon *, void, int>                    vt_SetSubType;
	static MemberVFuncThunk<      CBaseCombatWeapon *, void>                         vt_CheckReload;
	static MemberVFuncThunk<      CBaseCombatWeapon *, Activity, Activity, bool *>   vt_ActivityOverride;
	
};

class CBaseViewModel : public CBaseAnimating
{
public:
	CBaseCombatWeapon *GetWeapon() const { return this->m_hWeapon; }
	void SetWeapon(CBaseCombatWeapon *weapon) { this->m_hWeapon = weapon; }
	CBaseEntity *GetOwner() const { return this->m_hOwner; }
	void SetOwner(CBaseEntity *owner) { this->m_hOwner = owner; }

	void SetControlPanelsActive(bool enable) { ft_SetControlPanelsActive(this, enable); }
	void SpawnControlPanels()                { ft_SpawnControlPanels(this); }
	void DestroyControlPanels()              { ft_DestroyControlPanels(this); }

	void SetWeaponModel(const char *model, CBaseCombatWeapon *weapon) { return vt_SetWeaponModel(this, model, weapon); }
	
private:
	DECL_SENDPROP(int,                        m_nViewModelIndex);
	DECL_SENDPROP(CHandle<CBaseEntity>,       m_hOwner);
	DECL_SENDPROP(CHandle<CBaseCombatWeapon>, m_hWeapon);

	static MemberFuncThunk<CBaseViewModel *, void, bool> ft_SetControlPanelsActive;
	static MemberFuncThunk<CBaseViewModel *, void> ft_SpawnControlPanels;
	static MemberFuncThunk<CBaseViewModel *, void> ft_DestroyControlPanels;

	static MemberVFuncThunk<CBaseViewModel *, void, const char *, CBaseCombatWeapon *> vt_SetWeaponModel;
};

#endif