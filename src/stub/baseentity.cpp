#include "stub/baseentity.h"
#include "stub/baseplayer.h"
//#include "util/iterate.h"
#ifdef SE_IS_TF2
#include "stub/objects.h"
#endif

// This is set in extension.cpp
edict_t *g_pWorldEdict = nullptr;

#ifdef SE_IS_TF2
MemberFuncThunk<CGlobalEntityList *, CBaseEntity *, CBaseEntity *, const char *, IEntityFindFilter *> CGlobalEntityList::ft_FindEntityByClassname("CGlobalEntityList::FindEntityByClassname");
#else
MemberFuncThunk<CGlobalEntityList *, CBaseEntity *, CBaseEntity *, const char *> CGlobalEntityList::ft_FindEntityByClassname("CGlobalEntityList::FindEntityByClassname");
#endif

MemberFuncThunk<      CServerNetworkProperty *, void, edict_t *> CServerNetworkProperty::ft_AttachEdict("CServerNetworkProperty::AttachEdict");
MemberFuncThunk<      CServerNetworkProperty *, bool, CCheckTransmitInfo *> CServerNetworkProperty::ft_IsInPVS("CServerNetworkProperty::IsInPVS");

IMPL_DATAMAP(string_t,               CBaseEntity, m_target);
IMPL_DATAMAP(string_t,               CBaseEntity, m_iParent);
IMPL_DATAMAP(int,                    CBaseEntity, m_debugOverlays);
IMPL_DATAMAP(CServerNetworkProperty, CBaseEntity, m_Network);
IMPL_DATAMAP(string_t,               CBaseEntity, m_iClassname);
IMPL_DATAMAP(string_t,               CBaseEntity, m_iName);
IMPL_DATAMAP(int,                    CBaseEntity, m_iEFlags);
IMPL_DATAMAP(Vector,                 CBaseEntity, m_vecAbsOrigin);
IMPL_DATAMAP(QAngle,                 CBaseEntity, m_angAbsRotation);
IMPL_DATAMAP(Vector,                 CBaseEntity, m_vecAbsVelocity);
IMPL_DATAMAP(IPhysicsObject*,        CBaseEntity, m_pPhysicsObject);
IMPL_DATAMAP(matrix3x4_t,            CBaseEntity, m_rgflCoordinateFrame);
IMPL_DATAMAP(int,                    CBaseEntity, m_nNextThinkTick);
IMPL_DATAMAP(CHandle<CBaseEntity>,   CBaseEntity, m_hMoveChild);
IMPL_DATAMAP(CHandle<CBaseEntity>,   CBaseEntity, m_hMovePeer);
IMPL_DATAMAP(CHandle<CBaseEntity>,   CBaseEntity, m_hMoveParent);
IMPL_DATAMAP(float,                  CBaseEntity, m_flGravity);
IMPL_DATAMAP(QAngle,                 CBaseEntity, m_vecAngVelocity);
IMPL_DATAMAP(float,                  CBaseEntity, m_flLocalTime);
IMPL_DATAMAP(float,                  CBaseEntity, m_flAnimTime);
IMPL_DATAMAP(float,                  CBaseEntity, m_flSimulationTime);
IMPL_DATAMAP(float,                  CBaseEntity, m_flVPhysicsUpdateLocalTime);
IMPL_DATAMAP(int,                    CBaseEntity, m_spawnflags);
IMPL_DATAMAP(CBaseEntityOutput,      CBaseEntity, m_OnUser1);
IMPL_DATAMAP(CBaseEntityOutput,      CBaseEntity, m_OnUser2);
IMPL_DATAMAP(CBaseEntityOutput,      CBaseEntity, m_OnUser3);
IMPL_DATAMAP(CBaseEntityOutput,      CBaseEntity, m_OnUser4);
IMPL_DATAMAP(bool,                   CBaseEntity, m_takedamage);
IMPL_DATAMAP(void (CBaseEntity::*)(CBaseEntity *), CBaseEntity, m_pfnTouch);
IMPL_DATAMAP(int,                    CBaseEntity, m_iInitialTeamNum);
IMPL_REL_AFTER(ExtraEntityData *, CBaseEntity, m_extraEntityData, m_debugOverlays);
IMPL_REL_BEFORE(IHasAttributes *, CBaseEntity, m_pAttributes, m_iMaxHealth, 0, string_t, void *);
IMPL_DATAMAP(unsigned char,          CBaseEntity, m_nWaterLevel);
IMPL_SENDPROP(unsigned char,          CBaseEntity, m_nRenderFX,   CBaseEntity);
IMPL_SENDPROP(unsigned char,          CBaseEntity, m_iParentAttachment,  CBaseEntity);

#ifdef SE_IS_TF2
IMPL_SENDPROP(int,                  CBaseEntity, m_iMaxHealth,           CBaseObject);
#else
IMPL_DATAMAP(int,                  CBaseEntity, m_iMaxHealth);
#endif
IMPL_SENDPROP(int,                  CBaseEntity, m_iTextureFrameIndex,   CBaseEntity);
IMPL_SENDPROP(CCollisionProperty,   CBaseEntity, m_Collision,            CBaseEntity);
IMPL_SENDPROP(int,                  CBaseEntity, m_iTeamNum,             CBaseEntity);
IMPL_SENDPROP(int,                  CBaseEntity, m_iHealth,              CBasePlayer);
IMPL_SENDPROP(char,                 CBaseEntity, m_lifeState,            CBasePlayer);
IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseEntity, m_hGroundEntity,        CBasePlayer);
IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseEntity, m_hOwnerEntity,         CBaseEntity);
IMPL_SENDPROP(int,                  CBaseEntity, m_fFlags,               CBasePlayer);
IMPL_SENDPROP(int,                  CBaseEntity, m_CollisionGroup,       CBaseEntity);
IMPL_SENDPROP(unsigned char,        CBaseEntity, m_nRenderMode,          CBaseEntity);
IMPL_SENDPROP(unsigned char,        CBaseEntity, m_MoveType,             CBaseEntity, "movetype");
IMPL_SENDPROP(unsigned char,        CBaseEntity, m_MoveCollide,          CBaseEntity, "movecollide");

#ifdef SE_IS_TF2
IMPL_SENDPROP(int[4],               CBaseEntity, m_nModelIndexOverrides, CBaseEntity);
#endif
IMPL_SENDPROP(color32,              CBaseEntity, m_clrRender,            CBaseEntity);
IMPL_SENDPROP(Vector,               CBaseEntity, m_vecVelocity,          CBaseGrenade);
IMPL_SENDPROP(Vector,               CBaseEntity, m_vecOrigin,            CBaseEntity);
IMPL_SENDPROP(QAngle,               CBaseEntity, m_angRotation,          CBaseEntity);
IMPL_REL_AFTER(CBaseHandle,   CBaseEntity, m_RefEHandle, m_angRotation, );
IMPL_SENDPROP(int,                  CBaseEntity, m_fEffects,             CBaseEntity);

MemberFuncThunk<      CBaseEntity *, void                                                   > CBaseEntity::ft_Remove                     ("CBaseEntity::Remove");
MemberFuncThunk<      CBaseEntity *, void                                                   > CBaseEntity::ft_CalcAbsolutePosition       ("CBaseEntity::CalcAbsolutePosition");
MemberFuncThunk<      CBaseEntity *, void                                                   > CBaseEntity::ft_CalcAbsoluteVelocity       ("CBaseEntity::CalcAbsoluteVelocity");
MemberFuncThunk<      CBaseEntity *, bool, const char *                                     > CBaseEntity::ft_NameMatchesComplex         ("CBaseEntity::NameMatchesComplex");
MemberFuncThunk<      CBaseEntity *, bool, const char *                                     > CBaseEntity::ft_ClassMatchesComplex        ("CBaseEntity::ClassMatchesComplex");
MemberFuncThunk<      CBaseEntity *, void, const Vector&                                    > CBaseEntity::ft_SetAbsOrigin               ("CBaseEntity::SetAbsOrigin");
MemberFuncThunk<      CBaseEntity *, void, const QAngle&                                    > CBaseEntity::ft_SetAbsAngles               ("CBaseEntity::SetAbsAngles");
MemberFuncThunk<      CBaseEntity *, void, const Vector&                                    > CBaseEntity::ft_SetAbsVelocity             ("CBaseEntity::SetAbsVelocity");
MemberFuncThunk<      CBaseEntity *, void, const char *, float, float *                     > CBaseEntity::ft_EmitSound_member1          ("CBaseEntity::EmitSound [member: normal]");
MemberFuncThunk<      CBaseEntity *, void, const char *, HSOUNDSCRIPTHANDLE&, float, float *> CBaseEntity::ft_EmitSound_member2          ("CBaseEntity::EmitSound [member: normal + handle]");
MemberFuncThunk<      CBaseEntity *, void, const char *                                     > CBaseEntity::ft_StopSound                  ("CBaseEntity::StopSound");
MemberFuncThunk<      CBaseEntity *, float, const char *                                    > CBaseEntity::ft_GetNextThink               ("CBaseEntity::GetNextThink");
MemberFuncThunk<const CBaseEntity *, bool                                                   > CBaseEntity::ft_IsBSPModel                 ("CBaseEntity::IsBSPModel");
MemberFuncThunk<      CBaseEntity *, void, int, const char *, float, int, int, int, int     > CBaseEntity::ft_EntityText                 ("CBaseEntity::EntityText");
MemberFuncThunk<      CBaseEntity *, int, const CTakeDamageInfo&                            > CBaseEntity::ft_TakeDamage                 ("CBaseEntity::TakeDamage");
MemberFuncThunk<      CBaseEntity *, void, MoveType_t, MoveCollide_t                        > CBaseEntity::ft_SetMoveType                ("CBaseEntity::SetMoveType");
MemberFuncThunk<      CBaseEntity *, model_t *                                              > CBaseEntity::ft_GetModel                   ("CBaseEntity::GetModel");
MemberFuncThunk<      CBaseEntity *, void, float, const char *                              > CBaseEntity::ft_SetNextThink_name          ("CBaseEntity::SetNextThink [name]");
MemberFuncThunk<      CBaseEntity *, void, int, float                                       > CBaseEntity::ft_SetNextThink_index         ("CBaseEntity::SetNextThink [index]");
MemberFuncThunk<      CBaseEntity *, BASEPTR, BASEPTR, float, const char *                  > CBaseEntity::ft_ThinkSet                   ("CBaseEntity::ThinkSet");
MemberFuncThunk<      CBaseEntity *, int                                                    > CBaseEntity::ft_DispatchUpdateTransmitState("CBaseEntity::DispatchUpdateTransmitState");
MemberFuncThunk<      CBaseEntity *, void, int                                              > CBaseEntity::ft_SetEffects                 ("CBaseEntity::SetEffects");
MemberFuncThunk<      CBaseEntity *, void, int                                              > CBaseEntity::ft_AddEffects                 ("CBaseEntity::AddEffects");
MemberFuncThunk<      CBaseEntity *, bool, const char *, variant_t *                        > CBaseEntity::ft_ReadKeyField               ("CBaseEntity::ReadKeyField");
MemberFuncThunk<      CBaseEntity *, IPhysicsObject *                                       > CBaseEntity::ft_VPhysicsInitStatic         ("CBaseEntity::VPhysicsInitStatic");
MemberFuncThunk<      CBaseEntity *, void *,int                                             > CBaseEntity::ft_GetDataObject              ("CBaseEntity::GetDataObject");
MemberFuncThunk<      CBaseEntity *, int, int                                               > CBaseEntity::ft_SetTransmitState           ("CBaseEntity::SetTransmitState");
MemberFuncThunk<      CBaseEntity *, void, const char *, variant_t, CBaseEntity *, CBaseEntity *, float> CBaseEntity::ft_FireNamedOutput ("CBaseEntity::FireNamedOutput");
MemberFuncThunk<      CBaseEntity *, void, const Vector &                                   > CBaseEntity::ft_ApplyAbsVelocityImpulse    ("CBaseEntity::ApplyAbsVelocityImpulse");
MemberFuncThunk<      CBaseEntity *, void, const Vector &                                   > CBaseEntity::ft_SetLocalVelocity           ("CBaseEntity::SetLocalVelocity");
MemberFuncThunk<      CBaseEntity *, void, const Vector &                                   > CBaseEntity::ft_SetLocalOrigin             ("CBaseEntity::SetLocalOrigin");
MemberFuncThunk<      CBaseEntity *, void, const QAngle &                                   > CBaseEntity::ft_SetLocalAngles             ("CBaseEntity::SetLocalAngles");

MemberVFuncThunk<const CBaseEntity *, Vector                                                          > CBaseEntity::vt_EyePosition                   (TypeName<CBaseEntity>(), "CBaseEntity::EyePosition");
MemberVFuncThunk<      CBaseEntity *, const QAngle&                                                   > CBaseEntity::vt_EyeAngles                     (TypeName<CBaseEntity>(), "CBaseEntity::EyeAngles");
MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *                                             > CBaseEntity::vt_SetOwnerEntity                (TypeName<CBaseEntity>(), "CBaseEntity::SetOwnerEntity");
MemberVFuncThunk<      CBaseEntity *, void                                                            > CBaseEntity::vt_Spawn                         (TypeName<CBaseEntity>(), "CBaseEntity::Spawn");
MemberVFuncThunk<      CBaseEntity *, void                                                            > CBaseEntity::vt_Activate                      (TypeName<CBaseEntity>(), "CBaseEntity::Activate");
MemberVFuncThunk<      CBaseEntity *, void, Vector *, AngularImpulse *                                > CBaseEntity::vt_GetVelocity                   (TypeName<CBaseEntity>(), "CBaseEntity::GetVelocity");
MemberVFuncThunk<const CBaseEntity *, const Vector&                                                   > CBaseEntity::vt_WorldSpaceCenter              (TypeName<CBaseEntity>(), "CBaseEntity::WorldSpaceCenter");
MemberVFuncThunk<const CBaseEntity *, bool                                                            > CBaseEntity::vt_IsBaseCombatWeapon            (TypeName<CBaseEntity>(), "CBaseEntity::IsBaseCombatWeapon");
MemberVFuncThunk<const CBaseEntity *, bool                                                            > CBaseEntity::vt_IsWearable                    (TypeName<CBaseEntity>(), "CBaseEntity::IsWearable");
MemberVFuncThunk<const CBaseEntity *, bool                                                            > CBaseEntity::vt_IsCombatItem                  (TypeName<CBaseEntity>(), "CBaseEntity::IsCombatItem");
MemberVFuncThunk<      CBaseEntity *, void, int                                                       > CBaseEntity::vt_SetModelIndex                 (TypeName<CBaseEntity>(), "CBaseEntity::SetModelIndex");
MemberVFuncThunk<const CBaseEntity *, int                                                             > CBaseEntity::vt_GetModelIndex                 (TypeName<CBaseEntity>(), "CBaseEntity::GetModelIndex");
MemberVFuncThunk<const CBaseEntity *, string_t                                                        > CBaseEntity::vt_GetModelName                  (TypeName<CBaseEntity>(), "CBaseEntity::GetModelName");
MemberVFuncThunk<      CBaseEntity *, CBaseCombatCharacter *                                          > CBaseEntity::vt_MyCombatCharacterPointer      (TypeName<CBaseEntity>(), "CBaseEntity::MyCombatCharacterPointer");
MemberVFuncThunk<      CBaseEntity *, CBaseCombatWeapon *                                             > CBaseEntity::vt_MyCombatWeaponPointer         (TypeName<CBaseEntity>(), "CBaseEntity::MyCombatWeaponPointer");
MemberVFuncThunk<const CBaseEntity *, bool, int, int                                                  > CBaseEntity::vt_ShouldCollide                 (TypeName<CBaseEntity>(), "CBaseEntity::ShouldCollide");
MemberVFuncThunk<      CBaseEntity *, void                                                            > CBaseEntity::vt_DrawDebugGeometryOverlays     (TypeName<CBaseEntity>(), "CBaseEntity::DrawDebugGeometryOverlays");
MemberVFuncThunk<      CBaseEntity *, void, int                                                       > CBaseEntity::vt_ChangeTeam                    (TypeName<CBaseEntity>(), "CBaseEntity::ChangeTeam");
MemberVFuncThunk<      CBaseEntity *, void, int, int                                                  > CBaseEntity::vt_SetModelIndexOverride         (TypeName<CBaseEntity>(), "CBaseEntity::SetModelIndexOverride");
MemberVFuncThunk<      CBaseEntity *, datamap_t *                                                     > CBaseEntity::vt_GetDataDescMap                (TypeName<CBaseEntity>(), "CBaseEntity::GetDataDescMap");
MemberVFuncThunk<      CBaseEntity *, bool, const char *, CBaseEntity *, CBaseEntity *, variant_t, int> CBaseEntity::vt_AcceptInput                   (TypeName<CBaseEntity>(), "CBaseEntity::AcceptInput");
MemberVFuncThunk<      CBaseEntity *, void, const char *                                              > CBaseEntity::vt_SetModel                      (TypeName<CBaseEntity>(), "CBaseEntity::SetModel");
MemberVFuncThunk<      CBaseEntity *, float                                                           > CBaseEntity::vt_GetDamage                     (TypeName<CBaseEntity>(), "CBaseEntity::GetDamage");
MemberVFuncThunk<      CBaseEntity *, void, float                                                     > CBaseEntity::vt_SetDamage                     (TypeName<CBaseEntity>(), "CBaseEntity::SetDamage");
MemberVFuncThunk<      CBaseEntity *, bool, CBaseEntity *, int, CBaseEntity **                        > CBaseEntity::vt_FVisible_ent                  (TypeName<CBaseEntity>(), "CBaseEntity::FVisible [ent]");
MemberVFuncThunk<      CBaseEntity *, bool, const Vector&, int, CBaseEntity **                        > CBaseEntity::vt_FVisible_vec                  (TypeName<CBaseEntity>(), "CBaseEntity::FVisible [vec]");
MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *                                             > CBaseEntity::vt_Touch                         (TypeName<CBaseEntity>(), "CBaseEntity::Touch");
MemberVFuncThunk<const CBaseEntity *, INextBot *                                                      > CBaseEntity::vt_MyNextBotPointer              (TypeName<CBaseEntity>(), "CBaseEntity::MyNextBotPointer");
MemberVFuncThunk<      CBaseEntity *, void, const Vector *, const QAngle *, const Vector *            > CBaseEntity::vt_Teleport                      (TypeName<CBaseEntity>(), "CBaseEntity::Teleport");
MemberVFuncThunk<const CBaseEntity *, int                                                             > CBaseEntity::vt_GetMaxHealth                  (TypeName<CBaseEntity>(), "CBaseEntity::GetMaxHealth");
MemberVFuncThunk<      CBaseEntity *, bool                                                            > CBaseEntity::vt_IsAlive                       (TypeName<CBaseEntity>(), "CBaseEntity::IsAlive");
#ifdef SE_IS_TF2
MemberVFuncThunk<const CBaseEntity *, float                                                           > CBaseEntity::vt_GetDefaultItemChargeMeterValue(TypeName<CBaseEntity>(), "CBaseEntity::GetDefaultItemChargeMeterValue");
#endif
MemberVFuncThunk<      CBaseEntity *, bool                                                            > CBaseEntity::vt_IsDeflectable                 (TypeName<CBaseEntity>(), "CBaseEntity::IsDeflectable");
MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *, int                                        > CBaseEntity::vt_SetParent                     (TypeName<CBaseEntity>(), "CBaseEntity::SetParent");
MemberVFuncThunk<const CBaseEntity *, bool                                                            > CBaseEntity::vt_IsPlayer                      (TypeName<CBaseEntity>(), "CBaseEntity::IsPlayer");
MemberVFuncThunk<const CBaseEntity *, bool                                                            > CBaseEntity::vt_IsBaseObject                  (TypeName<CBaseEntity>(), "CBaseEntity::IsBaseObject");
MemberVFuncThunk<      CBaseEntity *, bool, const char *, const char *                                > CBaseEntity::vt_KeyValue                      (TypeName<CBaseEntity>(), "CBaseEntity::KeyValue");
MemberVFuncThunk<      CBaseEntity *, bool, const char *, char *, int                                 > CBaseEntity::vt_GetKeyValue                   (TypeName<CBaseEntity>(), "CBaseEntity::GetKeyValue");
MemberVFuncThunk<      CBaseEntity *, void, const FireBulletsInfo_t &                                 > CBaseEntity::vt_FireBullets                   (TypeName<CBaseEntity>(), "CBaseEntity::FireBullets");
MemberVFuncThunk<      CBaseEntity *, ServerClass *                                                   > CBaseEntity::vt_GetServerClass                (TypeName<CBaseEntity>(), "CBaseEntity::GetServerClass");
MemberVFuncThunk<      CBaseEntity *, CBaseAnimating *                                                > CBaseEntity::vt_GetBaseAnimating              (TypeName<CBaseEntity>(), "CBaseEntity::GetBaseAnimating");
MemberVFuncThunk<      CBaseEntity *, int, float, int                                                 > CBaseEntity::vt_TakeHealth                    (TypeName<CBaseEntity>(), "CBaseEntity::TakeHealth");
MemberVFuncThunk<      CBaseEntity *, int                                                             > CBaseEntity::vt_GetDamageType                 (TypeName<CBaseEntity>(), "CBaseEntity::GetDamageType");
MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *                                             > CBaseEntity::vt_StartTouch                    (TypeName<CBaseEntity>(), "CBaseEntity::StartTouch");
MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *                                             > CBaseEntity::vt_EndTouch                      (TypeName<CBaseEntity>(), "CBaseEntity::EndTouch");
MemberVFuncThunk<      CBaseEntity *, void                                                            > CBaseEntity::vt_PostClientActive              (TypeName<CBaseEntity>(), "CBaseEntity::PostClientActive");
MemberVFuncThunk<      CBaseEntity *, void                                                            > CBaseEntity::vt_VPhysicsDestroyObject         (TypeName<CBaseEntity>(), "CBaseEntity::VPhysicsDestroyObject");
MemberVFuncThunk<      CBaseEntity *, uint                                                            > CBaseEntity::vt_PhysicsSolidMaskForEntity     (TypeName<CBaseEntity>(), "CBaseEntity::PhysicsSolidMaskForEntity");
MemberVFuncThunk<const CBaseEntity *, const char *                                                    > CBaseEntity::vt_GetTracerType                 (TypeName<CBaseEntity>(), "CBaseEntity::GetTracerType");
MemberVFuncThunk<      CBaseEntity *, void, CCheckTransmitInfo *, bool                                > CBaseEntity::vt_SetTransmit                   (TypeName<CBaseEntity>(), "CBaseEntity::SetTransmit");
MemberVFuncThunk<      CBaseEntity *, int, CCheckTransmitInfo *                                       > CBaseEntity::vt_ShouldTransmit                (TypeName<CBaseEntity>(), "CBaseEntity::ShouldTransmit");

MemberFuncThunk<CBaseEntityOutput *, void, variant_t, CBaseEntity *, CBaseEntity *, float> CBaseEntityOutput::ft_FireOutput("CBaseEntityOutput::FireOutput");
MemberFuncThunk<CBaseEntityOutput *, void, const char *                                  > CBaseEntityOutput::ft_ParseEventAction("CBaseEntityOutput::ParseEventAction");
MemberFuncThunk<CBaseEntityOutput *, void                                                > CBaseEntityOutput::ft_DeleteAllElements("CBaseEntityOutput::DeleteAllElements");

StaticFuncThunk<CBaseEntity *, const char *, const Vector&, const QAngle&, CBaseEntity *>                        CBaseEntity::ft_Create             ("CBaseEntity::Create");
StaticFuncThunk<CBaseEntity *, const char *, const Vector&, const QAngle&, CBaseEntity *>                        CBaseEntity::ft_CreateNoSpawn      ("CBaseEntity::CreateNoSpawn");
StaticFuncThunk<int, const char *, bool>                                                                         CBaseEntity::ft_PrecacheModel      ("CBaseEntity::PrecacheModel");
StaticFuncThunk<bool, const char *>                                                                              CBaseEntity::ft_PrecacheSound      ("CBaseEntity::PrecacheSound");
StaticFuncThunk<HSOUNDSCRIPTHANDLE, const char *>                                                                CBaseEntity::ft_PrecacheScriptSound("CBaseEntity::PrecacheScriptSound");
StaticFuncThunk<void, IRecipientFilter&, int, const char *, const Vector *, float, float *>                      CBaseEntity::ft_EmitSound_static1  ("CBaseEntity::EmitSound [static: normal]");
StaticFuncThunk<void, IRecipientFilter&, int, const char *, HSOUNDSCRIPTHANDLE&, const Vector *, float, float *> CBaseEntity::ft_EmitSound_static2  ("CBaseEntity::EmitSound [static: normal + handle]");
StaticFuncThunk<void, IRecipientFilter&, int, const EmitSound_t&>                                                CBaseEntity::ft_EmitSound_static3  ("CBaseEntity::EmitSound [static: emitsound]");
StaticFuncThunk<void, IRecipientFilter&, int, const EmitSound_t&, HSOUNDSCRIPTHANDLE&>                           CBaseEntity::ft_EmitSound_static4  ("CBaseEntity::EmitSound [static: emitsound + handle]");
StaticFuncThunk<trace_t&>                                                                                        CBaseEntity::ft_GetTouchTrace      ("CBaseEntity::GetTouchTrace");
StaticFuncThunk<void, CBaseEntity *, CBaseEntity *>                                                              CBaseEntity::ft_TransformStepData_ParentToParent("CBaseEntity::TransformStepData_ParentToParent");
StaticFuncThunk<void, CBaseEntity *>                                                                             CBaseEntity::ft_TransformStepData_WorldToParent("CBaseEntity::TransformStepData_WorldToParent");

MemberFuncThunk<CCollisionProperty *, void, SurroundingBoundsType_t, const Vector *, const Vector *> ft_SetSurroundingBoundsType("CCollisionProperty::SetSurroundingBoundsType");
MemberFuncThunk<CCollisionProperty *, void> ft_MarkPartitionHandleDirty("CCollisionProperty::MarkPartitionHandleDirty");
MemberFuncThunk<CCollisionProperty *, void, int> ft_SetSolidFlags("CCollisionProperty::SetSolidFlags");
MemberFuncThunk<CCollisionProperty *, void, const Vector &, const Vector &> ft_SetCollisionBounds("CCollisionProperty::SetCollisionBounds");
MemberFuncThunk<CCollisionProperty *, void, SolidType_t> ft_SetSolid("CCollisionProperty::SetSolid");

StaticFuncThunk<int, CBaseEntity *> ft_ENTINDEX("ENTINDEX");

void CCollisionProperty::SetSurroundingBoundsType(SurroundingBoundsType_t type, const Vector *pMins, const Vector *pMaxs) {
	ft_SetSurroundingBoundsType(this, type, pMins, pMaxs);
}

void CCollisionProperty::MarkPartitionHandleDirty() {
	ft_MarkPartitionHandleDirty(this);
}

void CCollisionProperty::SetSolidFlags(int flags) {
	ft_SetSolidFlags(this, flags);
}

void CCollisionProperty::SetCollisionBounds(const Vector &min, const Vector &max) {
	ft_SetCollisionBounds(this, min, max);
}

void CCollisionProperty::SetSolid(SolidType_t solid) {
	ft_SetSolid(this, solid);
}

// bool CBaseEntity::IsPlayer() const
// {
// 	return (rtti_cast<const CBasePlayer *>(this) != nullptr);
// }

// bool CBaseEntity::IsBaseObject() const
// {
// 	return (rtti_cast<const CBaseObject *>(this) != nullptr);
// }


void CCollisionProperty::CalcNearestPoint(const Vector& vecWorldPt, Vector *pVecNearestWorldPt) const
{
	Vector localPt, localClosestPt;
	WorldToCollisionSpace(vecWorldPt, &localPt);
	CalcClosestPointOnAABB(this->m_vecMins.Get(), this->m_vecMaxs.Get(), localPt, localClosestPt);
	CollisionToWorldSpace(localClosestPt, pVecNearestWorldPt);
}

bool CCollisionProperty::IsPointInBounds( const Vector &vecWorldPt ) const
{
	Vector vecLocalSpace;
	WorldToCollisionSpace( vecWorldPt, &vecLocalSpace );
	return ( ( vecLocalSpace.x >= m_vecMins.Get().x && vecLocalSpace.x <= m_vecMaxs.Get().x ) &&
			( vecLocalSpace.y >= m_vecMins.Get().y && vecLocalSpace.y <= m_vecMaxs.Get().y ) &&
			( vecLocalSpace.z >= m_vecMins.Get().z && vecLocalSpace.z <= m_vecMaxs.Get().z ) );
}

const char *variant_t::ToString( void ) const
{
	COMPILE_TIME_ASSERT( sizeof(string_t) == sizeof(const char *) );

	static char szBuf[512];

	switch (fieldType)
	{
	case FIELD_STRING:
		{
			return(STRING(iszVal));
		}

	case FIELD_BOOLEAN:
		{
			if (bVal == 0)
			{
				Q_strncpy(szBuf, "false",sizeof(szBuf));
			}
			else
			{
				Q_strncpy(szBuf, "true",sizeof(szBuf));
			}
			return(szBuf);
		}

	case FIELD_INTEGER:
		{
			Q_snprintf( szBuf, sizeof( szBuf ), "%i", iVal );
			return(szBuf);
		}

	case FIELD_FLOAT:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%g", flVal);
			return(szBuf);
		}

	case FIELD_COLOR32:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "%d %d %d %d", (int)rgbaVal.r, (int)rgbaVal.g, (int)rgbaVal.b, (int)rgbaVal.a);
			return(szBuf);
		}

	case FIELD_VECTOR:
		{
			Q_snprintf(szBuf,sizeof(szBuf), "[%g %g %g]", (double)vecVal[0], (double)vecVal[1], (double)vecVal[2]);
			return(szBuf);
		}

	case FIELD_VOID:
		{
			szBuf[0] = '\0';
			return(szBuf);
		}

	case FIELD_EHANDLE:
		{
			auto entity = Entity();
			if (!entity) {
				Q_strncpy(szBuf, "<<null entity>>",sizeof(szBuf));
			}
			else {
				if (entity->GetEntityName() != NULL_STRING && *STRING(entity->GetEntityName()) != '\0') {
					Q_strncpy(szBuf, STRING(entity->GetEntityName()),sizeof(szBuf));
				}
				else {
					Q_snprintf(szBuf,sizeof(szBuf), "@h@%d(%s)", entity->GetRefEHandle().ToInt(), entity->GetClassname());
				}
			}
			return (szBuf);
		}
	}

	return("No conversion to string");
}

void UnloadAllCustomThinkFunc()
{
	for (CBaseEntity *ent = servertools->FirstEntity(); ent != nullptr; ent = servertools->NextEntity(ent)) {
		for (auto &think : CustomThinkFunc::List()) {
			if (ent->GetNextThink(think->m_sName) != -1) {
				ent->SetNextThink(-1, think->m_sName);
			}
		}
	}
}