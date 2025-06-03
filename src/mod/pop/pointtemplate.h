#ifndef _INCLUDE_SIGSEGV_MOD_POP_POINTTEMPLATE_H_
#define _INCLUDE_SIGSEGV_MOD_POP_POINTTEMPLATE_H_

#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include <boost/algorithm/string.hpp>
#include "util/misc.h"

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


using EntityKeys = std::multimap<std::string,std::string, CaseInsensitiveLess>;
using TemplateParams = std::map<std::string,std::string, CaseInsensitiveLess>;
using FixupNames = std::set<std::string, CaseInsensitiveLess>;
class PointTemplate
{
public:
	std::string name;
	int id = 0;
	std::vector<EntityKeys> entities;
	FixupNames fixup_names;
	bool keep_alive = false;
	bool no_fixup = false;
	std::string remove_if_killed = "";

	std::vector<InputInfoTemplate> on_spawn_triggers;
	std::vector<InputInfoTemplate> on_parent_kill_triggers;
	std::vector<InputInfoTemplate> on_kill_triggers;

	std::shared_ptr<PointTemplateInstance> SpawnTemplate(CBaseEntity *parent, const Vector &translation = vec3_origin, const QAngle &rotation = vec3_angle, bool autoparent = true, const char *attachment=nullptr, bool ignore_parent_alive_state = false, const TemplateParams &params = TemplateParams());
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
	TemplateParams parameters;

	std::vector<InputInfoTemplate> on_spawn_triggers_fixed;
	std::vector<InputInfoTemplate> on_parent_kill_triggers_fixed;
	std::vector<InputInfoTemplate> on_kill_triggers_fixed;

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
	TemplateParams parameters;
	
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
	TemplateParams parameters;
};

PointTemplateInfo Parse_SpawnTemplate(KeyValues *kv);
bool Parse_ShootTemplate(ShootTemplateData &data, KeyValues *kv);

PointTemplate *FindPointTemplate(const std::string &str);
extern std::unordered_map<std::string, PointTemplate, CaseInsensitiveHash, CaseInsensitiveCompare> g_templates;

inline std::unordered_map<std::string, PointTemplate, CaseInsensitiveHash, CaseInsensitiveCompare> &Point_Templates()
{
	return g_templates;
}

extern std::vector<std::shared_ptr<PointTemplateInstance>> g_templateInstances;
extern FixupNames g_fixupNames;

void Clear_Point_Templates();
void Update_Point_Templates();

extern StaticFuncThunk<void> ft_PrecachePointTemplates;
extern StaticFuncThunk<void, int, HierarchicalSpawn_t *, bool> ft_SpawnHierarchicalList;

inline void PrecachePointTemplates()
{
	ft_PrecachePointTemplates();
}

inline void SpawnHierarchicalList(int nEntities, HierarchicalSpawn_t *pSpawnList, bool bActivateEntities)
{
	ft_SpawnHierarchicalList(nEntities, pSpawnList, bActivateEntities);
}

#endif
