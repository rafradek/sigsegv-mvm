#ifndef _INCLUDE_SIGSEGV_MOD_POP_POINTTEMPLATE_H_
#define _INCLUDE_SIGSEGV_MOD_POP_POINTTEMPLATE_H_

#include "link/link.h"
#include "prop.h"

class PointTemplateInstance;

class CTFBotMvMEngineerHintFinder
{
public:
	static bool FindHint(bool box_check, bool out_of_range_ok, CHandle<CTFBotHintEngineerNest> *the_hint) {return ft_FindHint(box_check,out_of_range_ok,the_hint);}
private:
	static StaticFuncThunk<bool, bool, bool, CHandle<CTFBotHintEngineerNest> *>ft_FindHint;
};

class CEventQueue {
public:
	void AddEvent( const char *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID ) {ft_AddEvent(this,target,targetInput,Value,fireDelay,pActivator,pCaller,outputID);}
private:
	static MemberFuncThunk< CEventQueue*, void, const char*,const char *, variant_t, float, CBaseEntity *, CBaseEntity *, int>   ft_AddEvent;
};

static Vector defv=Vector();
static QAngle defa=QAngle();

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
		bool has_on_kill_trigger = false;
		bool no_fixup = false;
		std::vector<std::string> on_kill_triggers = std::vector<std::string>();

		PointTemplateInstance *SpawnTemplate(CBaseEntity *parent, Vector &translation = defv, QAngle &rotation = defa, bool autoparent = true, const char *attachment=nullptr);
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
		void OnKilledParent(bool clearing);
		int attachment=0;
	};
static PointTemplateInstance PointTemplateInstance_Invalid = PointTemplateInstance();

struct PointTemplateInfo
{
	Vector translation = Vector();
	QAngle rotation = QAngle();
	PointTemplate *templ;
	float delay =0.0f;
	std::string attachment;
	PointTemplateInstance *SpawnTemplate(CBaseEntity *parent) {
		if (templ != nullptr)
			return templ->SpawnTemplate(parent,translation,rotation,true,attachment.c_str());
		else
			return nullptr;
	}
};

PointTemplateInfo Parse_SpawnTemplate(KeyValues *kv);

std::unordered_map<std::string, PointTemplate> &Point_Templates();
std::unordered_multimap<std::string, CHandle<CBaseEntity>> &Teleport_Destination();

void Clear_Point_Templates();
void Update_Point_Templates();

extern StaticFuncThunk<void> ft_PrecachePointTemplates;
extern StaticFuncThunk<bool, const Vector&> ft_IsSpaceToSpawnHere;
extern StaticFuncThunk<void, IRecipientFilter&, float, char const*, Vector, QAngle, CBaseEntity*, ParticleAttachment_t> ft_TE_TFParticleEffect;

inline void PrecachePointTemplates()
{
	ft_PrecachePointTemplates();
}

inline void TE_TFParticleEffect(IRecipientFilter& recipement, float value, char const* name, Vector vector, QAngle angles, CBaseEntity* entity = nullptr, ParticleAttachment_t attach = ParticleAttachment_t())
{
	ft_TE_TFParticleEffect(recipement,value,name,vector,angles,entity,attach);
}

inline bool IsSpaceToSpawnHere(const Vector& pos)
{
	return ft_IsSpaceToSpawnHere(pos);
}
extern GlobalThunk<CEventQueue> g_EventQueue;
#endif
