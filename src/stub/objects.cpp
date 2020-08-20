#include "stub/objects.h"


IMPL_DATAMAP (int,                CBaseObject, m_nDefaultUpgradeLevel);

IMPL_SENDPROP(int,                CBaseObject, m_iUpgradeLevel,       CBaseObject);
IMPL_SENDPROP(int,                CBaseObject, m_iObjectType,         CBaseObject);
IMPL_SENDPROP(int,                CBaseObject, m_iObjectMode,         CBaseObject);
IMPL_SENDPROP(CHandle<CTFPlayer>, CBaseObject, m_hBuilder,            CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bMiniBuilding,       CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bDisposableBuilding, CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bBuilding, CBaseObject);
IMPL_SENDPROP(bool,               CBaseObject, m_bDisabled, CBaseObject);

MemberFuncThunk<CBaseObject *, void, float> CBaseObject::ft_SetHealth        ("CBaseObject::SetHealth");
MemberFuncThunk<CBaseObject *, void, float> CBaseObject::ft_SetPlasmaDisabled("CBaseObject::SetPlasmaDisabled");
MemberFuncThunk<CBaseObject *, bool>        CBaseObject::ft_HasSapper        ("CBaseObject::HasSapper");

MemberVFuncThunk<CBaseObject *, void, CTFPlayer *>   CBaseObject::vt_StartPlacement               (TypeName<CBaseObject>(), "CBaseObject::StartPlacement");
MemberVFuncThunk<CBaseObject *, bool, CBaseEntity *> CBaseObject::vt_StartBuilding                (TypeName<CBaseObject>(), "CBaseObject::StartBuilding");
MemberVFuncThunk<CBaseObject *, void>                CBaseObject::vt_DetonateObject               (TypeName<CBaseObject>(), "CBaseObject::DetonateObject");
MemberVFuncThunk<CBaseObject *, void>                CBaseObject::vt_InitializeMapPlacedObject    (TypeName<CBaseObject>(), "CBaseObject::InitializeMapPlacedObject");
MemberVFuncThunk<CBaseObject *, void>                CBaseObject::vt_FinishedBuilding             (TypeName<CBaseObject>(), "CBaseObject::FinishedBuilding");
MemberVFuncThunk<CBaseObject *, int>                 CBaseObject::vt_GetMiniBuildingStartingHealth(TypeName<CBaseObject>(), "CBaseObject::GetMiniBuildingStartingHealth");
MemberVFuncThunk<CBaseObject *, int>                 CBaseObject::vt_GetMaxHealthForCurrentLevel  (TypeName<CBaseObject>(), "CBaseObject::GetMaxHealthForCurrentLevel");


GlobalThunk<CUtlVector<IBaseObjectAutoList *>> IBaseObjectAutoList::m_IBaseObjectAutoListAutoList("IBaseObjectAutoList::m_IBaseObjectAutoListAutoList");


MemberVFuncThunk<CObjectDispenser *, float> CObjectDispenser::vt_GetDispenserRadius(TypeName<CObjectDispenser>(), "CObjectDispenser::GetDispenserRadius");

MemberVFuncThunk<CObjectSentrygun *, QAngle &> CObjectSentrygun::vt_GetTurretAngles(TypeName<CObjectSentrygun>(), "CObjectSentrygun::GetTurretAngles");