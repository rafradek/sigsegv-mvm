#include "mod.h"
#include "stub/entities.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "mod/pop/pointtemplate.h"
#include "util/misc.h"
#include "util/backtrace.h"
#include "stub/misc.h"
#include "stub/tfweaponbase.h"
int Template_Increment;


#define TEMPLATE_BRUSH_MODEL "models/weapons/w_models/w_rocket.mdl"



void FixupKeyvalue(std::string &val,int id, const char *parentname) {
	int amperpos = 0;
	while((amperpos = val.find('&',amperpos)) != -1){
		amperpos+=1;
		val.insert(amperpos,std::to_string(id));
		DevMsg("amp %d\n",amperpos);
	}
		
	int parpos = 0;
	while((parpos = val.find("!parent",parpos)) != -1){
		val.replace(parpos,7,parentname);
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

PointTemplateInstance *PointTemplate::SpawnTemplate(CBaseEntity *parent, const Vector &translation, const QAngle &rotation, bool autoparent, const char *attachment, bool ignore_parent_alive_state) {

	Template_Increment +=1;
	if (Template_Increment > 99999)
		Template_Increment = 0;

	
	auto templ_inst = new PointTemplateInstance();
	g_templateInstances.push_back(templ_inst);
	templ_inst->templ = this;
	templ_inst->parent = parent;
	templ_inst->id = Template_Increment;
	templ_inst->has_parent = parent != nullptr;
	templ_inst->ignore_parent_alive_state = ignore_parent_alive_state;

	if (this->has_parent_name && parent != nullptr){
		const char *str = STRING(parent->GetEntityName());
		if (strchr(str,'&') == NULL)
			parent->KeyValue("targetname", CFmtStr("%s&%d",str, Template_Increment));
	}
	const char *parentname;
	CBaseEntity* parent_helper = parent;

	if (parent != nullptr){

		int bone = -1;
		if (attachment != nullptr) {
			CBaseAnimating *animating = rtti_cast<CBaseAnimating *>(parent);
			if (animating != nullptr)
				bone = animating->LookupBone(attachment);
		}
		templ_inst->attachment = bone;

 		if(parent->IsPlayer() && autoparent){
			parent_helper = CreateEntityByName("point_teleport");
			parent_helper->SetAbsOrigin(parent->GetAbsOrigin());
			parent_helper->SetAbsAngles(parent->GetAbsAngles());
			parent_helper->Spawn();
			parent_helper->Activate();
			templ_inst->entities.push_back(parent_helper);
			templ_inst->parent_helper = parent_helper;
		}

		parentname = STRING(parent->GetEntityName());
		g_pointTemplateParent.insert(parent);
	}
	else
	{
		parentname = "";
	}
	
	std::unordered_map<std::string,CBaseEntity*> spawned;
	std::vector<CBaseEntity*> spawned_list;

	for (auto it = this->entities.begin(); it != this->entities.end(); ++it){
		std::multimap<std::string,std::string> &keys = *it;
		CBaseEntity *entity = CreateEntityByName(keys.find("classname")->second.c_str());
		if (entity != nullptr) {
			spawned_list.push_back(entity);
			for (auto it1 = keys.begin(); it1 != keys.end(); ++it1){
				std::string val = it1->second;
				
				if (it1->first == "TeleportWhere"){
					Teleport_Destination().insert({val,entity});
					continue;
				}

				if (!this->no_fixup)
					FixupKeyvalue(val,Template_Increment,parentname);

				servertools->SetKeyValue(entity, it1->first.c_str(), val.c_str());
			}

			auto itname = keys.find("targetname");
			if (itname != keys.end()){
				
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
				}
			}
			else
			{
				entity->Teleport(&translated,&rotated,&vec3_origin);
			}

			if (keys.find("parentname") != keys.end()) {
				std::string parstr = keys.find("parentname")->second;
				CBaseEntity *parentlocal = spawned[parstr];
				if (parentlocal != nullptr) {
					entity->SetParent(parentlocal, -1);
				}
			}
			
			SpawnEntity(entity);

			templ_inst->entities.push_back(entity);
			g_pointTemplateChild.insert(entity);
			
			//To make brush entities working
			if (keys.find("mins") != keys.end() && keys.find("maxs") != keys.end()){

				entity->SetModel(TEMPLATE_BRUSH_MODEL);
				entity->KeyValue("Solid", "3");
				entity->KeyValue("mins", keys.find("mins")->second.c_str());
				entity->KeyValue("maxs", keys.find("maxs")->second.c_str());
				entity->AddEffects(32); //DONT RENDER

			}
		}
		
	}
	
	for (auto it = spawned_list.begin(); it != spawned_list.end(); it++) {
		CBaseEntity *entity = *it;
		entity->Activate();
	}

	if(spawned.find("trigger_spawn_relay_inter") != spawned.end()){
		variant_t variant;
		variant.SetString(NULL_STRING);
		CBaseEntity *spawn_trigger = spawned.find("trigger_spawn_relay_inter")->second;
		
		if (parent != nullptr)
			spawn_trigger->AcceptInput("Trigger",parent,parent,variant,-1);
		else
			spawn_trigger->AcceptInput("Trigger",UTIL_EntityByIndex(0), UTIL_EntityByIndex(0),variant,-1);
		servertools->RemoveEntity(spawn_trigger);
	}
	return templ_inst;
}

PointTemplateInstance *PointTemplateInfo::SpawnTemplate(CBaseEntity *parent, bool autoparent){
	if (templ == nullptr && template_name.size() > 0)
		templ = FindPointTemplate(template_name);
	//DevMsg("Is templ null %d\n",templ == nullptr);
	if (templ != nullptr)
		return templ->SpawnTemplate(parent,translation,rotation,autoparent,attachment.c_str(), ignore_parent_alive_state);
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

	PointTemplateInstance *inst = this->templ->SpawnTemplate(player, vecSrc, angForward, false, nullptr);
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
	DevMsg("Parse SpawnTemplate Pre\n");
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
	}
	if (!hasname){
		info.template_name = kv->GetString();
	}

	//To lowercase
	info.templ = FindPointTemplate(info.template_name);

	DevMsg("Parse SpawnTemplate Post %d\n",info.templ == nullptr);
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
	}
	return data.templ != nullptr;
}

PointTemplate *FindPointTemplate(std::string &str) {
	std::transform(str.begin(), str.end(), str.begin(),
    [](unsigned char c){ return std::tolower(c); });

	auto it = Point_Templates().find(str);
	if (it != Point_Templates().end())
			return &(it->second);

	return nullptr;
}

void PointTemplateInstance::OnKilledParent(bool cleared) {

	if (this->templ == nullptr || this->mark_delete) {
		this->mark_delete = true;
		DevMsg("template null or deleted\n");
		return;
	}

	if (!cleared && this->templ->has_on_kill_trigger) {

		CBaseEntity *trigger = CreateEntityByName("logic_relay");
		variant_t variant1;
		variant1.SetString(NULL_STRING);

		for(auto it = this->templ->on_kill_triggers.begin(); it != this->templ->on_kill_triggers.end(); it++){
			std::string val = *(it); 
			if (!this->templ->no_fixup)
				FixupKeyvalue(val,this->id,"");
			trigger->KeyValue("ontrigger",val.c_str());

		}

		trigger->KeyValue("spawnflags", "2");
		servertools->DispatchSpawn(trigger);
		trigger->Activate();
		if (this->parent != nullptr && this->parent->IsPlayer())
			trigger->AcceptInput("trigger", this->parent, this->parent ,variant1,-1);
		else
			trigger->AcceptInput("trigger", UTIL_EntityByIndex(0),UTIL_EntityByIndex(0),variant1,-1);
		servertools->RemoveEntity(trigger);
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

	if (this->templ->has_parent_name && this->parent != nullptr && this->parent->IsPlayer()){
		std::string str = STRING(this->parent->GetEntityName());
		
		int pos = str.find('&');
		if (pos != -1) {
			str.resize(pos);
			parent->KeyValue("targetname", str.c_str());
		}
	}

	this->parent = nullptr;
	this->has_parent = false;
	this->mark_delete = !this->templ->keep_alive || cleared;
}
std::unordered_map<std::string, PointTemplate> &Point_Templates()
{
	static std::unordered_map<std::string, PointTemplate> templ;
	return templ;
}

std::unordered_multimap<std::string, CHandle<CBaseEntity>> &Teleport_Destination()
{
	static std::unordered_multimap<std::string, CHandle<CBaseEntity>> tp;
	return tp;
}

std::set<CHandle<CBaseEntity>> g_pointTemplateParent;
std::set<CHandle<CBaseEntity>> g_pointTemplateChild;
std::vector<PointTemplateInstance *> g_templateInstances;

void Clear_Point_Templates()
{
	for(auto it = g_templateInstances.begin(); it != g_templateInstances.end(); it++){
		auto inst = *(it);
		inst->OnKilledParent(true);
		delete inst;
		inst = nullptr;
	}
	g_templateInstances.clear();
	Point_Templates().clear();
}

void Update_Point_Templates()
{
	for(auto it = Teleport_Destination().begin(); it != Teleport_Destination().end();it++){
		if (it->second == nullptr) {
			Teleport_Destination().erase(it);
			break;
		}
	}
	for(auto it = g_templateInstances.begin(); it != g_templateInstances.end(); it++){
		auto inst = *(it);
		if (!inst->mark_delete) {
			if (inst->has_parent && (inst->parent == nullptr || inst->parent->IsMarkedForDeletion() || !(inst->parent->IsAlive() && !inst->ignore_parent_alive_state) )) {
				inst->OnKilledParent(false);
			}
			if (!inst->has_parent && !inst->is_wave_spawned) {
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
					inst->OnKilledParent(true);
				}
			}
		}
		if (inst->mark_delete) {
			g_templateInstances.erase(it);
			delete inst;
			it--;
			inst = nullptr;
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
	for(auto it = g_pointTemplateParent.begin(); it != g_pointTemplateParent.end();){
		if (*(it) == nullptr) {
			it = g_pointTemplateParent.erase(it);
		}
		else
			it++;
	}
	for(auto it = g_pointTemplateChild.begin(); it != g_pointTemplateChild.end();){
		if (*(it) == nullptr) {
			it = g_pointTemplateChild.erase(it);
		}
		else
			it++;
	}
}

StaticFuncThunk<void> ft_PrecachePointTemplates("PrecachePointTemplates");
MemberFuncThunk< CEventQueue*, void, const char*,const char *, variant_t, float, CBaseEntity *, CBaseEntity *, int>   CEventQueue::ft_AddEvent("CEventQueue::AddEvent");
StaticFuncThunk<bool, bool, bool, CHandle<CTFBotHintEngineerNest> *> CTFBotMvMEngineerHintFinder::ft_FindHint("CTFBotMvMEngineerHintFinder::FindHint");
StaticFuncThunk<void, IRecipientFilter&, float, char const*, Vector, QAngle, CBaseEntity*, ParticleAttachment_t> ft_TE_TFParticleEffect("TE_TFParticleEffect");

StaticFuncThunk<void, IRecipientFilter&,
	float,
	const char *,
	Vector,
	QAngle,
	te_tf_particle_effects_colors_t *,
	te_tf_particle_effects_control_point_t *,
	CBaseEntity *,
	ParticleAttachment_t,
	Vector> ft_TE_TFParticleEffectComplex("TE_TFParticleEffectComplex");

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
	
	/* Pointtemplate keep child entities after parent removal*/
	DETOUR_DECL_MEMBER(void, CBaseEntity_UpdateOnRemove)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);

		if (entity->FirstMoveChild() != nullptr && g_pointTemplateParent.find(entity) != g_pointTemplateParent.end()) {

			CBaseEntity *child = entity->FirstMoveChild();

			std::vector<CBaseEntity *> childrenToRemove;
			
			do {
				if (g_pointTemplateChild.find(child) != g_pointTemplateChild.end()) {

					childrenToRemove.push_back(child);
				}
			} 
			while ((child = entity->NextMovePeer()) != nullptr);

			for (auto childToRemove : childrenToRemove) {
				childToRemove->SetParent(nullptr, -1);
			}
		}
		DETOUR_MEMBER_CALL(CBaseEntity_UpdateOnRemove)();
	}

	class CMod : public IMod, public IModCallbackListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Pop:PointTemplates")
		{
			MOD_ADD_DETOUR_MEMBER(CUpgrades_Spawn, "CUpgrades::Spawn");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_UpdateOnRemove, "CBaseEntity::UpdateOnRemove");
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

		virtual void FrameUpdatePostEntityThink() override
		{
			Update_Point_Templates();
		}
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_pop_pointtemplate", "0", FCVAR_NOTIFY,
		"Mod: Enable point template logic",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}