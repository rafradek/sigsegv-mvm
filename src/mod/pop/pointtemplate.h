#ifndef _INCLUDE_SIGSEGV_MOD_POP_POINTTEMPLATE_H_
#define _INCLUDE_SIGSEGV_MOD_POP_POINTTEMPLATE_H_

#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"

#include "link/link.h"
#include "prop.h"

struct HierarchicalSpawn_t
{
	CHandle<CBaseEntity> m_hEntity;
	int			m_nDepth;
	CBaseEntity	*m_pDeferredParent;			// attachment parents can't be set until the parents are spawned
	const char	*m_pDeferredParentAttachment; // so defer setting them up until the second pass
};

class PointTemplateInstance;
typedef void ( *PointTemplateKilledCallback )( PointTemplateInstance * );

struct InputInfoTemplate
{
	std::string target;
	std::string input;
	std::string param;
	float delay;
};

class PointTemplate
{
public:
	std::string name;
	int id = 0;
	std::vector<std::multimap<std::string,std::string>> entities;
	std::set<std::string> fixup_names;
	bool has_parent_name = false;
	bool keep_alive = false;
	bool has_on_kill_parent_trigger = false;
	bool has_on_kill_trigger = false;
	bool no_fixup = false;
	std::string remove_if_killed = "";

	std::vector<std::string> on_parent_kill_triggers = std::vector<std::string>();
	std::vector<std::string> on_kill_triggers = std::vector<std::string>();

	std::shared_ptr<PointTemplateInstance> SpawnTemplate(CBaseEntity *parent, const Vector &translation = vec3_origin, const QAngle &rotation = vec3_angle, bool autoparent = true, const char *attachment=nullptr, bool ignore_parent_alive_state = false);
};


class PointTemplateInstance
{
public:

	int id;
	PointTemplate *templ;
	std::vector<CHandle<CBaseEntity>> entities;
	CHandle<CBaseEntity> parent;
	CHandle<CBaseEntity> parent_helper;
	bool has_parent = false;
	bool mark_delete = false;
	int attachment=0;
	bool is_wave_spawned = false;
	
	void OnKilledParent(bool clearing);
	bool ignore_parent_alive_state = false;
	bool all_entities_killed = false;
	PointTemplateKilledCallback on_kill_callback = nullptr;


};
static PointTemplateInstance PointTemplateInstance_Invalid = PointTemplateInstance();

struct PointTemplateInfo
{
	Vector translation = vec3_origin;
	QAngle rotation = vec3_angle;
	PointTemplate *templ = nullptr;
	float delay =0.0f;
	std::string attachment;
	std::string template_name;
	bool ignore_parent_alive_state = false;
	std::shared_ptr<PointTemplateInstance> SpawnTemplate(CBaseEntity *parent, bool autoparent = true);
};

class ShootTemplateData
{
public:
	bool Shoot(CTFPlayer *player, CTFWeaponBase *weapon);

	PointTemplate *templ = nullptr;
	float speed = 1000.0f;
	float spread = 0.0f;
	bool override_shoot = false;
	Vector offset = Vector(0,0,0);
	QAngle angles = QAngle(0,0,0);
	bool parent_to_projectile = false;
	std::string weapon = "";
	std::string weapon_classname = "";
};

PointTemplateInfo Parse_SpawnTemplate(KeyValues *kv);
bool Parse_ShootTemplate(ShootTemplateData &data, KeyValues *kv);

PointTemplate *FindPointTemplate(std::string &str);
std::unordered_map<std::string, PointTemplate> &Point_Templates();
std::unordered_multimap<std::string, CHandle<CBaseEntity>> &Teleport_Destination();
extern std::set<CHandle<CBaseEntity>> g_pointTemplateParent;
extern std::set<CHandle<CBaseEntity>> g_pointTemplateChild;
extern std::vector<std::shared_ptr<PointTemplateInstance>> g_templateInstances;

void Clear_Point_Templates();
void Update_Point_Templates();

extern StaticFuncThunk<void> ft_PrecachePointTemplates;
extern StaticFuncThunk<void, int, HierarchicalSpawn_t *, bool> ft_SpawnHierarchicalList;
extern StaticFuncThunk<void, IRecipientFilter&, float, char const*, Vector, QAngle, CBaseEntity*, ParticleAttachment_t> ft_TE_TFParticleEffect;
extern StaticFuncThunk<void, IRecipientFilter&,
	float,
	const char *,
	Vector,
	QAngle,
	te_tf_particle_effects_colors_t *,
	te_tf_particle_effects_control_point_t *,
	CBaseEntity *,
	ParticleAttachment_t,
	Vector> ft_TE_TFParticleEffectComplex;

inline void PrecachePointTemplates()
{
	ft_PrecachePointTemplates();
}

inline void TE_TFParticleEffect(IRecipientFilter& recipement, float value, char const* name, Vector vector, QAngle angles, CBaseEntity* entity = nullptr, ParticleAttachment_t attach = ParticleAttachment_t())
{
	ft_TE_TFParticleEffect(recipement,value,name,vector,angles,entity,attach);
}

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

inline void SpawnHierarchicalList(int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities)
{
	ft_SpawnHierarchicalList(nEntities, pSpawnList, bActivateEntities);
}

#endif
