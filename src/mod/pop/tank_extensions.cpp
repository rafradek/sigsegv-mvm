#include "mod.h"
#include "mod/pop/kv_conditional.h"
#include "stub/tfentities.h"
#include "stub/populators.h"
#include "stub/strings.h"
#include "stub/extraentitydata.h"
#include "stub/misc.h"
#include "stub/nextbot_cc.h"
#include "util/scope.h"
#include "mod/pop/pointtemplate.h"
#include "stub/tf_objective_resource.h"
#include "stub/particles.h"
#include "stub/nextbot_etc.h"
#include "util/value_override.h"
#include "mod/etc/mapentity_additions.h"

namespace Mod::Pop::Tank_Extensions
{
	struct SpawnerData
	{
		bool disable_smokestack =  false;
		float scale             =  1.00f;
		bool force_romevision   =  false;
		float max_turn_rate     =   0.0f;
		bool max_turn_rate_set  =  false;
		float gravity           =   0.0f;
		bool gravity_set        =  false;
		string_t icon           = MAKE_STRING("tank");
		bool is_miniboss        =   true;
		bool is_crit           	=  false;
		bool disable_models     =   false;
		bool disable_tracks     =   false;
		bool disable_bomb       =   false;
		bool immobile           =   false;
		bool replace_model_col  =   false;
		int team_num            = -1;
		float offsetz           =   0.0f;
		bool rotate_pitch       =  true;
		bool crit_immune        = false;
		bool trigger_destroy_fix = false;
		bool no_crush_damage    = false;
		bool solid_to_brushes   = false;
		bool no_screen_shake    = false;

		std::string sound_ping =   "";
		std::string sound_deploy =   "";
		std::string sound_engine_loop =   "";
		std::string sound_start =   "";

		std::vector<int> custom_model;
		int model_destruction = -1;
		int left_tracks_model = -1;
		int right_tracks_model = -1;
		int bomb_model = -1;
		
		std::vector<CHandle<CTFTankBoss>> tanks;
		std::vector<PointTemplateInfo> attachements;
		std::vector<PointTemplateInfo> attachements_destroy;
	};
	
	
	std::unordered_map<CTankSpawner *, SpawnerData> spawners;
	
	
	SpawnerData *FindSpawnerDataForTank(const CTFTankBoss *tank)
	{
		if (tank == nullptr) return nullptr;
		
		for (auto& pair : spawners) {
			SpawnerData& data = pair.second;
			for (auto h_tank : data.tanks) {
				if (h_tank.Get() == tank) {
					return &data;
				}
			}
		}
		
		return nullptr;
	}
	SpawnerData *FindSpawnerDataForBoss(const CTFBaseBoss *boss)
	{
		/* FindSpawnerDataForTank doesn't do anything special, just a ptr comparison,
		 * so there's no need to do an expensive rtti_cast or anything if we have a CTFBaseBoss ptr */
		return FindSpawnerDataForTank(static_cast<const CTFTankBoss *>(boss));
	}
	
	
	DETOUR_DECL_MEMBER(void, CTankSpawner_dtor0)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
	//	DevMsg("CTankSpawner %08x: dtor0\n", (uintptr_t)spawner);
		
		spawners.erase(spawner);
		
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CTankSpawner_dtor2)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
	//	DevMsg("CTankSpawner %08x: dtor2\n", (uintptr_t)spawner);

		spawners.erase(spawner);
		
		DETOUR_MEMBER_CALL();
	}
	
	void Parse_Model(KeyValues *kv, SpawnerData &spawner) {
		bool hasmodels = false;
		std::string startstr = "";
		std::string damage1str = "";
		std::string damage2str = "";
		std::string damage3str = "";
		std::string left_trackstr = "";
		std::string right_trackstr = "";
		std::string bombstr = "";
		std::string destructionstr = "";

		if (!spawner.custom_model.empty())
			Warning("CTankSpawner: Model block already found \n");

		FOR_EACH_SUBKEY(kv, subkey) {
			hasmodels = true;
			const char *name = subkey->GetName();
			if (FStrEq(name, "Default") ) {
				startstr = subkey->GetString();
			}
			else if (FStrEq(name, "Damage1") ) {
				damage1str = subkey->GetString();
			}
			else if (FStrEq(name, "Damage2") ) {
				damage2str = subkey->GetString();
			}
			else if (FStrEq(name, "Damage3") ) {
				damage3str = subkey->GetString();
			}
			else if (FStrEq(name, "Destruction") ) {
				destructionstr = subkey->GetString();
			}
			else if (FStrEq(name, "Bomb") ) {
				bombstr = subkey->GetString();
			}
			else if (FStrEq(name, "LeftTrack") ) {
				left_trackstr = subkey->GetString();
			}
			else if (FStrEq(name, "RightTrack") ) {
				right_trackstr = subkey->GetString();
			}
		}
		if (!hasmodels && kv->GetString() != nullptr) {
			startstr = kv->GetString();

			damage1str = startstr.substr(0, startstr.rfind('.')) + "_damage1.mdl";
			if (!filesystem->FileExists(damage1str.c_str(), "GAME"))
				damage1str = startstr;

			damage2str = startstr.substr(0, startstr.rfind('.')) + "_damage2.mdl";
			if (!filesystem->FileExists(damage2str.c_str(), "GAME"))
				damage2str = damage1str;

			damage3str = startstr.substr(0, startstr.rfind('.')) + "_damage3.mdl";
			if (!filesystem->FileExists(damage3str.c_str(), "GAME"))
				damage3str = damage2str;

			destructionstr = startstr.substr(0, startstr.rfind('.')) + "_part1_destruction.mdl";
			if (!filesystem->FileExists(destructionstr.c_str(), "GAME"))
				destructionstr = "";

			int boss_pos = 	startstr.rfind("boss_");
			int mdl_pos = startstr.rfind(".mdl");
			if (boss_pos > 0 && mdl_pos > 0) {
				left_trackstr = startstr;
				left_trackstr.replace(mdl_pos, 4, "_track_l.mdl");
				left_trackstr.replace(boss_pos, 5, "");
				if (!filesystem->FileExists(left_trackstr.c_str(), "GAME"))
					left_trackstr = "";

				right_trackstr = startstr;
				right_trackstr.replace(mdl_pos, 4, "_track_r.mdl");
				right_trackstr.replace(boss_pos, 5, "");
				if (!filesystem->FileExists(right_trackstr.c_str(), "GAME"))
					right_trackstr = "";
			}
		}

		int last_good_index = -1;
		if (startstr != "") {
			last_good_index = CBaseEntity::PrecacheModel(startstr.c_str());
		}
		spawner.custom_model.push_back(last_good_index);

		if (damage1str != "") {
			int model_index = CBaseEntity::PrecacheModel(damage1str.c_str());
			if (model_index != -1)
				last_good_index = model_index;
		}
		spawner.custom_model.push_back(last_good_index);

		if (damage2str != "") {
			int model_index = CBaseEntity::PrecacheModel(damage2str.c_str());
			if (model_index != -1)
				last_good_index = model_index;
		}
		spawner.custom_model.push_back(last_good_index);

		if (damage3str != "") {
			int model_index = CBaseEntity::PrecacheModel(damage3str.c_str());
			if (model_index != -1)
				last_good_index = model_index;
		}
		spawner.custom_model.push_back(last_good_index);
		
		if (destructionstr != "") {
			int model_index = CBaseEntity::PrecacheModel(destructionstr.c_str());
			if (model_index != -1)
				spawner.model_destruction = model_index;
		}

		if (left_trackstr != "") {
			int model_index = CBaseEntity::PrecacheModel(left_trackstr.c_str());
			if (model_index != -1)
				spawner.left_tracks_model = model_index;
		}

		if (right_trackstr != "") {
			int model_index = CBaseEntity::PrecacheModel(right_trackstr.c_str());
			if (model_index != -1)
				spawner.right_tracks_model = model_index;
		}

		if (bombstr != "") {
			int model_index = CBaseEntity::PrecacheModel(bombstr.c_str());
			if (model_index != -1)
				spawner.bomb_model = model_index;
		}
	}

	bool load_from_template = false;
	DETOUR_DECL_MEMBER(bool, CTankSpawner_Parse, KeyValues *kv)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
		auto orig_kv = kv;

		bool loaded_from_template = load_from_template;
		if (load_from_template) {
			kv = orig_kv->MakeCopy();
		}

		std::vector<KeyValues *> del_kv;
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			bool del = true;
			if (FStrEq(name, "DisableSmokestack")) {
			//	DevMsg("Got \"DisableSmokeStack\" = %d\n", subkey->GetBool());
				spawners[spawner].disable_smokestack = subkey->GetBool();
			} else if (FStrEq(name, "Scale")) {
			//	DevMsg("Got \"Scale\" = %f\n", subkey->GetFloat());
				spawners[spawner].scale = subkey->GetFloat();
			} else if (FStrEq(name, "ForceRomeVision")) {
			//	DevMsg("Got \"ForceRomeVision\" = %d\n", subkey->GetBool());
				spawners[spawner].force_romevision = subkey->GetBool();
			} else if (FStrEq(name, "MaxTurnRate")) {
			//	DevMsg("Got \"MaxTurnRate\" = %f\n", subkey->GetFloat());
				spawners[spawner].max_turn_rate = subkey->GetFloat();
				spawners[spawner].max_turn_rate_set = true;
			} else if (FStrEq(name, "ClassIcon")) {
			//	DevMsg("Got \"IconOverride\" = \"%s\"\n", subkey->GetString());
				spawners[spawner].icon = AllocPooledString(subkey->GetString());
			} else if (FStrEq(name, "IsCrit")) {
			//	DevMsg("Got \"IconOverride\" = \"%s\"\n", subkey->GetString());
				spawners[spawner].is_crit = subkey->GetBool();
			} else if (FStrEq(name, "IsMiniBoss")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].is_miniboss = subkey->GetBool();
			} else if (FStrEq(name, "Model")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				Parse_Model(subkey, spawners[spawner]);
			} else if (FStrEq(name, "DisableChildModels")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].disable_models = subkey->GetBool();
			} else if (FStrEq(name, "DisableTracks")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].disable_tracks = subkey->GetBool();
			} else if (FStrEq(name, "DisableBomb")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].disable_bomb = subkey->GetBool();
			} else if (FStrEq(name, "Immobile")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].immobile = subkey->GetBool();
			} else if (FStrEq(name, "Gravity")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].gravity = subkey->GetFloat();
				spawners[spawner].gravity_set = true;
			} else if (FStrEq(name, "Template")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				KeyValues *templates = g_pPopulationManager->m_pTemplates;
				if (templates != nullptr) {
					KeyValues *tmpl = templates->FindKey(subkey->GetString());
					if (tmpl != nullptr) {
						load_from_template = true;
						spawner->Parse(tmpl);
						load_from_template = false;
					}
				}
			} else if (FStrEq(name, "PingSound")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].sound_ping = subkey->GetString();
				if (!enginesound->PrecacheSound(subkey->GetString(), true))
					CBaseEntity::PrecacheScriptSound(subkey->GetString());
			} else if (FStrEq(name, "StartSound")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].sound_start = subkey->GetString();
				if (!enginesound->PrecacheSound(subkey->GetString(), true))
					CBaseEntity::PrecacheScriptSound(subkey->GetString());
			} else if (FStrEq(name, "DeploySound")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].sound_deploy = subkey->GetString();
				if (!enginesound->PrecacheSound(subkey->GetString(), true))
					CBaseEntity::PrecacheScriptSound(subkey->GetString());
			} else if (FStrEq(name, "EngineLoopSound")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].sound_engine_loop = subkey->GetString();
				if (!enginesound->PrecacheSound(subkey->GetString(), true))
					CBaseEntity::PrecacheScriptSound(subkey->GetString());
			} else if (FStrEq(name, "SpawnTemplate")) {
				spawners[spawner].attachements.push_back(Parse_SpawnTemplate(subkey));
			} else if (FStrEq(name, "DestroyTemplate")) {
				spawners[spawner].attachements_destroy.push_back(Parse_SpawnTemplate(subkey));
			} else if (FStrEq(name, "TeamNum")) {
				spawners[spawner].team_num = subkey->GetInt();
			} else if (FStrEq(name, "OffsetZ")) {
				spawners[spawner].offsetz = subkey->GetFloat();
			} else if (FStrEq(name, "ReplaceModelCollisions")) {
				spawners[spawner].replace_model_col = subkey->GetBool();
			} else if (FStrEq(name, "RotatePitch")) {
				spawners[spawner].rotate_pitch = subkey->GetBool();
			} else if (FStrEq(name, "CritImmune")) {
				spawners[spawner].crit_immune = subkey->GetBool();
			} else if (FStrEq(name, "TriggerDestroyBuildingFix")) {
				spawners[spawner].trigger_destroy_fix = subkey->GetBool();
			} else if (FStrEq(name, "NoCrushDamage")) {
				spawners[spawner].no_crush_damage = subkey->GetBool();
			} else if (FStrEq(name, "SolidToBrushes")) {
				spawners[spawner].solid_to_brushes = subkey->GetBool();
			} else if (FStrEq(name, "NoScreenShake")) {
				spawners[spawner].no_screen_shake = subkey->GetBool();
			} else {
				del = false;
			}
			
			if (del) {
			//	DevMsg("Key \"%s\": processed, will delete\n", name);
				del_kv.push_back(subkey);
			} else {
			//	DevMsg("Key \"%s\": passthru\n", name);
			}
		}
		
		for (auto subkey : del_kv) {
		//	DevMsg("Deleting key \"%s\"\n", subkey->GetName());
			kv->RemoveSubKey(subkey);
			subkey->deleteThis();
		}

		auto result = DETOUR_MEMBER_CALL(kv);

		if (loaded_from_template) {
			kv->deleteThis();
		}

		return result;
	}

	void ForceRomeVisionModels(CTFTankBoss *tank, bool romevision)
	{
		auto l_print_model_array = [](auto& array, const char *name){
			DevMsg("\n");
			for (size_t i = 0; i < countof(array); ++i) {
				DevMsg("  %s[%d]: \"%s\"\n", name, i, array[i]);
			}
			DevMsg("\n");
			for (size_t i = 0; i < countof(array); ++i) {
				DevMsg("  modelinfo->GetModelIndex(%s[%d]): %d\n", name, i, modelinfo->GetModelIndex(array[i]));
			}
		};
		
		auto l_print_overrides = [](CBaseEntity *ent, const char *prefix){
			DevMsg("\n");
			for (int i = 0; i < MAX_VISION_MODES; ++i) {
				DevMsg("  %s m_nModelIndexOverrides[%d]: %d \"%s\"\n", prefix, i, ent->m_nModelIndexOverrides[i], modelinfo->GetModelName(modelinfo->GetModel(ent->m_nModelIndexOverrides[i])));
			}
		};
		
		auto l_copy_rome_to_all_overrides = [](CBaseEntity *ent){
			
			for (int i = 0; i < MAX_VISION_MODES; ++i) {
				if (i == VISION_MODE_ROME) continue;
				ent->SetModelIndexOverride(i, ent->m_nModelIndexOverrides[VISION_MODE_ROME]);
			}
		};
		
	//	DevMsg("\n");
	//	DevMsg("  tank->m_iModelIndex: %d\n", (int)tank->m_iModelIndex);
		
	//	l_print_model_array(s_TankModel    .GetRef(), "s_TankModel");
	//	l_print_model_array(s_TankModelRome.GetRef(), "s_TankModelRome");
		
		//l_print_overrides(tank, "[BEFORE]");
		
		// primary method
		
		int mode_to_use = romevision ? VISION_MODE_ROME : VISION_MODE_NONE;
		
		if (romevision) {
			tank->SetModelIndexOverride(VISION_MODE_NONE, modelinfo->GetModelIndex(s_TankModelRome[tank->m_iModelIndex]));
			tank->SetModelIndexOverride(VISION_MODE_ROME, modelinfo->GetModelIndex(s_TankModelRome[tank->m_iModelIndex]));
		}
		else {
			tank->SetModelIndexOverride(VISION_MODE_NONE, modelinfo->GetModelIndex(s_TankModel[tank->m_iModelIndex]));
			tank->SetModelIndexOverride(VISION_MODE_ROME, modelinfo->GetModelIndex(s_TankModel[tank->m_iModelIndex]));
		}
		// alternative method (probably less reliable than the one above)
	//	l_copy_rome_to_all_overrides(tank);
		
		//l_print_overrides(tank, "[AFTER] ");
		
		
		for (CBaseEntity *child = tank->FirstMoveChild(); child != nullptr; child = child->NextMovePeer()) {
			if (!child->ClassMatches("prop_dynamic")) continue;
			
			DevMsg("\n");
			DevMsg("  child [classname \"%s\"] [model \"%s\"]\n", child->GetClassname(), STRING(child->GetModelName()));
			
			//l_print_overrides(child, "[BEFORE]");
			
				child->SetModelIndexOverride(mode_to_use == VISION_MODE_NONE ? VISION_MODE_ROME : VISION_MODE_NONE, child->m_nModelIndexOverrides[mode_to_use]);
			
			//l_print_overrides(child, "[AFTER] ");
		}
	}
	
	

	RefCount rc_CTankSpawner_Spawn;
	CTankSpawner *current_spawner = nullptr;

	
	DETOUR_DECL_MEMBER(bool, CTankSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
		DevMsg("CTankSpawner::Spawn %08x\n", (uintptr_t)this);
		
		SCOPED_INCREMENT(rc_CTankSpawner_Spawn);
		current_spawner = spawner;
		
		auto result = DETOUR_MEMBER_CALL(where, ents);
		

		if (result && ents != nullptr && !ents->IsEmpty()) {
			auto it = spawners.find(spawner);
			if (it != spawners.end()) {
				SpawnerData& data = (*it).second;
				auto tank = rtti_cast<CTFTankBoss *>(ents->Tail().Get());
					
				if (tank != nullptr) {
					data.tanks.push_back(tank);
					
					if (data.team_num != -1) {
						tank->SetTeamNumber(data.team_num);
						tank->AddGlowEffect();
					}

					if (data.scale != 1.00f) {
						/* need to call this BEFORE changing the scale; otherwise,
							* the collision bounding box will be very screwed up */
						tank->UpdateCollisionBounds();
						
						tank->SetModelScale(data.scale);
					}
					static ConVarRef sig_no_romevision_cosmetics("sig_no_romevision_cosmetics");
					if (data.force_romevision || sig_no_romevision_cosmetics.GetBool()) {
						ForceRomeVisionModels(tank, data.force_romevision);
					}

					
					if (data.disable_models || data.disable_tracks || data.disable_bomb) {
						for (CBaseEntity *child = tank->FirstMoveChild(); child != nullptr; child = child->NextMovePeer()) {
							if (!child->ClassMatches("prop_dynamic")) continue;
							const char * model = STRING(child->GetModelName());
							DevMsg("model name %s\n", model);
							if (data.disable_models || (data.disable_tracks && (FStrEq(model, "models/bots/boss_bot/tank_track_L.mdl") || 
								FStrEq(model, "models/bots/boss_bot/tank_track_R.mdl"))) || (data.disable_bomb && FStrEq(model, "models/bots/boss_bot/bomb_mechanism.mdl")))
							child->AddEffects(32);
						}
					}

					if (!data.custom_model.empty()) {
						if (data.replace_model_col)
							tank->SetModel(modelinfo->GetModelName(modelinfo->GetModel(data.custom_model[0])));

						tank->SetModelIndexOverride(VISION_MODE_NONE, data.custom_model[0]);
						tank->SetModelIndexOverride(VISION_MODE_ROME, data.custom_model[0]);
					}
					
					if (data.offsetz != 0.0f)
					{
						Vector offset = tank->GetAbsOrigin() + Vector(0, 0, data.offsetz);
						tank->SetAbsOrigin(offset);
					}
					for (CBaseEntity *child = tank->FirstMoveChild(); child != nullptr; child = child->NextMovePeer()) {
						if (!child->ClassMatches("prop_dynamic")) continue;
						
						int replace_model = -1;
						if (data.bomb_model != -1 && FStrEq(STRING(child->GetModelName()), "models/bots/boss_bot/bomb_mechanism.mdl"))
							replace_model = data.bomb_model;
						else if (data.left_tracks_model != -1 && FStrEq(STRING(child->GetModelName()), "models/bots/boss_bot/tank_track_L.mdl"))
							replace_model = data.left_tracks_model;
						else if (data.right_tracks_model != -1 && FStrEq(STRING(child->GetModelName()), "models/bots/boss_bot/tank_track_R.mdl"))
							replace_model = data.right_tracks_model;

						DevMsg("Replace child model %s %d\n",  STRING(child->GetModelName()), replace_model );
						if (replace_model != -1) {
							child->SetModelIndex(replace_model);
							child->SetModelIndexOverride(VISION_MODE_NONE, replace_model);
							child->SetModelIndexOverride(VISION_MODE_ROME, replace_model);
						}
						
					}

					for (auto it1 = data.attachements.begin(); it1 != data.attachements.end(); ++it1) {
						it1->SpawnTemplate(tank);
					}
					
					if (data.immobile) {
						tank->SetMoveType(MOVETYPE_NONE, MOVECOLLIDE_DEFAULT);
					}
				}
			}
		}
		
		return result;
	}
	
	
	
	CTFTankBoss *thinking_tank      = nullptr;
	SpawnerData *thinking_tank_data = nullptr;
	
	RefCount rc_CTFTankBoss_TankBossThink;
	DETOUR_DECL_MEMBER(void, CTFTankBoss_TankBossThink)
	{
		auto tank = reinterpret_cast<CTFTankBoss *>(this);
		Vector vec;
		QAngle ang;
		SpawnerData *data = FindSpawnerDataForTank(tank);
		CPathTrack *node = tank->m_hCurrentNode;
		if (data != nullptr) {
			thinking_tank      = tank;
			thinking_tank_data = data;
			vec = tank->GetAbsOrigin();
			ang = tank->GetAbsAngles();
			
		}
		
		SCOPED_INCREMENT(rc_CTFTankBoss_TankBossThink);
		DETOUR_MEMBER_CALL();

		if (data != nullptr && data->immobile) {
			tank->SetAbsOrigin(vec);
			tank->SetAbsAngles(ang);
			tank->m_hCurrentNode  = nullptr;
		}
		
		int spawnflags = tank->m_spawnflags;
		if (node != nullptr && tank->m_hCurrentNode == nullptr) {
			variant_t variant;
			variant.SetString(NULL_STRING);
			if (((data != nullptr && data->attachements.size() != 0) || (spawnflags & 1))) {
				tank->AcceptInput("FireUser4",tank,tank,variant,-1);
			}
			tank->FireCustomOutput<"onstartdeploy">(tank, tank, variant);
		}

		node = tank->m_hCurrentNode;
		thinking_tank      = nullptr;
		thinking_tank_data = nullptr;
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetModelIndexOverride, int index, int nValue)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (rc_CTFTankBoss_TankBossThink > 0 && thinking_tank != nullptr && thinking_tank_data != nullptr) {
			CTFTankBoss *tank = thinking_tank;
			SpawnerData *data = thinking_tank_data;

			static ConVarRef sig_no_romevision_cosmetics("sig_no_romevision_cosmetics");
			bool set = false;
			if (data->force_romevision || sig_no_romevision_cosmetics.GetBool()) {
			//	DevMsg("SetModelIndexOverride(%d, %d) for ent #%d \"%s\" \"%s\"\n", index, nValue, ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetModelName()));

				if (ent == tank) {
					if ((data->force_romevision && index == VISION_MODE_ROME) || (sig_no_romevision_cosmetics.GetBool() && index == VISION_MODE_NONE)) {
						DETOUR_MEMBER_CALL(VISION_MODE_NONE, nValue);
						DETOUR_MEMBER_CALL(VISION_MODE_ROME, nValue);
					}
					set = true;
				}
			}
			if (!data->custom_model.empty()) {
			//	DevMsg("SetModelIndexOverride(%d, %d) for ent #%d \"%s\" \"%s\"\n", index, nValue, ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetModelName()));
				
				if (ent == tank) {
					
					int health_per_model = tank->GetMaxHealth() / 4;
					int health_threshold = tank->GetMaxHealth() - health_per_model;
					int health_stage;
					for (health_stage = 0; health_stage < 4; health_stage++) {
						if (tank->GetHealth() > health_threshold)
							break;

						health_threshold -= health_per_model;
					}
					DevMsg("Health stage %d %d\n", data->custom_model[health_stage], health_stage);
					DETOUR_MEMBER_CALL(VISION_MODE_NONE, data->custom_model[health_stage]);
					DETOUR_MEMBER_CALL(VISION_MODE_ROME, data->custom_model[health_stage]);
					set = true;
				}
			//	if (ent->GetMoveParent() == tank && ent->ClassMatches("prop_dynamic")) {
			//		DevMsg("Blocking SetModelIndexOverride(%d, %d) for tank %d prop %d \"%s\"\n", index, nValue, ENTINDEX(tank), ENTINDEX(ent), STRING(ent->GetModelName()));
			//		return;
			//	}
			}
			if (set)
				return;
		}
		
		DETOUR_MEMBER_CALL(index, nValue);
	}
	
	
	DETOUR_DECL_MEMBER(int, CBaseAnimating_LookupAttachment, const char *szName)
	{
		if (rc_CTankSpawner_Spawn > 0 && current_spawner != nullptr &&
			szName != nullptr && strcmp(szName, "smoke_attachment") == 0) {
			
			auto it = spawners.find(current_spawner);
			if (it != spawners.end()) {
				SpawnerData& data = (*it).second;
				
				if (data.disable_smokestack) {
					/* return 0 so that CTFTankBoss::Spawn will assign 0 to its
					 * m_iSmokeAttachment variable, which results in skipping
					 * the particle logic in CTFTankBoss::TankBossThink */
					return 0;
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(szName);
	}
	
	VHOOK_DECL(float, CTFBaseBossLocomotion_GetGravity)
	{
		if (rc_CTFTankBoss_TankBossThink) {
			if (thinking_tank_data != nullptr && thinking_tank_data->gravity_set) {
				return thinking_tank_data->gravity;
			}
		}
		return VHOOK_CALL();
	}
	
	VHOOK_DECL(bool, CTFBaseBossLocomotion_IsOnGround)
	{
		if (rc_CTFTankBoss_TankBossThink) {
			if (thinking_tank_data != nullptr && thinking_tank_data->gravity_set && thinking_tank_data->gravity <= 0) {
				return true;
			}
		}
		return VHOOK_CALL();
	}

	DETOUR_DECL_MEMBER(float, CTFBaseBossLocomotion_GetStepHeight)
	{
		if (rc_CTFTankBoss_TankBossThink) {
			if (thinking_tank_data != nullptr && thinking_tank_data->gravity_set && thinking_tank_data->gravity <= 0) {
				return 0.0f;
			}
		}
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, NextBotGroundLocomotion_Update)
	{
		float prev_pitch = NAN;
		float prev_posz = NAN;
		if (rc_CTFTankBoss_TankBossThink && thinking_tank_data != nullptr) {
			prev_posz = thinking_tank->GetAbsOrigin().z;
			if (thinking_tank_data->offsetz != 0.0f) {
				Vector offset = thinking_tank->GetAbsOrigin() - Vector(0, 0, thinking_tank_data->offsetz);
				thinking_tank->SetAbsOrigin(offset);
			}
			
			if (thinking_tank_data->gravity == 0.0f && thinking_tank_data->gravity_set)
			{
				auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
				Vector move = loco->GetApproachPos();

				move.NormalizeInPlace();
				
				Vector right, up;
				VectorVectors(move, right, up);
				loco->SetGroundNormal(up);
				
				prev_pitch = thinking_tank->GetLocalAngles().x;

				prev_posz += move.z * loco->GetRunSpeed() * loco->GetUpdateInterval();
			}
		}
		DETOUR_MEMBER_CALL();
		if (rc_CTFTankBoss_TankBossThink && thinking_tank_data != nullptr) {
			
			if (thinking_tank_data->gravity == 0.0f && thinking_tank_data->gravity_set)
			{
				Vector tank_pos = thinking_tank->GetAbsOrigin();
				tank_pos.z = prev_posz;
				thinking_tank->SetAbsOrigin(tank_pos);
			}

			if (thinking_tank_data->offsetz != 0.0f) {
				Vector offset = thinking_tank->GetAbsOrigin() + Vector(0, 0, thinking_tank_data->offsetz);
				thinking_tank->SetAbsOrigin(offset);
			}

			if (!thinking_tank_data->rotate_pitch) {
				QAngle ang = thinking_tank->GetLocalAngles();
				ang.x = 0.0;
				thinking_tank->SetLocalAngles(ang);
			}
		}
	}

	CValueOverride_ConVar<float> tf_base_boss_max_turn_rate("tf_base_boss_max_turn_rate");
	DETOUR_DECL_MEMBER(void, CTFBaseBossLocomotion_FaceTowards, const Vector& vec)
	{
		auto loco = reinterpret_cast<NextBotGroundLocomotion *>(this);
		auto tank = static_cast<CTFTankBoss *>(loco->GetBot()->GetEntity());
		
		SpawnerData *data = FindSpawnerDataForTank(tank);

		float prev_pitch = tank->GetLocalAngles().x;

		if (data != nullptr && data->max_turn_rate_set) {
			tf_base_boss_max_turn_rate.Set(data->max_turn_rate);
		}
	
		DETOUR_MEMBER_CALL(vec);

		tf_base_boss_max_turn_rate.Reset();
	}
	
	DETOUR_DECL_MEMBER(string_t, CTankSpawner_GetClassIcon, int index)
	{
		auto tank = reinterpret_cast<CTankSpawner *>(this);
		
		SpawnerData *data = &spawners[tank];
		DevMsg("Tank data found icon %d", data != nullptr);
		if (data != nullptr) {

			return data->icon;
		}
		
		return DETOUR_MEMBER_CALL(index);
	}

	int restOfCurrency = -1;
	RefCount rc_CTFTankBoss_Event_Killed;
	DETOUR_DECL_MEMBER(void, CTFTankBoss_Event_Killed, CTakeDamageInfo &info)
	{
		auto tank = reinterpret_cast<CTFTankBoss *>(this);
		auto currency = tank->GetCurrencyValue();
		if (currency > 1500) {
			CCurrencyPackCustom *pCurrencyPack = static_cast<CCurrencyPackCustom *>(CBaseEntity::CreateNoSpawn("item_currencypack_custom", tank->GetAbsOrigin(), QAngle(0, RandomFloat(-180, 180), 0), nullptr));
			if (pCurrencyPack != nullptr) {
				pCurrencyPack->SetAmount(currency - 1500);
				DispatchSpawn( pCurrencyPack );
				Vector vel = Vector(RandomFloat(-1, 1), 1, RandomFloat(-1, 1)).Normalized() * 250;
				pCurrencyPack->DropSingleInstance( vel, nullptr, 0, 0 );
			}
		}
		restOfCurrency = MIN(1500, currency);
		SCOPED_INCREMENT(rc_CTFTankBoss_Event_Killed);
		
		DETOUR_MEMBER_CALL(info);
		restOfCurrency = -1;
	}

	
	DETOUR_DECL_MEMBER(bool, CTankSpawner_IsMiniBoss, int index)
	{
		auto tank = reinterpret_cast<CTankSpawner *>(this);
		
		SpawnerData *data = &spawners[tank];
		DevMsg("Tank data found mini boss%d", data != nullptr);
		if (data != nullptr) {
			return data->is_miniboss;
		}
		
		return DETOUR_MEMBER_CALL(index);
	}
	
	DETOUR_DECL_MEMBER(bool, IPopulationSpawner_HasAttribute, CTFBot::AttributeType attr, int index)
	{
		if (attr == CTFBot::ATTR_ALWAYS_CRIT) {
			auto tank = reinterpret_cast<CTankSpawner *>(this);
			if (tank) {
				SpawnerData *data = &spawners[tank];
				DevMsg("Tank data found crit %d", data != nullptr);
				if (data != nullptr) {
					return data->is_crit;
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(attr, index);
	}

	RefCount rc_CTFTankBoss_UpdatePingSound;

	DETOUR_DECL_MEMBER(void, CTFTankBoss_UpdatePingSound)
	{
		SCOPED_INCREMENT(rc_CTFTankBoss_UpdatePingSound);
		
		DETOUR_MEMBER_CALL();
	}

	RefCount rc_CTFTankBoss_Spawn;
	DETOUR_DECL_MEMBER(void, CTFTankBoss_Spawn)
	{
		SCOPED_INCREMENT(rc_CTFTankBoss_Spawn);
		
		DETOUR_MEMBER_CALL();

	}

	DETOUR_DECL_MEMBER(void, CBaseEntity_EmitSound, const char *sound, float start, float *duration)
	{
		if (rc_CTFTankBoss_UpdatePingSound || rc_CTFTankBoss_TankBossThink || rc_CTankSpawner_Spawn) {
			SpawnerData *data = rc_CTankSpawner_Spawn ? &(spawners[current_spawner]) : thinking_tank_data;
			if (data != nullptr) {
				if (rc_CTFTankBoss_UpdatePingSound && data->sound_ping != "")
					sound = data->sound_ping.c_str();
				else if(rc_CTFTankBoss_TankBossThink && FStrEq(sound, "MVM.TankDeploy") && data->sound_deploy != "")
					sound = data->sound_deploy.c_str();
				else if (FStrEq(sound, "MVM.TankStart") && data->sound_start != "")
					sound = data->sound_start.c_str();
			}
		}
		DETOUR_MEMBER_CALL(sound, start, duration);
	}

	DETOUR_DECL_STATIC(void, CBaseEntity_EmitSound2, IRecipientFilter& filter, int iEntIndex, const char *sound, const Vector *pOrigin, float start, float *duration )
	{
		if (rc_CTFTankBoss_UpdatePingSound || rc_CTFTankBoss_TankBossThink || rc_CTankSpawner_Spawn) {
			SpawnerData *data = rc_CTankSpawner_Spawn ? &(spawners[current_spawner]) : thinking_tank_data;
			if (data != nullptr) {
				DevMsg("%s, %s\n",sound,data->sound_engine_loop.c_str());
				if (FStrEq(sound, "MVM.TankEngineLoop") && data->sound_engine_loop != "")
					sound = data->sound_engine_loop.c_str();
			}
		}
		DETOUR_STATIC_CALL(filter, iEntIndex, sound, pOrigin, start, duration);
	}

	RefCount rc_CTFTankBoss_Explode;
	DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
		if (rc_CTFTankBoss_Explode && thinking_tank_data != nullptr) {

			if (thinking_tank_data->attachements_destroy.size() > 0) {
				return nullptr;
			}
		}
		return DETOUR_STATIC_CALL(className, iForceEdictIndex);
	}

	DETOUR_DECL_MEMBER(void, CTFTankDestruction_Spawn)
	{
		DETOUR_MEMBER_CALL();
		auto destruction = reinterpret_cast<CBaseAnimating *>(this);
		if (thinking_tank_data != nullptr && thinking_tank_data->model_destruction != -1) {
			for (int i = 0; i < MAX_VISION_MODES; ++i) {
				destruction->SetModelIndexOverride(i, thinking_tank_data->model_destruction);
			}
		}
	}
	
	DETOUR_DECL_MEMBER(void, CTFTankBoss_Explode)
	{
		
		auto tank = reinterpret_cast<CTFTankBoss *>(this);
		
		SpawnerData *data = FindSpawnerDataForTank(tank);
		DevMsg("Tank killed way 2 %d", data != nullptr);
		if (data != nullptr) {
			thinking_tank = tank;
			thinking_tank_data = data;
		}

		++rc_CTFTankBoss_Explode;
		DETOUR_MEMBER_CALL();
		--rc_CTFTankBoss_Explode;

		if (thinking_tank_data != nullptr) {
			for (auto it1 = thinking_tank_data->attachements_destroy.begin(); it1 != thinking_tank_data->attachements_destroy.end(); ++it1) {
				if (it1->templ != nullptr) {
					auto inst = it1->templ->SpawnTemplate(nullptr, thinking_tank->GetAbsOrigin() + it1->translation, thinking_tank->GetAbsAngles() + it1->rotation);
				}
			}
		}
		
		thinking_tank = nullptr;
		thinking_tank_data = nullptr;
	}

	DETOUR_DECL_MEMBER(void, CTFTankBoss_UpdateOnRemove)
	{
		auto tank = reinterpret_cast<CTFTankBoss *>(this);
		
		SpawnerData *data = FindSpawnerDataForTank(tank);
		if (data != nullptr) {
			const char *name = STRING(data->icon);
			DevMsg("Tank killed icon %s", name);
			CTFObjectiveResource *res = TFObjectiveResource();
			res->DecrementMannVsMachineWaveClassCount(data->icon, 1 | 8);
			if (data->sound_engine_loop != "") {

				tank->StopSound(data->sound_engine_loop.c_str());
			}
		}

		DETOUR_MEMBER_CALL();
	}

	CBaseEntity *entityOnFireCollide;
	DETOUR_DECL_MEMBER(float, CTFFlameManager_GetFlameDamageScale, void * point, CTFPlayer * player)
	{
		float ret = DETOUR_MEMBER_CALL(point, player);
		bool istank = entityOnFireCollide != nullptr && strcmp(entityOnFireCollide->GetClassname(),"tank_boss") == 0;
		if (istank) {
			ret+=0.04f;
			if (ret > 1.0f)
				ret = 1.0f;
			if (ret < 0.77f) {
				ret = 0.77f;
			}
		}
		return ret;
	}
	
	DETOUR_DECL_MEMBER(void, CTFFlameManager_OnCollide, CBaseEntity* entity, int value)
	{
		entityOnFireCollide = entity;
		DETOUR_MEMBER_CALL(entity, value);
		entityOnFireCollide = nullptr;
	}

	RefCount rc_CTFBaseBoss_OnTakeDamage;
	RefCount rc_CTFBaseBoss_OnTakeDamage_SameTeam;
	DETOUR_DECL_MEMBER(int, CTFBaseBoss_OnTakeDamage, CTakeDamageInfo &info)
	{
		auto tank = reinterpret_cast<CTFBaseBoss *>(this);
		SCOPED_INCREMENT(rc_CTFBaseBoss_OnTakeDamage);
		SCOPED_INCREMENT_IF(rc_CTFBaseBoss_OnTakeDamage_SameTeam, info.GetAttacker() != nullptr && info.GetAttacker()->GetTeamNumber() == tank->GetTeamNumber());
		return DETOUR_MEMBER_CALL(info);
	}

	class CTakeDamageInfoTF2 : public CTakeDamageInfo
	{
	public:
		int m_eCritType;
	};

	DETOUR_DECL_MEMBER(int, CTFGameRules_ApplyOnDamageModifyRules, CTakeDamageInfo& info, CBaseEntity *pVictim, bool b1)
	{
		int result = DETOUR_MEMBER_CALL(info, pVictim, b1);
		if (rc_CTFBaseBoss_OnTakeDamage) {
			auto tank = reinterpret_cast<CTFTankBoss *>(pVictim);
			SpawnerData *data = FindSpawnerDataForTank(tank);
			if (data != nullptr && data->crit_immune) {
				info.SetDamage(info.GetDamage() - info.GetDamageBonus());
				info.SetDamageType(info.GetDamageType() & ~(DMG_CRITICAL));
				reinterpret_cast<CTakeDamageInfoTF2 *>(&info)->m_eCritType = 0;

			}
		}
		return result;
	}

	// Fix buildings being destroyed by trigger entities parented to the tank
	DETOUR_DECL_MEMBER(void, CTFBaseBoss_Touch, CBaseEntity *toucher)
	{
		SpawnerData *data = FindSpawnerDataForTank(reinterpret_cast<CTFTankBoss *>(this));
		if (data != nullptr && data->trigger_destroy_fix) {
			return;
		}
		DETOUR_MEMBER_CALL(toucher);
	}

	RefCount rc_CTFBaseBoss_ResolvePlayerCollision;
	DETOUR_DECL_MEMBER(void, CTFBaseBoss_ResolvePlayerCollision, CTFPlayer *toucher)
	{
		SpawnerData *data = FindSpawnerDataForBoss(reinterpret_cast<CTFBaseBoss *>(this));
		SCOPED_INCREMENT_IF(rc_CTFBaseBoss_ResolvePlayerCollision, data != nullptr && data->no_crush_damage);
		DETOUR_MEMBER_CALL(toucher);
	}

	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, const CTakeDamageInfo& info)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		if (rc_CTFBaseBoss_ResolvePlayerCollision || (rc_CTFTankBoss_TankBossThink && thinking_tank_data != nullptr&& thinking_tank_data->no_crush_damage) ) {
			return 0;
		}
		return DETOUR_MEMBER_CALL(info);
	}

	DETOUR_DECL_MEMBER(uint, CTFTankBossBody_GetSolidMask)
	{
		auto data = FindSpawnerDataForTank((CTFTankBoss *) reinterpret_cast<IBody *>(this)->GetBot()->GetEntity());
		if (data != nullptr && data->solid_to_brushes) {
			return CONTENTS_SOLID | CONTENTS_WINDOW | CONTENTS_MOVEABLE;
		}
		return DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_STATIC(void, UTIL_ScreenShake, const Vector &center, float amplitude, float frequency, float duration, float radius, int eCommand, bool bAirShake)
	{
		if (rc_CTFTankBoss_TankBossThink && thinking_tank_data != nullptr && thinking_tank_data->no_screen_shake) {
			return;
		}
		return DETOUR_STATIC_CALL(center, amplitude, frequency, duration, radius, eCommand, bAirShake);
	}

	DETOUR_DECL_STATIC(void, HandleRageGain, CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale)
	{
		if (rc_CTFBaseBoss_OnTakeDamage_SameTeam) return;
		DETOUR_STATIC_CALL(pPlayer, iRequiredBuffFlags, flDamage, fInverseRageGainScale);
	}

	DETOUR_DECL_MEMBER(int, CTFTankBoss_GetCurrencyValue)
	{
		if (rc_CTFTankBoss_Event_Killed && restOfCurrency != -1) return restOfCurrency;
		auto result = DETOUR_MEMBER_CALL();
		return result;
	}

    Mod::Etc::Mapentity_Additions::ClassnameFilter tank_boss_filter("tank_boss", {
        {"SetGravity"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            
			auto data = FindSpawnerDataForTank((CTFTankBoss *)ent);
			if (data != nullptr) {
				Value.Convert(FIELD_FLOAT);
				data->gravity_set = true;
				data->gravity = Value.Float();
			}
        }},
        {"SetImmobile"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            
			auto data = FindSpawnerDataForTank((CTFTankBoss *)ent);
			if (data != nullptr) {
				Value.Convert(FIELD_BOOLEAN);
				data->immobile = Value.Bool();
			}
        }},
        {"SetOffsetZ"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            
			auto data = FindSpawnerDataForTank((CTFTankBoss *)ent);
			if (data != nullptr) {
				Value.Convert(FIELD_FLOAT);
				data->offsetz = Value.Float();
			}
        }},
        {"SetTurnRate"sv, false, [](CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t &Value){
            
			auto data = FindSpawnerDataForTank((CTFTankBoss *)ent);
			if (data != nullptr) {
				Value.Convert(FIELD_FLOAT);
				data->max_turn_rate_set = true;
				data->max_turn_rate = Value.Float();
			}
        }},
    });


	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Pop:Tank_Extensions")
		{
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_dtor0, "CTankSpawner::~CTankSpawner [D0]");
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_dtor2, "CTankSpawner::~CTankSpawner [D2]");
			
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_Parse, "CTankSpawner::Parse");
			
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_Spawn, "CTankSpawner::Spawn");
			
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_TankBossThink,         "CTFTankBoss::TankBossThink");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetModelIndexOverride, "CBaseEntity::SetModelIndexOverride");
			
			MOD_ADD_DETOUR_MEMBER(CBaseAnimating_LookupAttachment, "CBaseAnimating::LookupAttachment");
			
			MOD_ADD_DETOUR_MEMBER(CTFBaseBossLocomotion_FaceTowards, "CTFBaseBossLocomotion::FaceTowards");
			MOD_ADD_VHOOK(CTFBaseBossLocomotion_GetGravity, "21CTFBaseBossLocomotion", "NextBotGroundLocomotion::GetGravity");
			MOD_ADD_VHOOK(CTFBaseBossLocomotion_IsOnGround, "21CTFBaseBossLocomotion", "NextBotGroundLocomotion::IsOnGround");
			MOD_ADD_DETOUR_MEMBER(NextBotGroundLocomotion_Update, "NextBotGroundLocomotion::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBaseBossLocomotion_GetStepHeight, "CTFBaseBossLocomotion::GetStepHeight");
			
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_GetClassIcon, "CTankSpawner::GetClassIcon");
			
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_IsMiniBoss, "CTankSpawner::IsMiniBoss");
			MOD_ADD_DETOUR_MEMBER(IPopulationSpawner_HasAttribute, "IPopulationSpawner::HasAttribute");
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_Event_Killed, "CTFTankBoss::Event_Killed");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_EmitSound, "CBaseEntity::EmitSound [member: normal]");
			MOD_ADD_DETOUR_STATIC(CBaseEntity_EmitSound2, "CBaseEntity::EmitSound [static: normal]");
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_UpdatePingSound, "CTFTankBoss::UpdatePingSound");
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_Spawn, "CTFTankBoss::Spawn");
			MOD_ADD_DETOUR_STATIC(CreateEntityByName, "CreateEntityByName");
			MOD_ADD_DETOUR_MEMBER(CTFTankDestruction_Spawn,      "CTFTankDestruction::Spawn");
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_Explode, "CTFTankBoss::Explode");
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_UpdateOnRemove, "CTFTankBoss::UpdateOnRemove");
			MOD_ADD_DETOUR_MEMBER(CTFBaseBoss_OnTakeDamage, "CTFBaseBoss::OnTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");
			//MOD_ADD_DETOUR_MEMBER(CBaseEntity_Touch, "CBaseEntity::Touch");
			MOD_ADD_DETOUR_MEMBER(CTFBaseBoss_Touch, "CTFBaseBoss::Touch");
			MOD_ADD_DETOUR_MEMBER(CTFBaseBoss_ResolvePlayerCollision, "CTFBaseBoss::ResolvePlayerCollision");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage,        "CTFPlayer::OnTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CTFTankBossBody_GetSolidMask,        "CTFTankBossBody::GetSolidMask");
			MOD_ADD_DETOUR_STATIC(UTIL_ScreenShake,        "UTIL_ScreenShake");

			// No rage gain for same team tanks
			MOD_ADD_DETOUR_STATIC(HandleRageGain,        "HandleRageGain");

			// Tank flame damage fix
			MOD_ADD_DETOUR_MEMBER(CTFFlameManager_GetFlameDamageScale,        "CTFFlameManager::GetFlameDamageScale");
			MOD_ADD_DETOUR_MEMBER(CTFFlameManager_OnCollide,        "CTFFlameManager::OnCollide");

			// Drop large amount of currency as a single pack
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_GetCurrencyValue,        "CTFTankBoss::GetCurrencyValue");
			
		}
		
		virtual void OnUnload() override
		{
			spawners.clear();
		}
		
		virtual void OnDisable() override
		{
			spawners.clear();
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void LevelInitPreEntity() override
		{
			spawners.clear();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			spawners.clear();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_tank_extensions", "0", FCVAR_NOTIFY,
		"Mod: enable extended KV in CTankSpawner::Parse",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CKVCond_Tank : public IKVCond
	{
	public:
		virtual bool operator()() override
		{
			return s_Mod.IsEnabled();
		}
	};
	CKVCond_Tank cond;
}
