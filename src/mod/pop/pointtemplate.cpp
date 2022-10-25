#include "mod.h"
#include "stub/entities.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "stub/extraentitydata.h"
#include "mod/pop/pointtemplate.h"
#include "util/misc.h"
#include "util/iterate.h"
#include "util/backtrace.h"
#include "stub/misc.h"
#include "stub/tfweaponbase.h"
#include <fmt/core.h>
int Template_Increment;


#define TEMPLATE_BRUSH_MODEL "models/weapons/w_models/w_rocket.mdl"

ConVar fast_whole_map_trigger("sig_pop_pointtemplate_fast_whole_map_trigger", "1", FCVAR_NOTIFY,
		"Mod: Make whole map triggers faster");

class TemplateParentModule : public EntityModule
{
public:
	TemplateParentModule() {}
	TemplateParentModule(CBaseEntity *entity) {}
};

class TemplateEntityModule : public EntityModule
{
public:
	TemplateEntityModule() {}
	TemplateEntityModule(CBaseEntity *entity) : entity(entity) {}
	
	CBaseEntity *entity = nullptr;
	bool deleteAllOnRemove = false;

	std::shared_ptr<PointTemplateInstance> inst;
};

class WholeMapTriggerModule : public EntityModule, public AutoList<WholeMapTriggerModule>
{
public:
	WholeMapTriggerModule() {}
	WholeMapTriggerModule(CBaseEntity *entity) : entity(entity) {}
	
	CBaseEntity *entity = nullptr;
	std::unordered_map<std::string, std::pair<variant_t, variant_t>> props;
};

void TriggerList(CBaseEntity *activator, std::vector<InputInfoTemplate> &triggers, PointTemplateInstance *inst);

void FixupKeyvalue(std::string &val,int id, const char *parentname, FixupNames &matching) {
	size_t amperpos = 0;
	while((amperpos = val.find('\1',amperpos)) != std::string::npos){
		size_t endNamePos = val.find('\1',amperpos+1);
		if (endNamePos != std::string::npos) {
			std::string entName = val.substr(amperpos+1, endNamePos - amperpos - 1);

			// Find if there is already an entity with that name, if yes, fix up the name, otherwise delete the fixup marker 
			// bool found = false;
			// for (CBaseEntity* entity = nullptr; (entity = servertools->FindEntityByName(entity, entName.c_str())) != nullptr ; ) {
			// 	if (std::find(matching.begin(), matching.end(), entity) == matching.end()) {
			// 		found = true;
			// 		break;
			// 	}
			// }
			if (matching.contains(entName)) {
				val[endNamePos] = '\1';
				val.insert(endNamePos+1,std::to_string(id));
			}
			else {
				val.erase(endNamePos, 1);
			}
			val.erase(amperpos, 1);
		}
		else {
			amperpos++;
		}
	}
		
	size_t parpos = 0;
	while((parpos = val.find("!parent",parpos)) != std::string::npos){
		val.replace(parpos,7,parentname);
	}
}

void FixupParameters(std::string &val, const TemplateParams &params) {
	if (params.empty()) return;

	size_t parampos = 0;
	while((parampos = val.find('$',parampos)) != std::string::npos){
		size_t endNamePos = val.find('$',parampos+1);
		if (endNamePos != std::string::npos) {
			if (endNamePos - parampos > 1) {
				std::string paramName = val.substr(parampos+1, endNamePos - parampos - 1);
				auto paramFind = params.find(paramName);
				if (paramFind != params.end()) {
					val.replace(parampos, paramName.size()+2, paramFind->second);
					endNamePos = parampos + paramFind->second.size();
				}
			}
		}
		else {
			break;
		}
		parampos = endNamePos;
	}
}

void FixupTriggers(const std::vector<InputInfoTemplate> &triggers, PointTemplateInstance *inst, std::vector<InputInfoTemplate> &triggersFixed, FixupNames &names)
{
	for (auto &inputinfo : triggers) {
		std::string target = inputinfo.target;
		std::string param = inputinfo.param;
		if (!inst->templ->no_fixup) {
			auto parentname = inst->parent != nullptr ? fmt::format("@h@{}", inst->parent->GetRefEHandle().ToInt()) : "";
			FixupKeyvalue(target,inst->id,parentname.c_str(), names);
			FixupKeyvalue(param,inst->id,parentname.c_str(), names);
		}
		FixupParameters(target, inst->parameters);
		FixupParameters(param, inst->parameters);
		triggersFixed.push_back({target, inputinfo.input, param, inputinfo.delay});
	}
}

void SpawnEntity(CBaseEntity *entity) {

	/* Set infinite lifetime for target dummies */
	//static ConVarRef lifetime("tf_target_dummy_lifetime");
	//float prelifetime = lifetime.GetFloat();
	//lifetime.SetValue(99999.0f);

	servertools->DispatchSpawn(entity);

	//lifetime.SetValue(30.0f);
}


struct BrushEntityBoundingBox 
{
	BrushEntityBoundingBox(CBaseEntity *entity, std::string &min, std::string &max) : entity(entity), min(min), max(max) {}

	CBaseEntity *entity;
	std::string &min;
	std::string &max;
};

	
bool TriggerCollideable(CBaseEntity *entity)
{
	return entity->GetSolid() != SOLID_NONE && !entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER) && !entity->CollisionProp()->IsSolidFlagSet(FSOLID_NOT_SOLID);
}

std::shared_ptr<PointTemplateInstance> PointTemplate::SpawnTemplate(CBaseEntity *parent, const Vector &translation, const QAngle &rotation, bool autoparent, const char *attachment, bool ignore_parent_alive_state, const TemplateParams &params) {

	Template_Increment +=1;
	if (Template_Increment > 999999)
		Template_Increment = 0;

	
	auto templ_inst = std::make_shared<PointTemplateInstance>();
	g_templateInstances.push_back(templ_inst);
	templ_inst->templ = this;
	templ_inst->parent = parent;
	templ_inst->id = Template_Increment;
	templ_inst->has_parent = parent != nullptr;
	templ_inst->ignore_parent_alive_state = ignore_parent_alive_state;
	templ_inst->parameters = params;

	std::string parentname;
	CBaseEntity* parent_helper = parent;

	if (parent != nullptr){

		int bone = -1;
		if (attachment != nullptr) {
			CBaseAnimating *animating = rtti_cast<CBaseAnimating *>(parent);
			if (animating != nullptr)
				bone = animating->LookupBone(attachment);
		}
		templ_inst->attachment = bone;

 		if(parent->IsPlayer() && autoparent && !this->entities.empty()){
			parent_helper = CreateEntityByName("point_teleport");
			parent_helper->SetAbsOrigin(parent->GetAbsOrigin());
			parent_helper->SetAbsAngles(parent->GetAbsAngles());
			parent_helper->Spawn();
			parent_helper->Activate();
			templ_inst->entities.push_back(parent_helper);
			templ_inst->parent_helper = parent_helper;
		}

		parentname = fmt::format("@h@{}", parent->GetRefEHandle().ToInt());
		parent->AddEntityModule("parenttemplatemodule", new TemplateParentModule());
	}
	else
	{
		parentname = "";
	}
	
	std::unordered_map<std::string,CBaseEntity*> spawned;
	std::vector<std::pair<CBaseEntity*, string_t>> parent_string_restore;

	HierarchicalSpawn_t *list_spawned = new HierarchicalSpawn_t[this->entities.size()];

	int num_entity = 0;

	std::vector<BrushEntityBoundingBox> brush_entity_bounding_box;

	for (auto it = this->entities.begin(); it != this->entities.end(); ++it){
		auto &keys = *it;
		CBaseEntity *entity = CreateEntityByName(keys.find("classname")->second.c_str());
		if (entity != nullptr) {
			templ_inst->entities.push_back(entity);
			auto mod = entity->GetOrCreateEntityModule<TemplateEntityModule>("templatemodule");
			mod->inst = templ_inst;
			for (auto it1 = keys.begin(); it1 != keys.end(); ++it1){
				std::string val = it1->second;

				if (!this->no_fixup)
					FixupKeyvalue(val,Template_Increment,parentname.c_str(), g_fixupNames);
					
				FixupParameters(val, params);

				servertools->SetKeyValue(entity, it1->first.c_str(), val.c_str());
			}

			auto itname = keys.find("targetname");
			if (itname != keys.end()){
				if (this->remove_if_killed != "" && itname->second == this->remove_if_killed) {
					mod->deleteAllOnRemove = true;
				}
				spawned[itname->second]=entity;
			}
			
			Vector translated = vec3_origin;
			QAngle rotated = vec3_angle;
			VectorAdd(entity->GetAbsOrigin(),translation,translated);
			VectorAdd(entity->GetAbsAngles(),rotation,rotated);
			if (parent != nullptr && autoparent) {
						
				VMatrix matEntityToWorld,matNewTemplateToWorld, matStoredLocalToWorld;
				matEntityToWorld.SetupMatrixOrgAngles( translated, rotated );
				matNewTemplateToWorld.SetupMatrixOrgAngles( parent->GetAbsOrigin(), parent->GetAbsAngles() );
				MatrixMultiply( matNewTemplateToWorld, matEntityToWorld, matStoredLocalToWorld );

				Vector origin;
				QAngle angles;
				origin = matStoredLocalToWorld.GetTranslation();
				MatrixToAngles( matStoredLocalToWorld, angles );
				entity->SetAbsOrigin(origin);
				entity->SetAbsAngles(angles);

				if (keys.find("parentname") == keys.end()){
					entity->SetParent(parent_helper, -1);
					
					// Set string to NULL to prevent SpawnHierarchicalList from messing up parenting with the spawntemplate parent
					parent_string_restore.push_back({entity, entity->m_iParent});
					entity->m_iParent = NULL_STRING;
				}
			}
			else
			{
				entity->Teleport(&translated,&rotated,&vec3_origin);
			}

			/*if (keys.find("parentname") != keys.end()) {
				std::string parstr = keys.find("parentname")->second;
				CBaseEntity *parentlocal = spawned[parstr];
				if (parentlocal != nullptr) {
					entity->SetParent(parentlocal, -1);
				}
			}*/
			
			list_spawned[num_entity].m_hEntity = entity;
			list_spawned[num_entity].m_nDepth = 0;
			list_spawned[num_entity].m_pDeferredParentAttachment = NULL;
			list_spawned[num_entity].m_pDeferredParent = NULL;
			
			//To make brush entities working
			if (keys.find("mins") != keys.end() && keys.find("maxs") != keys.end()){
				brush_entity_bounding_box.push_back({entity, keys.find("mins")->second, keys.find("maxs")->second});
			}
			num_entity++;
		}
		
	}
	
	SpawnHierarchicalList(num_entity, list_spawned, true);

	for (auto &box : brush_entity_bounding_box) {
		box.entity->SetModel(TEMPLATE_BRUSH_MODEL);
		box.entity->SetSolid(SOLID_BBOX);
		Vector min, max;
		UTIL_StringToVector(min.Base(), box.min.c_str());
		UTIL_StringToVector(max.Base(), box.max.c_str());
		//sscanf(box.min.c_str(), "%f %f %f", &min.x, &min.y, &min.z);
		//sscanf(box.max.c_str(), "%f %f %f", &max.x, &max.y, &max.z);
		if (fast_whole_map_trigger.GetBool() && ((max.z - min.z) * (max.y - min.y) * (max.x - min.x)) > 4500.0f * 4500.0f * 4500.0f  && box.entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
			min = vec3_origin;
			max = vec3_origin;
			box.entity->SetAbsOrigin(Vector(-30000,-30000,-30000));
			box.entity->GetOrCreateEntityModule<WholeMapTriggerModule>("wholemaptrigger");
			ForEachEntity([&](CBaseEntity *entity) {
				if (TriggerCollideable(entity)) {
					box.entity->StartTouch(entity);
				}
			});
		}
		box.entity->CollisionProp()->SetCollisionBounds(min, max);
		box.entity->AddEffects(32); //DONT RENDER
		if (box.entity->GetMoveParent() != nullptr) {
			box.entity->CollisionProp()->AddSolidFlags(FSOLID_ROOT_PARENT_ALIGNED);
		}
	}

	// Restore parenting string
	for (auto &pair : parent_string_restore) {
		pair.first->m_iParent = pair.second;
	}

	FixupTriggers(this->on_spawn_triggers, templ_inst.get(), templ_inst->on_spawn_triggers_fixed, g_fixupNames);
	FixupTriggers(this->on_parent_kill_triggers, templ_inst.get(), templ_inst->on_parent_kill_triggers_fixed, g_fixupNames);
	FixupTriggers(this->on_kill_triggers, templ_inst.get(), templ_inst->on_kill_triggers_fixed, g_fixupNames);
	if (!templ_inst->on_spawn_triggers_fixed.empty()) {
		TriggerList(parent != nullptr ? parent : UTIL_EntityByIndex(0), templ_inst->on_spawn_triggers_fixed, templ_inst.get());
	}

	for (auto &str : this->fixup_names) {
		g_fixupNames.insert(str);
	}

	delete[] list_spawned;

	return templ_inst;
}

/// @brief Spawns template specified by the object
/// @param parent 
/// @param autoparent 
/// @return The shared pointer to template instance, returns null pointer if template is invalid
std::shared_ptr<PointTemplateInstance> PointTemplateInfo::SpawnTemplate(CBaseEntity *parent, bool autoparent){
	if (templ == nullptr && template_name.size() > 0)
		templ = FindPointTemplate(template_name);
	//DevMsg("Is templ null %d\n",templ == nullptr);
	if (templ != nullptr)
		return templ->SpawnTemplate(parent,translation,rotation,autoparent,attachment.c_str(), ignore_parent_alive_state, parameters);
	else
		return nullptr;
}

bool ShootTemplateData::Shoot(CTFPlayer *player, CTFWeaponBase *weapon) {

	Vector vecSrc;
	QAngle angForward;
	Vector vecOffset( 23.5f, 12.0f, -3.0f );
	vecOffset += this->offset;
	if ( player->GetFlags() & FL_DUCKING )
	{
		vecOffset.z = 8.0f;
	}
	weapon->GetProjectileFireSetup( player, vecOffset, &vecSrc, &angForward, false ,2000);
	
	angForward += this->angles;

	auto inst = this->templ->SpawnTemplate(player, vecSrc, angForward, false, nullptr, false, parameters);
	for (auto entity : inst->entities) {
		Vector vForward,vRight,vUp;
		QAngle angSpawnDir( angForward );
		AngleVectors( angSpawnDir, &vForward, &vRight, &vUp );
		Vector vecShootDir = vForward;
		vecShootDir += vRight * RandomFloat(-1, 1) * this->spread;
		vecShootDir += vForward * RandomFloat(-1, 1) * this->spread;
		vecShootDir += vUp * RandomFloat(-1, 1) * this->spread;
		VectorNormalize( vecShootDir );
		vecShootDir *= this->speed;

		// Apply it to the entity
		IPhysicsObject *pPhysicsObject = entity->VPhysicsGetObject();
		if ( pPhysicsObject )
		{
			pPhysicsObject->AddVelocity(&vecShootDir, NULL);
		}
		else
		{
			entity->SetAbsVelocity( vecShootDir );
		}
	}

	return this->override_shoot;
}

PointTemplateInfo Parse_SpawnTemplate(KeyValues *kv) {
	PointTemplateInfo info;
	bool hasname = false;
	
	FOR_EACH_SUBKEY(kv, subkey) {
		hasname = true;
		const char *name = subkey->GetName();
		if (FStrEq(name, "Name")){
			info.template_name = subkey->GetString();
		}
		else if (FStrEq(name, "Origin")) {
			sscanf(subkey->GetString(),"%f %f %f",&info.translation.x,&info.translation.y,&info.translation.z);
		}
		else if (FStrEq(name, "Angles")) {
			sscanf(subkey->GetString(),"%f %f %f",&info.rotation.x,&info.rotation.y,&info.rotation.z);
		}
		else if (FStrEq(name, "Delay")) {
			info.delay = subkey->GetFloat();
		}
		else if (FStrEq(name, "Bone")) {
			info.attachment = subkey->GetString();
		}
		else if (FStrEq(name, "Params")) {
			FOR_EACH_SUBKEY(subkey, subkey2) {
				info.parameters[subkey2->GetName()] = subkey2->GetString();
			}
		}
	}
	if (!hasname && kv->GetString() != nullptr) {
		info.template_name = kv->GetString();
	}

	if (info.template_name == "") {
		Warning("Parse_SpawnTemplate: missing template name\n");
	}

	//To lowercase
	info.templ = FindPointTemplate(info.template_name);

	if (info.templ == nullptr) {
		Warning("Parse_SpawnTemplate: template (%s) does not exist\n", info.template_name.c_str());
	}

	return info;
}

bool Parse_ShootTemplate(ShootTemplateData &data, KeyValues *kv)
{
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();

		if (FStrEq(name, "Speed")) {
			data.speed = subkey->GetFloat();
		}
		else if (FStrEq(name, "Spread")){
			data.spread = subkey->GetFloat();
		}
		else if (FStrEq(name, "Offset")){
			sscanf(subkey->GetString(),"%f %f %f",&data.offset.x,&data.offset.y,&data.offset.z);
		}
		else if (FStrEq(name, "Angles")) {
			sscanf(subkey->GetString(),"%f %f %f",&data.angles.x,&data.angles.y,&data.angles.z);
		}
		else if (FStrEq(name, "OverrideShoot")) {
			data.override_shoot = subkey->GetBool();
		}
		else if (FStrEq(name, "AttachToProjectile")) {
			data.parent_to_projectile = subkey->GetBool();
		}
		else if (FStrEq(name, "Name")) {
			std::string tname = subkey->GetString();
			data.templ = FindPointTemplate(tname);
		}
		else if (FStrEq(name, "ItemName")) {
			data.weapon = subkey->GetString();
		}
		else if (FStrEq(name, "Classname")) {
			data.weapon_classname = subkey->GetString();
		}
		else if (FStrEq(name, "Params")) {
			FOR_EACH_SUBKEY(subkey, subkey2) {
				data.parameters[subkey2->GetName()] = subkey2->GetString();
			}
		}
	}
	return data.templ != nullptr;
}

PointTemplate *FindPointTemplate(const std::string &str) {

	auto it = Point_Templates().find(str);
	if (it != Point_Templates().end())
		return &(it->second);

	return nullptr;
}

void TriggerList(CBaseEntity *activator, std::vector<InputInfoTemplate> &triggers, PointTemplateInstance *inst)
{

	//CBaseEntity *trigger = CreateEntityByName("logic_relay");
	//variant_t variant1;
	for (auto &inputinfo : triggers) {
		variant_t variant;
		//if (!param.empty()) {
			variant.SetString(AllocPooledString(inputinfo.param.c_str()));
		//}
		//else {
		//	variant.SetString(NULL_STRING);
		//}
		g_EventQueue.GetRef().AddEvent( STRING(AllocPooledString(inputinfo.target.c_str())), STRING(AllocPooledString(inputinfo.input.c_str())), variant, inputinfo.delay, activator, activator, -1);
	}
	/*for(auto it = triggers.begin(); it != triggers.end(); it++){
		std::string val = *(it); 
		if (!inst->templ->no_fixup)
			FixupKeyvalue(val,inst->id,"");
		trigger->KeyValue("ontrigger",val.c_str());

	}

	trigger->KeyValue("spawnflags", "2");
	servertools->DispatchSpawn(trigger);
	trigger->Activate();
	if (activator != nullptr && activator->IsPlayer())
		trigger->AcceptInput("trigger", activator, activator ,variant1,-1);
	else
		trigger->AcceptInput("trigger", UTIL_EntityByIndex(0),UTIL_EntityByIndex(0),variant1,-1);
	servertools->RemoveEntity(trigger);*/
}

void PointTemplateInstance::OnKilledParent(bool cleared) {

	if (this->templ == nullptr || this->mark_delete) {
		this->mark_delete = true;
		DevMsg("template null or deleted\n");
		return;
	}

	if (!cleared && !this->templ->on_parent_kill_triggers.empty()) {
		TriggerList(this->parent != nullptr && this->parent->IsPlayer() ? this->parent.Get() : UTIL_EntityByIndex(0), this->on_parent_kill_triggers_fixed, this);
	}

	for(auto it = this->entities.begin(); it != this->entities.end(); it++){
		if (*(it) != nullptr){
			if (cleared || !(this->templ->keep_alive)){
				
				servertools->RemoveEntity(*(it));
			}
			else {
				CBaseEntity *ent =*(it);
				CBaseEntity *parent = this->parent;
				if (this->parent != nullptr && ent != nullptr && ent->GetMoveParent() == this->parent){
					ent->SetParent(nullptr, -1);
				}
			}
		}
	}

	this->mark_delete = !this->templ->keep_alive || cleared;
	
	if (this->mark_delete && on_kill_callback != nullptr) {
		(*on_kill_callback)(this);
	}
	
	this->parent = nullptr;
	this->has_parent = false;
}

std::unordered_map<std::string, PointTemplate, CaseInsensitveHash, CaseInsensitveCompare> g_templates;

std::vector<std::shared_ptr<PointTemplateInstance>> g_templateInstances;
FixupNames g_fixupNames;

void Clear_Point_Templates()
{
	for(auto it = g_templateInstances.begin(); it != g_templateInstances.end(); it++){
		auto inst = *(it);
		inst->OnKilledParent(true);
	}
	g_templateInstances.clear();
	g_fixupNames.clear();
	Point_Templates().clear();
}

void Update_Point_Templates()
{
	for(auto it = g_templateInstances.begin(); it != g_templateInstances.end(); it++){
		auto inst = *(it);
		if (!inst->mark_delete) {
			if (inst->has_parent && (inst->parent == nullptr || inst->parent->IsMarkedForDeletion() || !(inst->parent->IsAlive() || inst->ignore_parent_alive_state) )) {
				inst->OnKilledParent(false);
			}
			if (!inst->all_entities_killed) {
				bool hasalive = false;
				for(auto it = inst->entities.begin(); it != inst->entities.end(); it++){
					CHandle<CBaseEntity> &ent = *(it);
					if (ent != nullptr){
						hasalive = true;
						break;
					}
				}
				if (!hasalive)
				{
					inst->all_entities_killed = true;
					TriggerList(inst->parent != nullptr && inst->parent->IsPlayer() ? inst->parent.Get() : UTIL_EntityByIndex(0), inst->on_kill_triggers_fixed, inst.get());
				}
			}
			if (inst->all_entities_killed && !((inst->has_parent || inst->is_wave_spawned) && !inst->on_parent_kill_triggers_fixed.empty())) {
				inst->OnKilledParent(true);
			}
		}
		if (inst->parent_helper != nullptr && (inst->mark_delete || inst->parent_helper->FirstMoveChild() == nullptr)) {
			inst->parent_helper->Remove();
			inst->parent_helper = nullptr;
		}
		if (inst->mark_delete) {
			g_templateInstances.erase(it);
			it--;
			continue;
		}

		if (inst->parent_helper != nullptr && inst->parent != nullptr) {
			//DevMsg("Setting parent helper pos %f, parent pos %f\n",it->parent_helper->GetAbsOrigin().x, it->parent->GetAbsOrigin().x);
			Vector pos;
			QAngle ang;
			CBaseEntity *parent =inst->parent;
			if (inst->attachment != -1){
				CBaseAnimating *anim = rtti_cast<CBaseAnimating *>(parent);
				anim->GetBonePosition(inst->attachment,pos,ang);
			}
			else{
                matrix3x4_t matWorldToMeasure;
                MatrixInvert( parent->EntityToWorldTransform(), matWorldToMeasure );
                matrix3x4_t matNewTargetToWorld;
		        MatrixInvert( matWorldToMeasure, matNewTargetToWorld );
		        MatrixAngles( matNewTargetToWorld, ang, pos );
				pos = parent->GetAbsOrigin();
				ang = parent->GetAbsAngles();
			}
			inst->parent_helper->SetAbsOrigin(pos);
			inst->parent_helper->SetAbsAngles(ang);
			
			//if (it->entities[1] != nullptr)
			//	DevMsg("childpos %f %d %d\n",it->entities[1]->GetAbsOrigin().x, it->entities[1]->GetMoveParent() != nullptr, it->entities[1]->GetMoveParent() == it->parent_helper);
		}
	}
}

StaticFuncThunk<void> ft_PrecachePointTemplates("PrecachePointTemplates");
StaticFuncThunk<void, int, HierarchicalSpawn_t *, bool> ft_SpawnHierarchicalList("SpawnHierarchicalList");

namespace Mod::Pop::PointTemplate
{
	/* Prevent additional CUpgrades entities from pointtemplate from taking over global entity*/
	DETOUR_DECL_MEMBER(void, CUpgrades_Spawn)
	{
		CUpgrades *prev = g_hUpgradeEntity.GetRef();
		DETOUR_MEMBER_CALL(CUpgrades_Spawn)();

		if (prev != nullptr && !prev->IsMarkedForDeletion()) {
			g_hUpgradeEntity.GetRef() = prev;
		}
	}

	void OnDestroyUpgrades(CUpgrades *upgrades)
	{
		CUpgrades *prev = g_hUpgradeEntity.GetRef();
		// Choose a different upgrade station
		if (prev == upgrades ) {
			ForEachEntityByRTTI<CUpgrades>([&](CUpgrades *upgrades2) {
				if (upgrades2 != upgrades && !upgrades2->IsMarkedForDeletion()) {
					g_hUpgradeEntity.GetRef() = upgrades2;
				}
			});
		}
	}
	/* */
	DETOUR_DECL_MEMBER(void, CUpgrades_D2)
	{
		OnDestroyUpgrades(reinterpret_cast<CUpgrades *>(this));

		DETOUR_MEMBER_CALL(CUpgrades_D2)();
	}

	DETOUR_DECL_MEMBER(void, CUpgrades_D0)
	{
		OnDestroyUpgrades(reinterpret_cast<CUpgrades *>(this));

		DETOUR_MEMBER_CALL(CUpgrades_D0)();
	}

	
	/* Pointtemplate keep child entities after parent removal*/
	DETOUR_DECL_MEMBER(void, CBaseEntity_UpdateOnRemove)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);

		if (entity->FirstMoveChild() != nullptr && entity->GetEntityModule<TemplateParentModule>("parenttemplatemodule") != nullptr) {

			CBaseEntity *child = entity->FirstMoveChild();

			std::vector<CBaseEntity *> childrenToRemove;
			
			do {
				if (child->GetEntityModule<TemplateEntityModule>("templatemodule") != nullptr) {
					childrenToRemove.push_back(child);
				}
			} 
			while ((child = child->NextMovePeer()) != nullptr);

			for (auto childToRemove : childrenToRemove) {
				childToRemove->SetParent(nullptr, -1);
			}
		}
		auto templMod = entity->GetEntityModule<TemplateEntityModule>("templatemodule");
		
		if (templMod != nullptr)
		{
			if (templMod->deleteAllOnRemove) {
				for (auto entityinst : templMod->inst->entities) {
					if (entityinst != nullptr && entityinst != entity)
						entityinst->Remove();
				}
			}
		}

		if (!WholeMapTriggerModule::List().empty() && TriggerCollideable(entity)) {
			for (auto mod : WholeMapTriggerModule::List()) {
				if (mod->entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
					mod->entity->EndTouch(entity);
				}
			}
		}
		DETOUR_MEMBER_CALL(CBaseEntity_UpdateOnRemove)();
	}

	CBaseEntity *templateTargetEntity = nullptr;
	bool SpawnOurTemplate(CEnvEntityMaker* maker, Vector vector, QAngle angles)
	{
		std::string src = STRING((string_t)maker->m_iszTemplate);
		//DevMsg("Spawning template %s\n", src.c_str());
		auto tmpl = FindPointTemplate(src);
		if (tmpl != nullptr) {
			
			bool autoparent = maker->GetCustomVariableFloat<"autoparent">();
			if (autoparent) {
				vector = vec3_origin;
				angles = vec3_angle;
			}
			TemplateParams params;
			if (maker->GetExtraEntityData() != nullptr) {
				for (auto &variable : maker->GetExtraEntityData()->GetCustomVariables()) {
					if (StringStartsWith(STRING(variable.key), "Param", false)) {
						params[STRING(variable.key)+5] = variable.value.String();
					}
				}
			}
			auto inst = tmpl->SpawnTemplate(templateTargetEntity,vector,angles, autoparent, nullptr, false, params);
			for (auto entity : inst->entities) {
				if (entity == nullptr)
					continue;

				if (entity->GetMoveType() == MOVETYPE_NONE)
					continue;

				// Calculate a velocity for this entity
				Vector vForward,vRight,vUp;
				QAngle angSpawnDir( maker->m_angPostSpawnDirection );
				if ( maker->m_bPostSpawnUseAngles )
				{
					if ( entity->GetMoveParent()  )
					{
						angSpawnDir += entity->GetMoveParent()->GetAbsAngles();
					}
					else
					{
						angSpawnDir += entity->GetAbsAngles();
					}
				}
				AngleVectors( angSpawnDir, &vForward, &vRight, &vUp );
				Vector vecShootDir = vForward;
				vecShootDir += vRight * RandomFloat(-1, 1) * maker->m_flPostSpawnDirectionVariance;
				vecShootDir += vForward * RandomFloat(-1, 1) * maker->m_flPostSpawnDirectionVariance;
				vecShootDir += vUp * RandomFloat(-1, 1) * maker->m_flPostSpawnDirectionVariance;
				VectorNormalize( vecShootDir );
				vecShootDir *= maker->m_flPostSpawnSpeed;

				// Apply it to the entity
				IPhysicsObject *pPhysicsObject = entity->VPhysicsGetObject();
				if ( pPhysicsObject )
				{
					pPhysicsObject->AddVelocity(&vecShootDir, NULL);
				}
				else
				{
					entity->SetAbsVelocity( vecShootDir );
				}
			}
			
			return true;
		}
		else
			return false;
	}
	
	DETOUR_DECL_MEMBER(void, CEnvEntityMaker_InputForceSpawn, inputdata_t &inputdata)
	{
		auto me = reinterpret_cast<CEnvEntityMaker *>(this);
		if (!SpawnOurTemplate(me,me->GetAbsOrigin(),me->GetAbsAngles())){
			DETOUR_MEMBER_CALL(CEnvEntityMaker_InputForceSpawn)(inputdata);
		}
	}
	DETOUR_DECL_MEMBER(void, CEnvEntityMaker_InputForceSpawnAtEntityOrigin, inputdata_t &inputdata)
	{
		auto me = reinterpret_cast<CEnvEntityMaker *>(this);
		templateTargetEntity = servertools->FindEntityByName( NULL, STRING(inputdata.value.StringID()), me, inputdata.pActivator, inputdata.pCaller );
		DETOUR_MEMBER_CALL(CEnvEntityMaker_InputForceSpawnAtEntityOrigin)(inputdata);
		templateTargetEntity = nullptr;
	}
	DETOUR_DECL_MEMBER(void, CEnvEntityMaker_SpawnEntity, Vector vector, QAngle angles)
	{
		auto me = reinterpret_cast<CEnvEntityMaker *>(this);
		if (!SpawnOurTemplate(me,vector,angles)){
			DETOUR_MEMBER_CALL(CEnvEntityMaker_SpawnEntity)(vector,angles);
		}
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetParent, CBaseEntity *pParentEntity, int iAttachment)
	{
		auto me = reinterpret_cast<CBaseEntity *>(this);
		DETOUR_MEMBER_CALL(CBaseEntity_SetParent)(pParentEntity,iAttachment);
		if (me->GetSolid() == SOLID_BBOX && me->GetBaseAnimating() == nullptr && strcmp(STRING(me->GetModelName()), TEMPLATE_BRUSH_MODEL) == 0) {
			me->CollisionProp()->AddSolidFlags(FSOLID_ROOT_PARENT_ALIGNED);
		}
	}

	DETOUR_DECL_MEMBER(void, CCollisionProperty_SetSolid, SolidType_t solid)
	{
		CBaseEntity *me = reinterpret_cast<CBaseEntity *>(reinterpret_cast<CCollisionProperty *>(this)->GetEntityHandle());
		if (me == nullptr || WholeMapTriggerModule::List().empty()) { DETOUR_MEMBER_CALL(CCollisionProperty_SetSolid)(solid); return; }

		auto solidpre = TriggerCollideable(me);
		DETOUR_MEMBER_CALL(CCollisionProperty_SetSolid)(solid);
		auto solidnow = TriggerCollideable(me);
		if (!solidpre && solidnow) {
			for (auto mod : WholeMapTriggerModule::List()) {
				if (mod->entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
				//mod->entity->GetDamageType();
					mod->entity->StartTouch(me);
				}
			}
		}
		else if (solidpre && !solidnow) {
			for (auto mod : WholeMapTriggerModule::List()) {
				if (mod->entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
					mod->entity->EndTouch(me);
				}
			}
		}
	}

	ConVar cvar_whole_map_trigger_all("sig_pop_pointtemplate_fast_whole_map_trigger_all", "0", FCVAR_NONE, "Mod: Optimize whole map triggers, the triggers must have higher volume than a cube with specified size");
	void CheckSetWholeMapTrigger(CBaseEntity *me)
	{
		WholeMapTriggerModule *mod;
		float minRadius = cvar_whole_map_trigger_all.GetFloat();
		if (minRadius > 0 && me->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER) && (mod = me->GetEntityModule<WholeMapTriggerModule>("wholemaptrigger")) != nullptr) {
			if (me->GetModel() != nullptr) {
				Vector min;
				Vector max;
				modelinfo->GetModelBounds(me->GetModel(), min, max);
				float volume = ((max.z - min.z) * (max.y - min.y) * (max.x - min.x));
				if (volume > minRadius * minRadius * minRadius) {
					me->SetModel(TEMPLATE_BRUSH_MODEL);
					me->SetSolid(SOLID_BBOX);
					me->SetAbsOrigin(Vector(-30000,-30000,-30000));
					me->CollisionProp()->SetCollisionBounds(vec3_origin, vec3_origin);
					mod = me->GetOrCreateEntityModule<WholeMapTriggerModule>("wholemaptrigger");
					ForEachEntity([&](CBaseEntity *entity) {
						if (TriggerCollideable(entity)) {
							me->StartTouch(entity);
						}
					});
				}
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CBaseEntity_SetModel, const char *model)
	{
		DETOUR_MEMBER_CALL(CBaseEntity_SetModel)(model);
		CheckSetWholeMapTrigger(reinterpret_cast<CBaseEntity *>(this));
	}


	RefCount rc_CCollisionProperty_SetSolidFlags;
	RefCount rc_CTFPlayer_Event_Killed;
	DETOUR_DECL_MEMBER(void, CCollisionProperty_SetSolidFlags, int flags)
	{
		CBaseEntity *me = reinterpret_cast<CBaseEntity *>(reinterpret_cast<CCollisionProperty *>(this)->GetEntityHandle());
		if (me == nullptr) { DETOUR_MEMBER_CALL(CCollisionProperty_SetSolidFlags)(flags); return; }
		
		if (!cvar_whole_map_trigger_all.GetBool() && WholeMapTriggerModule::List().empty()) { DETOUR_MEMBER_CALL(CCollisionProperty_SetSolidFlags)(flags); return; }

		SCOPED_INCREMENT(rc_CCollisionProperty_SetSolidFlags);
		auto solidpre = TriggerCollideable(me);
		auto triggerpre = me->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER);
		DETOUR_MEMBER_CALL(CCollisionProperty_SetSolidFlags)(flags);
		//Msg("entity %s %d %d %d\n", me->GetClassname(), me->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER), me->CollisionProp()->IsSolidFlagSet(FSOLID_NOT_SOLID), TriggerCollideable(me));
		auto solidnow = TriggerCollideable(me);
		if (!solidpre && solidnow) {
			int restore = -1; 
			if (me->IsPlayer() && !me->IsAlive()) {
				restore = me->m_lifeState;
				me->m_lifeState = LIFE_ALIVE;
			}
			for (auto mod : WholeMapTriggerModule::List()) {
				if (mod->entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
					mod->entity->StartTouch(me);
				}
			}
			if (restore != -1) {
				me->m_lifeState = restore;
			}
		}
		else if (solidpre && !solidnow) {
			if (!rc_CTFPlayer_Event_Killed && me->IsPlayer()) {
				for (auto mod : WholeMapTriggerModule::List()) {
					if (mod->entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
						mod->entity->EndTouch(me);
					}
				}
			}
		}

		if (triggerpre && !me->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
			auto mod = me->GetEntityModule<WholeMapTriggerModule>("wholemaptrigger");
			if (mod != nullptr) {
				ForEachEntity([&](CBaseEntity *entity) {
					if (TriggerCollideable(entity)) {
						me->EndTouch(entity);
					}
				});
			}
		}
		else if (!triggerpre && me->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
			auto mod = me->GetEntityModule<WholeMapTriggerModule>("wholemaptrigger");
			if (mod != nullptr) {
				ForEachEntity([&](CBaseEntity *entity) {
					if (TriggerCollideable(entity)) {
						me->StartTouch(entity);
					}
				});
			}
			else {
				CheckSetWholeMapTrigger(me);
			}
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayer_Event_Killed, const CTakeDamageInfo& info)
	{
		SCOPED_INCREMENT(rc_CTFPlayer_Event_Killed);
		auto player = reinterpret_cast<CTFPlayer *>(this);

		DETOUR_MEMBER_CALL(CTFPlayer_Event_Killed)(info);
		for (auto mod : WholeMapTriggerModule::List()) {
			if (mod->entity->CollisionProp()->IsSolidFlagSet(FSOLID_TRIGGER)) {
				mod->entity->EndTouch(player);
			}
		}
	}

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Pop:PointTemplates")
		{
			MOD_ADD_DETOUR_MEMBER(CUpgrades_Spawn, "CUpgrades::Spawn");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_D0, "CUpgrades::~CUpgrades [D0]");
			MOD_ADD_DETOUR_MEMBER(CUpgrades_D2, "CUpgrades::~CUpgrades [D2]");
			
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_UpdateOnRemove, "CBaseEntity::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CEnvEntityMaker_SpawnEntity,                   "CEnvEntityMaker::SpawnEntity");
			MOD_ADD_DETOUR_MEMBER(CEnvEntityMaker_InputForceSpawn,               "CEnvEntityMaker::InputForceSpawn");
			MOD_ADD_DETOUR_MEMBER(CEnvEntityMaker_InputForceSpawnAtEntityOrigin, "CEnvEntityMaker::InputForceSpawnAtEntityOrigin");

			// Set FSOLID_ROOT_PARENT_ALIGNED to parented brush entities spawned by point templates
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetParent, "CBaseEntity::SetParent");

			// Optimize whole map triggers, auto touch logic
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetModel, "CBaseEntity::SetModel");
			MOD_ADD_DETOUR_MEMBER(CCollisionProperty_SetSolid, "CCollisionProperty::SetSolid");
			MOD_ADD_DETOUR_MEMBER(CCollisionProperty_SetSolidFlags, "CCollisionProperty::SetSolidFlags");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_Event_Killed, "CTFPlayer::Event_Killed");
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePostEntityThink() override
		{
			Update_Point_Templates();
			if (fast_whole_map_trigger.GetBool() && !WholeMapTriggerModule::List().empty()) {
				ForEachEntity([&](CBaseEntity *entity){
					if (TriggerCollideable(entity)) {
						for (auto mod : WholeMapTriggerModule::List()) {
							mod->entity->Touch(entity);
						}
					}
				});
			}
		}
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_pop_pointtemplate", "0", FCVAR_NOTIFY,
		"Mod: Enable point template logic",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});

}