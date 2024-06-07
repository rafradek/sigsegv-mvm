#include "stub/entities.h"

IMPL_DATAMAP(bool,             CAmbientGeneric, m_fActive);
IMPL_DATAMAP(string_t,         CAmbientGeneric, m_iszSound);

IMPL_DATAMAP(int, CPathTrack, m_eOrientationType);

IMPL_DATAMAP(string_t,             CEnvEntityMaker, m_iszTemplate);
IMPL_DATAMAP(QAngle,               CEnvEntityMaker,   m_angPostSpawnDirection);
IMPL_DATAMAP(float,                CEnvEntityMaker,   m_flPostSpawnDirectionVariance);
IMPL_DATAMAP(float,                CEnvEntityMaker,   m_flPostSpawnSpeed);
IMPL_DATAMAP(bool,                 CEnvEntityMaker,   m_bPostSpawnUseAngles);

MemberFuncThunk<CPathTrack *, CPathTrack *> CPathTrack::ft_GetNext("CPathTrack::GetNext");

#ifdef SE_IS_TF2
IMPL_DATAMAP(float,                CItem, m_flNextResetCheckTime);
#endif
IMPL_DATAMAP(bool,                 CItem, m_bActivateWhenAtRest);
IMPL_DATAMAP(Vector,               CItem, m_vOriginalSpawnOrigin);
IMPL_DATAMAP(QAngle,               CItem, m_vOriginalSpawnAngles);
IMPL_DATAMAP(IPhysicsConstraint *, CItem, m_pConstraint);


IMPL_DATAMAP(bool, CBaseTrigger, m_bDisabled);

MemberVFuncThunk<CBaseTrigger *, bool, CBaseEntity *> CBaseTrigger::vt_PassesTriggerFilters(TypeName<CBaseTrigger>(),"CBaseTrigger::PassesTriggerFilters");


IMPL_DATAMAP(int,      CFuncNavPrerequisite, m_task);
IMPL_DATAMAP(string_t, CFuncNavPrerequisite, m_taskEntityName);
IMPL_DATAMAP(float,    CFuncNavPrerequisite, m_taskValue);
IMPL_DATAMAP(bool,     CFuncNavPrerequisite, m_isDisabled);

#define IMPL_DATAMAP_ONCASE(CLASSNAME, PROPNAME, PROPSTR) \
	const size_t CLASSNAME::_adj_##PROPNAME = offsetof(CLASSNAME, PROPNAME); \
	size_t CLASSNAME::_offset_##PROPNAME = -CLASSNAME::_adj_##PROPNAME; \
	CProp_DataMap CLASSNAME::s_prop_##PROPNAME(#CLASSNAME, #PROPSTR, &CLASSNAME::_offset_##PROPNAME)

IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase01, m_OnCase[0]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase02, m_OnCase[1]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase03, m_OnCase[2]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase04, m_OnCase[3]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase05, m_OnCase[4]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase06, m_OnCase[5]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase07, m_OnCase[6]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase08, m_OnCase[7]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase09, m_OnCase[8]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase10, m_OnCase[9]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase11, m_OnCase[10]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase12, m_OnCase[11]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase13, m_OnCase[12]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase14, m_OnCase[13]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase15, m_OnCase[14]);
IMPL_DATAMAP_ONCASE(CLogicCase, m_OnCase16, m_OnCase[15]);

const size_t CLogicCase::_adj_m_nCase = offsetof(CLogicCase, m_nCase);
size_t CLogicCase::_offset_m_nCase = -offsetof(CLogicCase, m_nCase);
CProp_DataMap CLogicCase::s_prop_m_nCase("CLogicCase", "m_nCase[0]", &CLogicCase::_offset_m_nCase);
	
IMPL_DATAMAP(CBaseEntityOutput, CLogicCase, m_OnDefault);


IMPL_DATAMAP(CBaseEntityOutput, CBaseFilter, m_OnPass);
IMPL_DATAMAP(CBaseEntityOutput, CBaseFilter, m_OnFail);

MemberFuncThunk<CBaseFilter *, bool, CBaseEntity *, CBaseEntity *> CBaseFilter::ft_PassesFilter("CBaseFilter::PassesFilter");


IMPL_DATAMAP (string_t, CSmokeStack, m_strMaterialModel);
IMPL_SENDPROP(int,      CSmokeStack, m_iMaterialModel, CSmokeStack);


IMPL_DATAMAP (CHandle<CBasePlayer>, CGameUI, m_player);


IMPL_DATAMAP (CHandle<CBaseEntity>, CTriggerCamera, m_hPlayer);
IMPL_DATAMAP (CHandle<CBaseEntity>, CTriggerCamera, m_hTarget);

MemberFuncThunk<CTriggerCamera *, void> CTriggerCamera::ft_Enable("CTriggerCamera::Enable");
MemberFuncThunk<CTriggerCamera *, void> CTriggerCamera::ft_Disable("CTriggerCamera::Disable");

IMPL_DATAMAP (bool, CFuncRotating, m_bReversed);
IMPL_DATAMAP (float, CFuncRotating, m_flMaxSpeed);
IMPL_DATAMAP (bool, CFuncRotating, m_bStopAtStartPos);
IMPL_DATAMAP (QAngle, CFuncRotating, m_vecMoveAng);
IMPL_DATAMAP (float, CFuncRotating, m_flTargetSpeed);

MemberFuncThunk<CFuncRotating *, void, float> CFuncRotating::ft_SetTargetSpeed("CFuncRotating::SetTargetSpeed");

IMPL_DATAMAP (unsigned int, CPropVehicle, m_nVehicleType);
IMPL_DATAMAP (string_t, CPropVehicle, m_vehicleScript);
IMPL_DATAMAP (CHandle<CBasePlayer>, CPropVehicle, m_hPhysicsAttacker);
IMPL_DATAMAP (float, CPropVehicle, m_flLastPhysicsInfluenceTime);

IMPL_DATAMAP (bool, CPropVehicleDriveable, m_bEnterAnimOn);
IMPL_DATAMAP (bool, CPropVehicleDriveable, m_bExitAnimOn);
IMPL_DATAMAP (bool, CPropVehicleDriveable, m_flMinimumSpeedToEnterExit);
IMPL_DATAMAP (CBaseServerVehicle *, CPropVehicleDriveable, m_pServerVehicle);
IMPL_DATAMAP (CHandle<CBasePlayer>, CPropVehicleDriveable, m_hPlayer);
IMPL_DATAMAP (float, CPropVehicleDriveable, m_nSpeed);
IMPL_DATAMAP (float, CPropVehicleDriveable, m_bLocked);

StaticFuncThunk<CSprite *, const char *, const Vector &, bool> CSprite::ft_SpriteCreate("CSprite::SpriteCreate");

IMPL_SENDPROP(float, CSpriteTrail, m_flLifeTime, CSpriteTrail);

IMPL_DATAMAP (bool, CParticleSystem, m_bStartActive);
IMPL_DATAMAP (string_t, CParticleSystem, m_iszEffectName);
IMPL_SENDPROP(CHandle<CBaseEntity>[63], CParticleSystem, m_hControlPointEnts, CParticleSystem);

MemberVFuncThunk<CBaseServerVehicle *, void, bool, bool> CBaseServerVehicle::vt_HandleEntryExitFinish(TypeName<CBaseServerVehicle>(), "CBaseServerVehicle::HandleEntryExitFinish");
MemberVFuncThunk<CBaseServerVehicle *, void, CBasePlayer *, CUserCmd *,  void *, void *> CBaseServerVehicle::vt_SetupMove(TypeName<CBaseServerVehicle>(), "CBaseServerVehicle::SetupMove");
MemberVFuncThunk<CBaseServerVehicle *, bool, CBaseCombatCharacter *> CBaseServerVehicle::vt_HandlePassengerExit(TypeName<CBaseServerVehicle>(), "CBaseServerVehicle::HandlePassengerExit");
MemberVFuncThunk<CBaseServerVehicle *, void, CBaseCombatCharacter *, bool> CBaseServerVehicle::vt_HandlePassengerEntry(TypeName<CBaseServerVehicle>(), "CBaseServerVehicle::HandlePassengerEntry");
MemberVFuncThunk<CBaseServerVehicle *, CBaseEntity *> CBaseServerVehicle::vt_GetDriver(TypeName<CBaseServerVehicle>(), "CBaseServerVehicle::GetDriver");
MemberVFuncThunk<CBaseServerVehicle *, CBaseEntity *> CBaseServerVehicle::vt_GetVehicleEnt(TypeName<CBaseServerVehicle>(), "CBaseServerVehicle::GetVehicleEnt");

GlobalThunk<CBaseEntity *> g_WorldEntity("g_WorldEntity");

MemberFuncThunk<CBeam *, void, CBaseEntity *, CBaseEntity *> CBeam::ft_EntsInit("CBeam::EntsInit");

StaticFuncThunk<CBeam *, const char *, float> CBeam::ft_BeamCreate("CBeam::BeamCreate");