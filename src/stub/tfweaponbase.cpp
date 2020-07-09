#include "stub/tfweaponbase.h"


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

MemberFuncThunk<CTFWeaponBase *, bool> CTFWeaponBase::ft_IsSilentKiller("CTFWeaponBase::IsSilentKiller");

MemberVFuncThunk<const CTFWeaponBase *, int> CTFWeaponBase::vt_GetWeaponID(     TypeName<CTFBonesaw>(),     "CTFBonesaw::GetWeaponID");
MemberVFuncThunk<const CTFWeaponBase *, int> CTFWeaponBase::vt_GetPenetrateType(TypeName<CTFSniperRifle>(), "CTFSniperRifle::GetPenetrateType");


//MemberVFuncThunk<CTFCompoundBow *, bool>  CTFCompoundBow::vt_CanCharge(         TypeName<CTFCompoundBow>(), "CTFCompoundBow::CanCharge");
//MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetChargeBeginTime(TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetChargeBeginTime");
MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetChargeMaxTime(  TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetChargeMaxTime");
MemberVFuncThunk<CTFCompoundBow *, float> CTFCompoundBow::vt_GetCurrentCharge(  TypeName<CTFCompoundBow>(), "CTFCompoundBow::GetCurrentCharge");


IMPL_SENDPROP(CTFMinigun::MinigunState_t, CTFMinigun, m_iWeaponState, CTFMinigun);


IMPL_SENDPROP(float, CTFSniperRifle, m_flChargedDamage, CTFSniperRifle);

IMPL_SENDPROP(int, CTFSpellBook, m_iSelectedSpellIndex, CTFSpellBook);
IMPL_SENDPROP(int, CTFSpellBook, m_iSpellCharges, CTFSpellBook);

MemberFuncThunk<CTFSniperRifleDecap *, int> CTFSniperRifleDecap::ft_GetCount("CTFSniperRifleDecap::GetCount");


MemberVFuncThunk<CTFWeaponBaseMelee *, int>            CTFWeaponBaseMelee::vt_GetSwingRange(TypeName<CTFWeaponBaseMelee>(), "CTFWeaponBaseMelee::GetSwingRange");
MemberVFuncThunk<CTFWeaponBaseMelee *, bool, trace_t&> CTFWeaponBaseMelee::vt_DoSwingTrace (TypeName<CTFWeaponBaseMelee>(), "CTFWeaponBaseMelee::DoSwingTrace");


MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> CTFKnife::ft_CanPerformBackstabAgainstTarget("CTFKnife::CanPerformBackstabAgainstTarget");
MemberFuncThunk<CTFKnife *, bool, CTFPlayer *> CTFKnife::ft_IsBehindAndFacingTarget        ("CTFKnife::IsBehindAndFacingTarget");


IMPL_SENDPROP(bool, CTFBottle, m_bBroken, CTFBottle);


IMPL_SENDPROP(CHandle<CTFWearableRobotArm>, CTFRobotArm, m_hRobotArm, CTFRobotArm);


IMPL_SENDPROP(CHandle<CBaseEntity>, CWeaponMedigun, m_hHealingTarget, CWeaponMedigun);


MemberFuncThunk<CTFFlameThrower *, Vector, bool> CTFFlameThrower::ft_GetMuzzlePosHelper("CTFFlameThrower::GetMuzzlePosHelper");


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
