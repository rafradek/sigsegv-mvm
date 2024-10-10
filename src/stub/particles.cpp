#include "stub/particles.h"
#include "link/link.h"


static StaticFuncThunk<void, const char *, ParticleAttachment_t, CBaseEntity *, const char *, bool> ft_DispatchParticleEffect1("DispatchParticleEffect [overload 1]");
void DispatchParticleEffect(const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, bool bResetAllParticlesOnEntity)
{
	ft_DispatchParticleEffect1(pszParticleName, iAttachType, pEntity, pszAttachmentName, bResetAllParticlesOnEntity);
}

static StaticFuncThunk<void, const char *, ParticleAttachment_t, CBaseEntity *, int, bool> ft_DispatchParticleEffect2("DispatchParticleEffect [overload 2]");
void DispatchParticleEffect(const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, int iAttachmentPoint, bool bResetAllParticlesOnEntity)
{
	ft_DispatchParticleEffect2(pszParticleName, iAttachType, pEntity, iAttachmentPoint, bResetAllParticlesOnEntity);
}

static StaticFuncThunk<void, const char *, Vector, QAngle, CBaseEntity *> ft_DispatchParticleEffect3("DispatchParticleEffect [overload 3]");
void DispatchParticleEffect(const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CBaseEntity *pEntity)
{
	ft_DispatchParticleEffect3(pszParticleName, vecOrigin, vecAngles, pEntity);
}

static StaticFuncThunk<void, const char *, Vector, Vector, QAngle, CBaseEntity *> ft_DispatchParticleEffect4("DispatchParticleEffect [overload 4]");
void DispatchParticleEffect(const char *pszParticleName, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity)
{
	ft_DispatchParticleEffect4(pszParticleName, vecOrigin, vecStart, vecAngles, pEntity);
}

static StaticFuncThunk<void, int, Vector, Vector, QAngle, CBaseEntity *> ft_DispatchParticleEffect5("DispatchParticleEffect [overload 5]");
void DispatchParticleEffect(int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CBaseEntity *pEntity)
{
	ft_DispatchParticleEffect5(iEffectIndex, vecOrigin, vecStart, vecAngles, pEntity);
}

static StaticFuncThunk<void, const char *, ParticleAttachment_t, CBaseEntity *, const char *, Vector, Vector, bool, bool> ft_DispatchParticleEffect6("DispatchParticleEffect [overload 6]");
void DispatchParticleEffect(const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, Vector vecColor1, Vector vecColor2, bool bUseColors, bool bResetAllParticlesOnEntity)
{
	ft_DispatchParticleEffect6(pszParticleName, iAttachType, pEntity, pszAttachmentName, vecColor1, vecColor2, bUseColors, bResetAllParticlesOnEntity);
}

static StaticFuncThunk<void, const char *, Vector, QAngle, Vector, Vector, bool, CBaseEntity *, ParticleAttachment_t> ft_DispatchParticleEffect7("DispatchParticleEffect [overload 7]");
void DispatchParticleEffect(const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, Vector vecColor1, Vector vecColor2, bool bUseColors, CBaseEntity *pEntity, ParticleAttachment_t iAttachType)
{
	ft_DispatchParticleEffect7(pszParticleName, vecOrigin, vecAngles, vecColor1, vecColor2, bUseColors, pEntity, iAttachType);
}

static StaticFuncThunk<void, CBaseEntity *> ft_StopParticleEffects("StopParticleEffects");
void StopParticleEffects(CBaseEntity *pEntity)
{
	ft_StopParticleEffects(pEntity);
}

//static StaticFuncThunk<void, IRecipientFilter&, float, const Vector &, const Vector &, int, int, int, int, int> ft_TE_TFExplosion("TE_TFExplosion");
//void TE_TFExplosion( IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, int nEntIndex, int nDefID = -1, int nSound = 11 /*SPECIAL1*/, int iCustomParticle = INVALID_STRING_INDEX )
//{
//	ft_TE_TFExplosion(filter, flDelay, vecOrigin, vecNormal, iWeaponID, nEntIndex, nDefID, nSound, iCustomParticle);
//}

//static StaticFuncThunk<int, const char *> ft_GetParticleSystemIndex("GetParticleSystemIndex");
//int GetParticleSystemIndex( const char * name)
//{
//	return ft_GetParticleSystemIndex(name);
//}
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, Vector vecColor1, Vector vecColor2, bool bUseColors, bool bResetAllParticlesOnEntity,  te_tf_particle_effects_control_point_t *controlPoint, IRecipientFilter *pFilter)
{
	DispatchParticleEffect(pszParticleName, iAttachType, pEntity, pszAttachmentName, vec3_origin, false, vecColor1, vecColor2, bUseColors, bResetAllParticlesOnEntity, controlPoint, pFilter);
}

void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, Vector vecOrigin, bool hasOrigin, Vector vecColor1, Vector vecColor2, bool bUseColors, bool bResetAllParticlesOnEntity,  te_tf_particle_effects_control_point_t *controlPoint, IRecipientFilter *pFilter)
{
	int iAttachment = -1;
	if ( pEntity && pszAttachmentName != nullptr)
	{
		CBaseAnimating *animating = rtti_cast<CBaseAnimating *>(pEntity);
		if (animating != nullptr) {
			// Find the attachment point index
			iAttachment =animating->LookupAttachment( pszAttachmentName );
			if ( iAttachment <= 0 )
			{
				return;
			}
		}
	}

	CEffectData	data;

	data.m_nHitBox = GetParticleSystemIndex( pszParticleName );

	if (hasOrigin)
		data.m_vOrigin = vecOrigin;

	if ( pEntity )
	{
		data.m_nEntIndex = pEntity->entindex();
		data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
		if (!hasOrigin)
			data.m_vOrigin = pEntity->GetAbsOrigin();
	}
	data.m_nDamageType = iAttachType;
	data.m_nAttachmentIndex = iAttachment;

	if ( bResetAllParticlesOnEntity )
	{
		data.m_fFlags |= PARTICLE_DISPATCH_RESET_PARTICLES;
	}

	if ( bUseColors )
	{
		data.m_bCustomColors = true;
		data.m_CustomColors.m_vecColor1 = vecColor1;
		data.m_CustomColors.m_vecColor2 = vecColor2;
	}
	if (controlPoint != nullptr) {
		data.m_bControlPoint1 = true;
		data.m_ControlPoint1.m_eParticleAttachment = controlPoint->m_eParticleAttachment;
		data.m_ControlPoint1.m_vecOffset = controlPoint->m_vecOffset;
	}

	if (pFilter != nullptr) {
		TE_DispatchEffect( *pFilter, 0.0f, data.m_vOrigin, "ParticleEffect", data);
	}
	else if ( ( data.m_fFlags & PARTICLE_DISPATCH_FROM_ENTITY ) != 0 &&
		 ( iAttachType == PATTACH_ABSORIGIN_FOLLOW || iAttachType == PATTACH_POINT_FOLLOW || iAttachType == PATTACH_ROOTBONE_FOLLOW ) )
	{
		CReliableBroadcastRecipientFilter filter;
		TE_DispatchEffect( filter, 0.0f, data.m_vOrigin, "ParticleEffect", data);
	}
	else
	{
		CPASFilter filter(data.m_vOrigin);
		TE_DispatchEffect( filter, 0.0f, data.m_vOrigin, "ParticleEffect", data);
	}
}

int laserSprite;

void TE_BeamPointsForDebug(const Vector &start, const Vector &end, float life, int r, int g, int b, int a, float width, CBasePlayer *target) {
	CRecipientFilter filter;
	if (target != nullptr) {
		filter.AddRecipient(target);
	}
	else {
		filter.AddAllPlayers();
	}
	TE_BeamPoints(filter, 0.0f, &start, &end, laserSprite, 0, 0, 0, life, width, width, 7, 0.0, r, g, b, a, 0);
}

void TE_BBoxForDebug(const Vector &mins, const Vector &maxs, float life, int r, int g, int b, int a, float width, CBasePlayer *target) {
	Vector pos1;
	Vector pos2;
	pos1[0] = mins[0];
	pos1[1] = mins[1];
	pos1[2] = mins[2];
	pos2[0] = maxs[0];
	pos2[1] = mins[1];
	pos2[2] = mins[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	pos2[0] = mins[0];
	pos2[1] = maxs[1];
	pos2[2] = mins[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	pos2[0] = mins[0];
	pos2[1] = mins[1];
	pos2[2] = maxs[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);

	pos1[0] = maxs[0];
	pos1[1] = maxs[1];
	pos1[2] = maxs[2];
	pos2[0] = maxs[0];
	pos2[1] = maxs[1];
	pos2[2] = mins[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	pos2[0] = mins[0];
	pos2[1] = maxs[1];
	pos2[2] = maxs[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	pos2[0] = maxs[0];
	pos2[1] = mins[1];
	pos2[2] = maxs[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	
	pos1[0] = mins[0];
	pos1[1] = mins[1];
	pos1[2] = maxs[2];
	pos2[0] = maxs[0];
	pos2[1] = mins[1];
	pos2[2] = maxs[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	pos2[0] = mins[0];
	pos2[1] = maxs[1];
	pos2[2] = maxs[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	
	pos1[0] = maxs[0];
	pos1[1] = maxs[1];
	pos1[2] = mins[2];
	pos2[0] = mins[0];
	pos2[1] = maxs[1];
	pos2[2] = mins[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	pos2[0] = maxs[0];
	pos2[1] = mins[1];
	pos2[2] = mins[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	
	pos1[0] = maxs[0];
	pos1[1] = mins[1];
	pos1[2] = mins[2];
	pos2[0] = maxs[0];
	pos2[1] = mins[1];
	pos2[2] = maxs[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
	pos1[0] = mins[0];
	pos1[1] = maxs[1];
	pos1[2] = mins[2];
	pos2[0] = mins[0];
	pos2[1] = maxs[1];
	pos2[2] = maxs[2];
	TE_BeamPointsForDebug(pos1, pos2, life, r, g, b, a, width, target);
}