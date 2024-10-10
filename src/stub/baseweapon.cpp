#include "baseweapon.h"

IMPL_SENDPROP(float,                         CBaseCombatWeapon, m_flNextPrimaryAttack,   CBaseCombatWeapon);
IMPL_SENDPROP(float,                         CBaseCombatWeapon, m_flNextSecondaryAttack, CBaseCombatWeapon);
IMPL_SENDPROP(float,                         CBaseCombatWeapon, m_flTimeWeaponIdle,      CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iState,                CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iPrimaryAmmoType,      CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iSecondaryAmmoType,    CBaseCombatWeapon);
#ifdef SE_IS_TF2
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_nCustomViewmodelModelIndex, CBaseCombatWeapon);

IMPL_REL_BEFORE(float,                       CBaseCombatWeapon, m_flCritTokenBucket, m_nCritChecks, 0);
IMPL_REL_BEFORE(int,                         CBaseCombatWeapon, m_nCritChecks, m_nCritSeedRequests, 0);
IMPL_REL_BEFORE(int,                         CBaseCombatWeapon, m_nCritSeedRequests, m_nViewModelIndex, 0);
#endif
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iClip1,                CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iClip2,                CBaseCombatWeapon);
IMPL_SENDPROP(CHandle<CBaseCombatCharacter>, CBaseCombatWeapon, m_hOwner,                CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iViewModelIndex,       CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_nViewModelIndex,       CBaseCombatWeapon);
IMPL_SENDPROP(bool,                          CBaseCombatWeapon, m_bFlipViewModel,        CBaseCombatWeapon);
IMPL_SENDPROP(int,                           CBaseCombatWeapon, m_iWorldModelIndex,      CBaseCombatWeapon);
IMPL_DATAMAP(bool,                           CBaseCombatWeapon, m_bReloadsSingly);
IMPL_DATAMAP(bool,                           CBaseCombatWeapon, m_bInReload);
IMPL_DATAMAP(int,                            CBaseCombatWeapon, m_nIdealSequence);
IMPL_DATAMAP(Activity,                       CBaseCombatWeapon, m_IdealActivity);


MemberFuncThunk<const CBaseCombatWeapon *, bool> CBaseCombatWeapon::ft_IsMeleeWeapon("CBaseCombatWeapon::IsMeleeWeapon");
MemberFuncThunk<CBaseCombatWeapon *, void, CBaseCombatCharacter *> CBaseCombatWeapon::ft_SetOwner("CBaseCombatWeapon::SetOwner");
MemberFuncThunk<const CBaseCombatWeapon *, FileWeaponInfo_t const &> CBaseCombatWeapon::ft_GetWpnData("CBaseCombatWeapon::GetWpnData");
MemberFuncThunk<const CBaseCombatWeapon *, bool, Activity> CBaseCombatWeapon::ft_SetIdealActivity("CBaseCombatWeapon::SetIdealActivity");
#ifdef SE_IS_TF2
MemberFuncThunk<CBaseCombatWeapon *, void, const char *> CBaseCombatWeapon::ft_SetCustomViewModel("CBaseCombatWeapon::SetCustomViewModel");
#endif

MemberVFuncThunk<const CBaseCombatWeapon *, int>                          CBaseCombatWeapon::vt_GetMaxClip1  (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetMaxClip1");
MemberVFuncThunk<const CBaseCombatWeapon *, int>                          CBaseCombatWeapon::vt_GetMaxClip2  (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetMaxClip2");
MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         CBaseCombatWeapon::vt_HasAmmo      (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::HasAmmo");
MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         CBaseCombatWeapon::vt_HasPrimaryAmmo(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::HasPrimaryAmmo");
MemberVFuncThunk<      CBaseCombatWeapon *, void, CBaseCombatCharacter *> CBaseCombatWeapon::vt_Equip        (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::Equip");
MemberVFuncThunk<      CBaseCombatWeapon *, void, const Vector&>          CBaseCombatWeapon::vt_Drop         (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::Drop");
MemberVFuncThunk<const CBaseCombatWeapon *, const char *, int>            CBaseCombatWeapon::vt_GetViewModel (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetViewModel");
MemberVFuncThunk<const CBaseCombatWeapon *, const char *>                 CBaseCombatWeapon::vt_GetWorldModel(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetWorldModel");
MemberVFuncThunk<      CBaseCombatWeapon *, void>                         CBaseCombatWeapon::vt_SetViewModel (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::SetViewModel");
MemberVFuncThunk<      CBaseCombatWeapon *, void>                         CBaseCombatWeapon::vt_PrimaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::PrimaryAttack");
MemberVFuncThunk<      CBaseCombatWeapon *, void>                         CBaseCombatWeapon::vt_SecondaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::SecondaryAttack");
//MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         CBaseCombatWeapon::vt_CanPerformPrimaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::CanPerformPrimaryAttack");
MemberVFuncThunk<      CBaseCombatWeapon *, bool>                         CBaseCombatWeapon::vt_CanPerformSecondaryAttack (TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::CanPerformSecondaryAttack");
MemberVFuncThunk<      CBaseCombatWeapon *, char const *, int>            CBaseCombatWeapon::vt_GetShootSound(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetShootSound");
MemberVFuncThunk<      CBaseCombatWeapon *, int>                          CBaseCombatWeapon::vt_GetPrimaryAmmoType(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::GetPrimaryAmmoType");
MemberVFuncThunk<      CBaseCombatWeapon *, void, int>                    CBaseCombatWeapon::vt_SetSubType(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::SetSubType");
MemberVFuncThunk<      CBaseCombatWeapon *, void>                         CBaseCombatWeapon::vt_CheckReload(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::CheckReload");
MemberVFuncThunk<      CBaseCombatWeapon *, Activity, Activity, bool *>   CBaseCombatWeapon::vt_ActivityOverride(TypeName<CBaseCombatWeapon>(), "CBaseCombatWeapon::ActivityOverride");

IMPL_SENDPROP(int,                        CBaseViewModel, m_nViewModelIndex, CBaseViewModel);
IMPL_SENDPROP(CHandle<CBaseEntity>,       CBaseViewModel, m_hOwner,          CBaseViewModel);
IMPL_SENDPROP(CHandle<CBaseCombatWeapon>, CBaseViewModel, m_hWeapon,         CBaseViewModel);


MemberFuncThunk<CBaseViewModel *, void, bool> CBaseViewModel::ft_SetControlPanelsActive("CBaseViewModel::SetControlPanelsActive");
MemberFuncThunk<CBaseViewModel *, void> CBaseViewModel::ft_SpawnControlPanels("CBaseViewModel::SpawnControlPanels");
MemberFuncThunk<CBaseViewModel *, void> CBaseViewModel::ft_DestroyControlPanels("CBaseViewModel::DestroyControlPanels");

MemberVFuncThunk<CBaseViewModel *, void, const char *, CBaseCombatWeapon *> CBaseViewModel::vt_SetWeaponModel(TypeName<CBaseViewModel>(), "CBaseViewModel::SetWeaponModel");