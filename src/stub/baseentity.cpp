#include "stub/baseentity.h"
#include "stub/baseplayer.h"
#include "stub/objects.h"


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

IMPL_SENDPROP(int,                  CBaseEntity, m_iTextureFrameIndex,   CBaseEntity);
IMPL_SENDPROP(CCollisionProperty,   CBaseEntity, m_Collision,            CBaseEntity);
IMPL_SENDPROP(int,                  CBaseEntity, m_iTeamNum,             CBaseEntity);
IMPL_SENDPROP(int,                  CBaseEntity, m_iMaxHealth,           CBaseObject);
IMPL_SENDPROP(int,                  CBaseEntity, m_iHealth,              CBasePlayer);
IMPL_SENDPROP(char,                 CBaseEntity, m_lifeState,            CBasePlayer);
IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseEntity, m_hGroundEntity,        CBasePlayer);
IMPL_SENDPROP(CHandle<CBaseEntity>, CBaseEntity, m_hOwnerEntity,         CBaseEntity);
IMPL_SENDPROP(int,                  CBaseEntity, m_fFlags,               CBasePlayer);
IMPL_SENDPROP(int,                  CBaseEntity, m_CollisionGroup,       CBaseEntity);
IMPL_SENDPROP(unsigned char,        CBaseEntity, m_nRenderMode,          CBaseEntity);
IMPL_SENDPROP(unsigned char,        CBaseEntity, m_MoveType,             CBaseEntity, "movetype");
IMPL_SENDPROP(unsigned char,        CBaseEntity, m_MoveCollide,          CBaseEntity, "movecollide");
IMPL_SENDPROP(int[4],               CBaseEntity, m_nModelIndexOverrides, CBaseEntity);
IMPL_SENDPROP(color32,              CBaseEntity, m_clrRender,            CBaseEntity);
IMPL_SENDPROP(Vector,               CBaseEntity, m_vecVelocity,          CBaseGrenade);
IMPL_SENDPROP(Vector,               CBaseEntity, m_vecOrigin,            CBaseEntity);
IMPL_SENDPROP(QAngle,               CBaseEntity, m_angRotation,          CBaseEntity);
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
MemberFuncThunk<      CBaseEntity *, void, const Vector&, Vector *                          > CBaseEntity::ft_EntityToWorldSpace         ("CBaseEntity::EntityToWorldSpace");
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

MemberVFuncThunk<      CBaseEntity *, Vector                                                          > CBaseEntity::vt_EyePosition                   (TypeName<CBaseEntity>(), "CBaseEntity::EyePosition");
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
MemberVFuncThunk<      CBaseEntity *, INextBot *                                                      > CBaseEntity::vt_MyNextBotPointer              (TypeName<CBaseEntity>(), "CBaseEntity::MyNextBotPointer");
MemberVFuncThunk<      CBaseEntity *, void, const Vector *, const QAngle *, const Vector *            > CBaseEntity::vt_Teleport                      (TypeName<CBaseEntity>(), "CBaseEntity::Teleport");
MemberVFuncThunk<const CBaseEntity *, int                                                             > CBaseEntity::vt_GetMaxHealth                  (TypeName<CBaseEntity>(), "CBaseEntity::GetMaxHealth");
MemberVFuncThunk<      CBaseEntity *, bool                                                            > CBaseEntity::vt_IsAlive                       (TypeName<CBaseEntity>(), "CBaseEntity::IsAlive");
MemberVFuncThunk<const CBaseEntity *, float                                                           > CBaseEntity::vt_GetDefaultItemChargeMeterValue(TypeName<CBaseEntity>(), "CBaseEntity::GetDefaultItemChargeMeterValue");
MemberVFuncThunk<      CBaseEntity *, bool                                                            > CBaseEntity::vt_IsDeflectable                 (TypeName<CBaseEntity>(), "CBaseEntity::IsDeflectable");
MemberVFuncThunk<      CBaseEntity *, void, CBaseEntity *, int                                        > CBaseEntity::vt_SetParent                     (TypeName<CBaseEntity>(), "CBaseEntity::SetParent");
MemberVFuncThunk<const CBaseEntity *, bool                                                            > CBaseEntity::vt_IsPlayer                      (TypeName<CBaseEntity>(), "CBaseEntity::IsPlayer");
MemberVFuncThunk<const CBaseEntity *, bool                                                            > CBaseEntity::vt_IsBaseObject                  (TypeName<CBaseEntity>(), "CBaseEntity::IsBaseObject");
MemberVFuncThunk<      CBaseEntity *, bool, const char *, const char *                                > CBaseEntity::vt_KeyValue                      (TypeName<CBaseEntity>(), "CBaseEntity::KeyValue");
MemberVFuncThunk<      CBaseEntity *, bool, const char *, char *, int                                 > CBaseEntity::vt_GetKeyValue                   (TypeName<CBaseEntity>(), "CBaseEntity::GetKeyValue");
MemberVFuncThunk<      CBaseEntity *, void, const FireBulletsInfo_t &                                 > CBaseEntity::vt_FireBullets                   (TypeName<CBaseEntity>(), "CBaseEntity::FireBullets");

MemberFuncThunk<CBaseEntityOutput *, void, variant_t, CBaseEntity *, CBaseEntity *, float> CBaseEntityOutput::ft_FireOutput("CBaseEntityOutput::FireOutput");

StaticFuncThunk<CBaseEntity *, const char *, const Vector&, const QAngle&, CBaseEntity *>                        CBaseEntity::ft_Create             ("CBaseEntity::Create");
StaticFuncThunk<CBaseEntity *, const char *, const Vector&, const QAngle&, CBaseEntity *>                        CBaseEntity::ft_CreateNoSpawn      ("CBaseEntity::CreateNoSpawn");
StaticFuncThunk<int, const char *, bool>                                                                         CBaseEntity::ft_PrecacheModel      ("CBaseEntity::PrecacheModel");
StaticFuncThunk<bool, const char *>                                                                              CBaseEntity::ft_PrecacheSound      ("CBaseEntity::PrecacheSound");
StaticFuncThunk<HSOUNDSCRIPTHANDLE, const char *>                                                                CBaseEntity::ft_PrecacheScriptSound("CBaseEntity::PrecacheScriptSound");
StaticFuncThunk<void, IRecipientFilter&, int, const char *, const Vector *, float, float *>                      CBaseEntity::ft_EmitSound_static1  ("CBaseEntity::EmitSound [static: normal]");
StaticFuncThunk<void, IRecipientFilter&, int, const char *, HSOUNDSCRIPTHANDLE&, const Vector *, float, float *> CBaseEntity::ft_EmitSound_static2  ("CBaseEntity::EmitSound [static: normal + handle]");
StaticFuncThunk<void, IRecipientFilter&, int, const EmitSound_t&>                                                CBaseEntity::ft_EmitSound_static3  ("CBaseEntity::EmitSound [static: emitsound]");
StaticFuncThunk<void, IRecipientFilter&, int, const EmitSound_t&, HSOUNDSCRIPTHANDLE&>                           CBaseEntity::ft_EmitSound_static4  ("CBaseEntity::EmitSound [static: emitsound + handle]");

MemberFuncThunk<CCollisionProperty *, void, SurroundingBoundsType_t, const Vector *, const Vector *> ft_SetSurroundingBoundsType("CCollisionProperty::SetSurroundingBoundsType");
MemberFuncThunk<CCollisionProperty *, void> ft_MarkPartitionHandleDirty("CCollisionProperty::MarkPartitionHandleDirty");
MemberFuncThunk<CCollisionProperty *, void, int> ft_SetSolidFlags("CCollisionProperty::SetSolidFlags");

void CCollisionProperty::SetSurroundingBoundsType(SurroundingBoundsType_t type, const Vector *pMins, const Vector *pMaxs) {
	ft_SetSurroundingBoundsType(this, type, pMins, pMaxs);
}

void CCollisionProperty::MarkPartitionHandleDirty() {
	ft_MarkPartitionHandleDirty(this);
}

void CCollisionProperty::SetSolidFlags(int flags) {
	ft_SetSolidFlags(this, flags);
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
	COMPILE_TIME_ASSERT( sizeof(string_t) == sizeof(int) );

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
			const char *pszName = (Entity()) ? STRING(Entity()->GetEntityName()) : "<<null entity>>";
			Q_strncpy( szBuf, pszName, 512 );
			return (szBuf);
		}
	}

	return("No conversion to string");
}