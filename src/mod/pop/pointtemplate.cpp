#include "stub/entities.h"
#include "stub/gamerules.h"
#include "stub/tfplayer.h"
#include "mod/pop/pointtemplate.h"
#include "util/misc.h"
#include "stub/misc.h"
int Template_Increment;
std::vector<PointTemplateInstance *> Template_Instances;

GlobalThunk<CEventQueue> g_EventQueue("g_EventQueue");

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

PointTemplateInstance *PointTemplate::SpawnTemplate(CBaseEntity *parent, Vector &translation, QAngle &rotation, bool autoparent, const char *attachment) {

	DevMsg("Spawning from template %s",this->name.c_str());
	Template_Increment +=1;
	if (Template_Increment > 99999)
		Template_Increment = 0;

	
	auto templ_inst = new PointTemplateInstance();
	Template_Instances.push_back(templ_inst);
	templ_inst->templ=this;
	templ_inst->parent=parent;
	templ_inst->id=Template_Increment;
	templ_inst->has_parent=parent != nullptr;

	if (this->has_parent_name && parent != nullptr){
		const char *str = STRING(parent->GetEntityName());
		if (strchr(str,'&') == NULL)
			parent->KeyValue("targetname", CFmtStr("%s&%d",str, Template_Increment));
	}
	const char *parentname;
	CBaseEntity* parent_helper = parent;

	//Vector parentpos({0,0,0});
	//QAngle parentang({0,0,0});

	if (parent != nullptr){
		CBaseAnimating *animating = rtti_cast<CBaseAnimating *>(parent);
		int bone = -1;
		if (attachment != nullptr)
			bone=animating->LookupBone(attachment);
		templ_inst->attachment=bone;
		//if (bone != -1)
		//	animating->GetBonePosition(bone,parentpos,parentang);

 		if(parent->IsPlayer()){
			// parent_helper = CreateEntityByName("prop_dynamic");
			// parent_helper->SetModel("models/weapons/w_models/w_rocket.mdl");
			parent_helper = CreateEntityByName("point_teleport");
			parent_helper->SetAbsOrigin(parent->GetAbsOrigin());
			parent_helper->SetAbsAngles(parent->GetAbsAngles());
			parent_helper->Spawn();
			parent_helper->Activate();
			//parent_helper->AddEffects(32);
			//parent_helper->KeyValue("Solid", "0");
			templ_inst->entities.push_back(parent_helper);
			templ_inst->parent_helper = parent_helper;
		}

		parentname = STRING(parent->GetEntityName());
	}
	else
	{
		parentname = "";
	}
	
	std::unordered_map<std::string,CBaseEntity*> spawned;
	CBaseEntity *spawn_trigger;

	for (auto it = this->entities.begin(); it != this->entities.end(); ++it){
		std::multimap<std::string,std::string> &keys = *it;
		DevMsg("Spawning entity %s, keys %d",keys.find("classname")->second.c_str(),keys.size());
		CBaseEntity *entity = CreateEntityByName(keys.find("classname")->second.c_str());
		if (entity != nullptr) {
			
			DevMsg("Entity not null\n");
			for (auto it1 = keys.begin(); it1 != keys.end(); ++it1){
				std::string val = it1->second;
				
				if (it1->first == "TeleportWhere"){
					DevMsg("Setting teleportwhere");
					Teleport_Destination().insert({val,entity});
					continue;
				}

				DevMsg("keyvaluepre %s %s\n",it1->first.c_str(), val.c_str());
				if (!this->no_fixup)
					FixupKeyvalue(val,Template_Increment,parentname);

				entity->KeyValue(it1->first.c_str(), val.c_str());
				DevMsg("keyvalue %s %s\n",it1->first.c_str(), val.c_str());
			}

			auto itname = keys.find("targetname");
			if (itname != keys.end()){
				
				spawned[itname->second]=entity;
				DevMsg("targetname found\n");
			}
			
			Vector translated=Vector();
			QAngle rotated=QAngle();
			VectorAdd(entity->GetAbsOrigin(),translation,translated);
			VectorAdd(entity->GetAbsAngles(),rotation,rotated);
			if (parent != nullptr && autoparent) {
						
				DevMsg("vector %f %f %f",translation.x, translation.y, translation.z);
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

				DevMsg("has parent pre %d",entity->GetMoveParent() != nullptr);
				if (keys.find("parentname") == keys.end()){
					variant_t variant1;
					variant1.SetString(MAKE_STRING("!activator"));
					entity->AcceptInput("setparent", parent_helper, parent_helper,variant1,-1);
				}
				
				DevMsg("has parent post %d",entity->GetMoveParent() != nullptr);
			}
			else
			{
				entity->Teleport(&translated,&rotated,&vec3_origin);
				//entity->SetAbsAngles(rotated);
			}

			if (keys.find("parentname") != keys.end()) {
				std::string parstr = keys.find("parentname")->second;
				//parstr.append(std::to_string(Template_Increment));
				CBaseEntity *parentlocal = spawned[parstr];
				DevMsg("parent %d %s",spawned.find(parstr) != spawned.end(), parstr.c_str());
				if (parentlocal != nullptr) {
					variant_t variant1;
					variant1.SetString(MAKE_STRING("!activator"));
					entity->AcceptInput("setparent", parentlocal, parentlocal,variant1,-1);
					DevMsg("found local ent\n");
				}
				else
					DevMsg("not found local ent\n");
			}
			
			servertools->DispatchSpawn(entity);
			entity->Activate();
			templ_inst->entities.push_back(entity);
			
			
			//CObjectTeleporter+0xaec: CUtlStringList m_TeleportWhere
			// if (keys.find("where") != keys.end()) {
			// 	DevMsg("Finding spawner\n");
			// 	CUtlStringList* list = ((CUtlStringList*)((uintptr_t)entity+0xaec));
			// 	list->CopyAndAddToTail(keys.find("where")->second.c_str());
				
			// 	FOR_EACH_VEC((*list), j) {
			// 		DevMsg("Found spawner %s\n",(*list)[j]);
			// 	}
			// }
			//To make brush entities working
			DevMsg("wth");
			if (keys.find("mins") != keys.end() && keys.find("maxs") != keys.end()){

				entity->SetModel("models/weapons/w_models/w_rocket.mdl");
				entity->KeyValue("Solid", "3");
				entity->KeyValue("mins", keys.find("mins")->second.c_str());
				entity->KeyValue("maxs", keys.find("maxs")->second.c_str());
				entity->AddEffects(32); //DONT RENDER
				DevMsg("solid type %d",entity->GetSolid());

			}
			
			DevMsg("wth\n");
			DevMsg("range %f %f\n",entity->CollisionProp()->OBBMaxs().x,entity->CollisionProp()->OBBMins().x);
			DevMsg("Entity spawned %s\n",keys.find("classname")->second.c_str());
		}
		
	}
	if(spawned.find("trigger_spawn_relay&") != spawned.end()){
		variant_t variant;
		variant.SetString(MAKE_STRING(""));
		if (parent != nullptr)
			spawned.find("trigger_spawn_relay&")->second->AcceptInput("Trigger",parent,parent,variant,-1);
		else
			spawned.find("trigger_spawn_relay&")->second->AcceptInput("Trigger",UTIL_EntityByIndex(0), UTIL_EntityByIndex(0),variant,-1);
		servertools->RemoveEntity(spawned.find("trigger_spawn_relay&")->second);
	}
	/*for (auto it = this->onspawn_inputs.begin(); it != this->onspawn_inputs.end(); ++it){
		variant_t variant1;

		std::string arg = it->param;
		int parpos = 0;
		while((parpos = arg.find("!parent",parpos)) != -1){
			arg.replace(parpos,7,parentname);
		}

		int amperpos = 0;
		while((amperpos = arg.find('&',amperpos)) != -1){
			amperpos+=1;
			arg.insert(amperpos,std::to_string(Template_Increment));
			DevMsg("amp %d\n",amperpos);
		}

		std::string target = it->target;
		parpos = 0;
		while((parpos = target.find("!parent",parpos)) != -1){
			target.replace(parpos,7,parentname);
		}

		amperpos = 0;
		while((amperpos = target.find('&',amperpos)) != -1){
			amperpos+=1;
			target.insert(amperpos,std::to_string(Template_Increment));
			DevMsg("amp %d\n",amperpos);
		}

		DevMsg("Executing onspawn %s %s %s %f\n",target.c_str(),it->input.c_str(),arg.c_str(),it->delay);
		//if (!arg.empty())
		string_t m_iParameter = AllocPooledString(arg.c_str());
			variant1.SetString(m_iParameter);
		CEventQueue &que = g_EventQueue;
		que.AddEvent(it->target.c_str(),it->input.c_str(),variant1,it->delay,parent,parent,-1);
	}*/
	return templ_inst;
}

PointTemplateInfo Parse_SpawnTemplate(KeyValues *kv) {
	DevMsg("Parse SpawnTemplate Pre\n");
	PointTemplateInfo info;
	bool hasname = false;
	
	FOR_EACH_SUBKEY(kv, subkey) {
		hasname = true;
		const char *name = subkey->GetName();
		if (FStrEq(name, "Name")){
			auto it = Point_Templates().find(subkey->GetString()) ;
			if(it != Point_Templates().end())
				info.templ = &(it->second);
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
		if (Point_Templates().find(kv->GetString()) != Point_Templates().end())
			info.templ = &Point_Templates()[kv->GetString()];
	}

	DevMsg("Parse SpawnTemplate Post\n");
	return info;
}


void PointTemplateInstance::OnKilledParent(bool cleared) {

	//DevMsg("init %d %d\n", cleared, this->templ->has_on_kill_trigger);
	if (!cleared && this->templ->has_on_kill_trigger) {
		DevMsg("Firing output onkill\n");
		CBaseEntity *trigger = CreateEntityByName("logic_relay");
		variant_t variant1;
		variant1.SetString(MAKE_STRING(""));

		for(auto it = this->templ->on_kill_triggers.begin(); it != this->templ->on_kill_triggers.end(); it++){
			std::string val = *(it); 
			if (!this->templ->no_fixup)
				FixupKeyvalue(val,this->id,"");
			trigger->KeyValue("ontrigger",val.c_str());
			DevMsg("With value %s\n",val.c_str());
		}
		trigger->KeyValue("spawnflags", "2");
		servertools->DispatchSpawn(trigger);
		trigger->Activate();
		if (this->parent->IsPlayer() && this->parent != nullptr)
			trigger->AcceptInput("trigger", this->parent, this->parent ,variant1,-1);
		else
			trigger->AcceptInput("trigger", UTIL_EntityByIndex(0),UTIL_EntityByIndex(0),variant1,-1);
		servertools->RemoveEntity(trigger);
	}
	//DevMsg("Removing entities \n");
	//DevMsg("Removing entities %d \n", this->templ->keep_alive);
	for(auto it = this->entities.begin(); it != this->entities.end(); it++){
		if (*(it) != nullptr){
			if (cleared || !(this->templ->keep_alive)){
				
				//DevMsg("Removing entitytmp\n");
				servertools->RemoveEntityImmediate(*(it));
			}
			else {
				//DevMsg("Keeping entitytmp\n");
				CBaseEntity *ent =*(it);
				if (this->parent != nullptr && ent != nullptr && ent->GetMoveParent() == this->parent){
					variant_t variant1;
					variant1.SetString(MAKE_STRING(""));
					ent->AcceptInput("clearparent", UTIL_EntityByIndex(0),UTIL_EntityByIndex(0),variant1,-1);
				}
			}
		}
	}
	//DevMsg("Renaming\n");
	if (this->templ->has_parent_name && this->parent != nullptr && this->parent->IsPlayer()){
		std::string str = STRING(this->parent->GetEntityName());
		
		int pos = str.find('&');
		if (pos != -1) {
			str.resize(pos);
			parent->KeyValue("targetname", str.c_str());
		}
		//DevMsg("Unsetted targetname %s\n",this->parent->GetEntityName());
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

void Clear_Point_Templates()
{
	for(auto it = Template_Instances.begin(); it != Template_Instances.end(); it++){
		auto inst = *(it);
		inst->OnKilledParent(true);
		delete inst;
	}
	Template_Instances.clear();
	Point_Templates().clear();
}

void Update_Point_Templates()
{
	for(auto it = Teleport_Destination().begin(); it != Teleport_Destination().end();it++){
		//DevMsg("Remove teleportwhere1");
		if (it->second == nullptr) {
			DevMsg("Remove teleportwhere");
			Teleport_Destination().erase(it);
			break;
		}
	}
	for(auto it = Template_Instances.begin(); it != Template_Instances.end(); it++){
		auto inst = *(it);
		if (inst->has_parent && (inst->parent == nullptr || !(inst->parent->IsAlive()))) {
			inst->OnKilledParent(false);
		}
		if (!inst->has_parent) {
			bool hasalive = false;
			for(auto it = inst->entities.begin(); it != inst->entities.end(); it++){
				CHandle<CBaseEntity> &ent = *(it);
				if (ent != nullptr && ent->IsAlive()){
					hasalive = true;
					break;
				}
			}
			if (!hasalive)
			{
				inst->OnKilledParent(true);
			}
		}
		if (inst->mark_delete) {
			Template_Instances.erase(it);
			delete inst;
			it--;
			continue;
		}
		if (inst->parent_helper != nullptr && inst->parent != nullptr) {
			//DevMsg("Setting parent helper pos %f, parent pos %f\n",it->parent_helper->GetAbsOrigin().x, it->parent->GetAbsOrigin().x);
			Vector pos;
			QAngle ang;
			CBaseEntity *parent =inst->parent;
			if (inst->attachment != -1){
				CBaseAnimating *anim =rtti_cast<CBaseAnimating *>(parent);
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
MemberFuncThunk< CEventQueue*, void, const char*,const char *, variant_t, float, CBaseEntity *, CBaseEntity *, int>   CEventQueue::ft_AddEvent("CEventQueue::AddEvent");
StaticFuncThunk<bool, bool, bool, CHandle<CTFBotHintEngineerNest> *> CTFBotMvMEngineerHintFinder::ft_FindHint("CTFBotMvMEngineerHintFinder::FindHint");
StaticFuncThunk<void, IRecipientFilter&, float, char const*, Vector, QAngle, CBaseEntity*, ParticleAttachment_t> ft_TE_TFParticleEffect("TE_TFParticleEffect");
StaticFuncThunk<bool, const Vector&> ft_IsSpaceToSpawnHere("IsSpaceToSpawnHere");