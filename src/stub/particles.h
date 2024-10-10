#ifndef _INCLUDE_SIGSEGV_STUB_PARTICLES_H_
#define _INCLUDE_SIGSEGV_STUB_PARTICLES_H_

#include "link/link.h"

/*class CEffectData
{
public:
	Vector m_vOrigin;
	Vector m_vStart;
	Vector m_vNormal;
	QAngle m_vAngles;
	int		m_fFlags;
	int		m_nEntIndex;
	float	m_flScale;
	float	m_flMagnitude;
	float	m_flRadius;
	int		m_nAttachmentIndex;
	short	m_nSurfaceProp;

	// Some TF2 specific things
	int		m_nMaterial;
	int		m_nDamageType;
	int		m_nHitBox;
	
	unsigned char	m_nColor;

	// Color customizability
	bool							m_bCustomColors;
	te_tf_particle_effects_colors_t	m_CustomColors;

	bool									m_bControlPoint1;
	te_tf_particle_effects_control_point_t	m_ControlPoint1;

// Don't mess with stuff below here. DispatchEffect handles all of this.
public:
	CEffectData()
	{
		m_vOrigin.Init();
		m_vStart.Init();
		m_vNormal.Init();
		m_vAngles.Init();

		m_fFlags = 0;
		m_nEntIndex = 0;
		m_flScale = 1.f;
		m_nAttachmentIndex = 0;
		m_nSurfaceProp = 0;

		m_flMagnitude = 0.0f;
		m_flRadius = 0.0f;

		m_nMaterial = 0;
		m_nDamageType = 0;
		m_nHitBox = 0;

		m_nColor = 0;

		m_bCustomColors = false;
		m_CustomColors.m_vecColor1.Init();
		m_CustomColors.m_vecColor2.Init();

		m_bControlPoint1 = false;
		m_ControlPoint1.m_eParticleAttachment = PATTACH_ABSORIGIN;
		m_ControlPoint1.m_vecOffset.Init();
	}

private:
	int m_iEffectName;
};*/

static StaticFuncThunk<int, const char *> ft_GetParticleSystemIndex("GetParticleSystemIndex");
inline int GetParticleSystemIndex( const char * name)
{
	return ft_GetParticleSystemIndex(name);
}

static StaticFuncThunk<const char *, int> ft_GetParticleSystemNameFromIndex("GetParticleSystemNameFromIndex");
inline const char * GetParticleSystemNameFromIndex(int index)
{
	return ft_GetParticleSystemNameFromIndex(index);
}

static StaticFuncThunk<void, IRecipientFilter&, float, const Vector &, const char *, const CEffectData &> ft_TE_DispatchEffect("TE_DispatchEffect");
inline void TE_DispatchEffect( IRecipientFilter &filter, float delay, const Vector &pos, const char *pName, const CEffectData &data )
{
	ft_TE_DispatchEffect(filter, delay, pos, pName, data);
}

static StaticFuncThunk<void, IRecipientFilter&, float, int, int, const Vector&, const QAngle &, const Vector&, int, int> ft_TE_PhysicsProp("TE_PhysicsProp");
inline void TE_PhysicsProp( IRecipientFilter& filter, float delay, int modelindex, int skin, const Vector& pos, const QAngle &angles, const Vector& vel, int flags, int effects )
{
	ft_TE_PhysicsProp(filter, delay, modelindex, skin, pos, angles, vel, flags, effects);
}

static StaticFuncThunk<void, IRecipientFilter&, float, const Vector &, const QAngle&, const Vector&, const Vector&, int, int, int, float, int> ft_TE_BreakModel("TE_BreakModel");
inline void TE_BreakModel(IRecipientFilter& filter, float delay,
	const Vector& pos, const QAngle &angles, const Vector& size, const Vector& vel, 
	int modelindex, int randomization, int count, float time, int flags)
{
	ft_TE_BreakModel(filter, delay, pos, angles, size, vel, modelindex, randomization, count, time, flags);
}

static StaticFuncThunk<void, IRecipientFilter&, float, const Vector *, const Vector *, int, int, int, int, float, float, float, int, float, int, int, int, int, int> ft_TE_BeamPoints("TE_BeamPoints");
inline void TE_BeamPoints(IRecipientFilter& filter, float delay,
	const Vector *start, const Vector *end, int modelindex, int haloindex, int startframe, int framerate,
	float life, float width, float endWidth, int fadeLength, float amplitude, int r, int g, int b, int a, int speed)
{
	ft_TE_BeamPoints(filter, delay, start, end, modelindex, haloindex, startframe, framerate, life, width, endWidth, fadeLength, amplitude, r, g, b, a, speed);
}

void TE_BeamPointsForDebug(const Vector &start, const Vector &end, float life, int r = 255, int g = 255, int b = 255, int a = 255, float width = 4.0f, CBasePlayer *target = nullptr);

void TE_BBoxForDebug(const Vector &start, const Vector &end, float life, int r = 255, int g = 255, int b = 255, int a = 255, float width = 4.0f, CBasePlayer *target = nullptr);

#ifdef SE_IS_TF2

static StaticFuncThunk<void, IRecipientFilter&, float, const Vector &, const Vector &, int, int, int, int, int> ft_TE_TFExplosion("TE_TFExplosion");
inline void TE_TFExplosion( IRecipientFilter &filter, float flDelay, const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, int nEntIndex, int nDefID = -1, int nSound = 11 /*SPECIAL1*/, int iCustomParticle = INVALID_STRING_INDEX )
{
	ft_TE_TFExplosion(filter, flDelay, vecOrigin, vecNormal, iWeaponID, nEntIndex, nDefID, nSound, iCustomParticle);
}

static StaticFuncThunk<void, IRecipientFilter&, float, const Vector &, const Vector &, int> ft_TE_TFBlood("TE_TFBlood");
inline void TE_TFBlood( IRecipientFilter &filter, float delay, const Vector &pos, const Vector &normal, int entindex)
{
	ft_TE_TFBlood(filter, delay, pos, normal, entindex);
}

static StaticFuncThunk<void, IRecipientFilter&, float, char const*, Vector, QAngle, CBaseEntity*, ParticleAttachment_t> ft_TE_TFParticleEffect("TE_TFParticleEffect");
inline void TE_TFParticleEffect(IRecipientFilter& recipement, float value, char const* name, Vector vector, QAngle angles, CBaseEntity* entity = nullptr, ParticleAttachment_t attach = ParticleAttachment_t())
{
	ft_TE_TFParticleEffect(recipement,value,name,vector,angles,entity,attach);
}

#endif


#ifdef SE_IS_TF2
static StaticFuncThunk<void, IRecipientFilter&,
	float,
	const char *,
	Vector,
	QAngle,
	te_tf_particle_effects_colors_t *,
	te_tf_particle_effects_control_point_t *,
	CBaseEntity *,
	ParticleAttachment_t,
	Vector> ft_TE_TFParticleEffectComplex("TE_TFParticleEffectComplex");

inline void TE_TFParticleEffectComplex
(
	IRecipientFilter &filter,
	float flDelay,
	const char *pszParticleName,
	Vector vecOrigin,
	QAngle vecAngles,
	te_tf_particle_effects_colors_t *pOptionalColors /*= NULL*/,
	te_tf_particle_effects_control_point_t *pOptionalControlPoint1 /*= NULL*/,
	CBaseEntity *pEntity /*= NULL*/,
	ParticleAttachment_t eAttachType /*= PATTACH_CUSTOMORIGIN*/,
	Vector vecStart /* = vec3_origin */
)
{
	ft_TE_TFParticleEffectComplex(filter,flDelay,pszParticleName,vecOrigin,vecAngles,pOptionalColors,pOptionalControlPoint1, pEntity, eAttachType, vecStart);
}
#endif

void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, Vector vecColor1, Vector vecColor2, bool bUseColors, bool bResetAllParticlesOnEntity,  te_tf_particle_effects_control_point_t *controlPoint, IRecipientFilter *pFilter);
void DispatchParticleEffect( const char *pszParticleName, ParticleAttachment_t iAttachType, CBaseEntity *pEntity, const char *pszAttachmentName, Vector vecOrigin, bool hasOrigin, Vector vecColor1, Vector vecColor2, bool bUseColors, bool bResetAllParticlesOnEntity,  te_tf_particle_effects_control_point_t *controlPoint, IRecipientFilter *pFilter);
#endif