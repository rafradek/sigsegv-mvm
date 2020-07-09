#include "mod.h"
#include "mod/pop/kv_conditional.h"
#include "stub/entities.h"
#include "stub/populators.h"
#include "stub/strings.h"
#include "stub/misc.h"
#include "util/scope.h"
#include "mod/pop/pointtemplate.h"
#include "stub/tf_objective_resource.h"

namespace Mod::Pop::Tank_Extensions
{

	struct SpawnerData
	{
		bool disable_smokestack =  false;
		float scale             =  1.00f;
		bool force_romevision   =  false;
		float max_turn_rate     =    NAN;
		std::string icon        = "";
		bool is_miniboss        =   true;
		bool is_crit           	=  false;
		bool disable_models     =   false;
		bool immobile          =   false;
		std::string custom_model=   "";
		
		std::vector<CHandle<CTFTankBoss>> tanks;
		std::vector<PointTemplateInfo> attachements;
	};
	
	
	std::map<CTankSpawner *, SpawnerData> spawners;
	
	
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
		
		DETOUR_MEMBER_CALL(CTankSpawner_dtor0)();
	}
	
	DETOUR_DECL_MEMBER(void, CTankSpawner_dtor2)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
	//	DevMsg("CTankSpawner %08x: dtor2\n", (uintptr_t)spawner);

		spawners.erase(spawner);
		
		DETOUR_MEMBER_CALL(CTankSpawner_dtor2)();
	}
	

	DETOUR_DECL_MEMBER(bool, CTankSpawner_Parse, KeyValues *kv)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
		DevMsg("CTankSpawner::Parse\n");
		
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
			} else if (FStrEq(name, "ClassIcon")) {
			//	DevMsg("Got \"IconOverride\" = \"%s\"\n", subkey->GetString());
				spawners[spawner].icon = subkey->GetString();
			} else if (FStrEq(name, "IsCrit")) {
			//	DevMsg("Got \"IconOverride\" = \"%s\"\n", subkey->GetString());
				spawners[spawner].is_crit = subkey->GetBool();
			} else if (FStrEq(name, "IsMiniBoss")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].is_miniboss = subkey->GetBool();
			} else if (FStrEq(name, "Model")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].custom_model = subkey->GetString();
			} else if (FStrEq(name, "DisableChildModels")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].disable_models = subkey->GetBool();
			} else if (FStrEq(name, "Immobile")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].immobile = subkey->GetBool();
			} else if (FStrEq(name, "SpawnTemplate")) {
			//	DevMsg("Got \"IsMiniBoss\" = %d\n", subkey->GetBool());
				spawners[spawner].attachements.push_back(Parse_SpawnTemplate(subkey));
				//Parse_PointTemplate(spawner, subkey);
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
		
		return DETOUR_MEMBER_CALL(CTankSpawner_Parse)(kv);
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
		
		l_print_overrides(tank, "[BEFORE]");
		
		// primary method
		
		int mode_to_use = romevision ? VISION_MODE_ROME : VISION_MODE_NONE;
		
		for (int i = 0; i < MAX_VISION_MODES; ++i) {
			tank->SetModelIndexOverride(i, modelinfo->GetModelIndex(s_TankModelRome[tank->m_iModelIndex]));
		}
		// alternative method (probably less reliable than the one above)
	//	l_copy_rome_to_all_overrides(tank);
		
		l_print_overrides(tank, "[AFTER] ");
		
		
		for (CBaseEntity *child = tank->FirstMoveChild(); child != nullptr; child = child->NextMovePeer()) {
			if (!child->ClassMatches("prop_dynamic")) continue;
			
			DevMsg("\n");
			DevMsg("  child [classname \"%s\"] [model \"%s\"]\n", child->GetClassname(), STRING(child->GetModelName()));
			
			l_print_overrides(child, "[BEFORE]");
			
			for (int i = 0; i < MAX_VISION_MODES; ++i) {
				if (i == VISION_MODE_ROME) continue;
				child->SetModelIndexOverride(i, child->m_nModelIndexOverrides[VISION_MODE_ROME]);
			}
			
			l_print_overrides(child, "[AFTER] ");
		}
	}
	
	

	RefCount rc_CTankSpawner_Spawn;
	CTankSpawner *current_spawner = nullptr;
	DETOUR_DECL_MEMBER(int, CTankSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
		DevMsg("CTankSpawner::Spawn %08x\n", (uintptr_t)this);
		
		SCOPED_INCREMENT(rc_CTankSpawner_Spawn);
		current_spawner = spawner;
		
		auto result = DETOUR_MEMBER_CALL(CTankSpawner_Spawn)(where, ents);
		

		if (ents != nullptr) {
			auto it = spawners.find(spawner);
			if (it != spawners.end()) {
				SpawnerData& data = (*it).second;
				
				FOR_EACH_VEC((*ents), i) {
					CBaseEntity *ent = (*ents)[i];
					
					auto tank = rtti_cast<CTFTankBoss *>(ent);
					if (tank != nullptr) {
						data.tanks.push_back(tank);
						
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

						
						if (data.disable_models) {
							for (CBaseEntity *child = tank->FirstMoveChild(); child != nullptr; child = child->NextMovePeer()) {
								if (!child->ClassMatches("prop_dynamic")) continue;
								child->AddEffects(32);
							}
						}

						if (!data.custom_model.empty()) {
							//tank->SetModel( data.custom_model.c_str());
							CBaseEntity::PrecacheModel(data.custom_model.c_str());
							for (int i = 0; i < MAX_VISION_MODES; ++i) {
								tank->SetModelIndexOverride(i, modelinfo->GetModelIndex(data.custom_model.c_str()));
							}
							DevMsg("Vision node: %d",modelinfo->GetModelIndex(data.custom_model.c_str()));
							/*CBaseEntity *prop = CreateEntityByName("prop_dynamic");
							prop->KeyValue("model", data.custom_model.c_str());
							prop->SetAbsOrigin(tank->GetAbsOrigin());
							prop->SetAbsAngles(tank->GetAbsAngles());
							variant_t variant1;
							variant1.SetString(MAKE_STRING("!activator"));
							prop->AcceptInput("setparent", tank, tank,variant1,-1);
							DispatchSpawn(prop);
							prop->Activate();
							tank->AddEffects(32);*/
						}

						for (auto it1 = data.attachements.begin(); it1 != data.attachements.end(); ++it1) {
							it1->SpawnTemplate(tank);
						}
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
		DETOUR_MEMBER_CALL(CTFTankBoss_TankBossThink)();

		if (data != nullptr && data->immobile) {
			tank->SetAbsOrigin(vec);
			tank->SetAbsAngles(ang);
			tank->m_hCurrentNode  = nullptr;
		}
		
		if (node != nullptr && tank->m_hCurrentNode == nullptr && data != nullptr && !data->attachements.size() > 0) {
			variant_t variant;
			variant.SetString(MAKE_STRING(""));
			tank->AcceptInput("FireUser4",tank,tank,variant,-1);
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
			if (data->force_romevision || sig_no_romevision_cosmetics.GetBool()) {
			//	DevMsg("SetModelIndexOverride(%d, %d) for ent #%d \"%s\" \"%s\"\n", index, nValue, ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetModelName()));
				
				if (ent == tank) {
					if (data->force_romevision && index == VISION_MODE_ROME || (sig_no_romevision_cosmetics.GetBool() && index == VISION_MODE_NONE)) {
						DETOUR_MEMBER_CALL(CBaseEntity_SetModelIndexOverride)(VISION_MODE_NONE, nValue);
						DETOUR_MEMBER_CALL(CBaseEntity_SetModelIndexOverride)(VISION_MODE_ROME, nValue);
					}
					return;
				}
			}
			if (!data->custom_model.empty()) {
			//	DevMsg("SetModelIndexOverride(%d, %d) for ent #%d \"%s\" \"%s\"\n", index, nValue, ENTINDEX(ent), ent->GetClassname(), STRING(ent->GetModelName()));
				
				if (ent == tank) {
						DETOUR_MEMBER_CALL(CBaseEntity_SetModelIndexOverride)(VISION_MODE_NONE, modelinfo->GetModelIndex(data->custom_model.c_str()));
						DETOUR_MEMBER_CALL(CBaseEntity_SetModelIndexOverride)(VISION_MODE_ROME, modelinfo->GetModelIndex(data->custom_model.c_str()));
				}
			//	if (ent->GetMoveParent() == tank && ent->ClassMatches("prop_dynamic")) {
			//		DevMsg("Blocking SetModelIndexOverride(%d, %d) for tank %d prop %d \"%s\"\n", index, nValue, ENTINDEX(tank), ENTINDEX(ent), STRING(ent->GetModelName()));
			//		return;
			//	}
			}
		}
		
		DETOUR_MEMBER_CALL(CBaseEntity_SetModelIndexOverride)(index, nValue);
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
		
		return DETOUR_MEMBER_CALL(CBaseAnimating_LookupAttachment)(szName);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBaseBossLocomotion_FaceTowards, const Vector& vec)
	{
		auto loco = reinterpret_cast<ILocomotion *>(this);
		auto tank = rtti_cast<CTFTankBoss *>(loco->GetBot()->GetEntity());
		
		static ConVarRef tf_base_boss_max_turn_rate("tf_base_boss_max_turn_rate");
		
		SpawnerData *data = FindSpawnerDataForTank(tank);
		if (data != nullptr && !std::isnan(data->max_turn_rate) && tf_base_boss_max_turn_rate.IsValid()) {
			float saved_rate = tf_base_boss_max_turn_rate.GetFloat();
			tf_base_boss_max_turn_rate.SetValue(data->max_turn_rate);
			
			DETOUR_MEMBER_CALL(CTFBaseBossLocomotion_FaceTowards)(vec);
			
			tf_base_boss_max_turn_rate.SetValue(saved_rate);
		} else {
			DETOUR_MEMBER_CALL(CTFBaseBossLocomotion_FaceTowards)(vec);
		}
	}
	
	
	DETOUR_DECL_MEMBER(string_t, CTankSpawner_GetClassIcon, int index)
	{
		auto tank = reinterpret_cast<CTankSpawner *>(this);
		
		SpawnerData *data = &spawners[tank];
		DevMsg("Tank data found icon %d", data != nullptr);
		if (data != nullptr && data->icon.size() > 0) {
			return AllocPooledString(data->icon.c_str());
		}
		
		return DETOUR_MEMBER_CALL(CTankSpawner_GetClassIcon)(index);
	}

	DETOUR_DECL_MEMBER(void, CTFTankBoss_Event_Killed, CTakeDamageInfo &info)
	{
		auto tank = reinterpret_cast<CTFTankBoss *>(this);
		
		SpawnerData *data = FindSpawnerDataForTank(tank);
		DevMsg("Tank killed way 2 %d", data != nullptr);
		if (data != nullptr && data->icon.size() > 0) {
			const char *name = data->icon.c_str();
			CTFObjectiveResource *res = TFObjectiveResource();
			bool found = false;
			for (int i = 0; i < 12; ++i) {
				if (FStrEq(name,STRING(res->m_iszMannVsMachineWaveClassNames[i]))) {
					res->m_nMannVsMachineWaveClassCounts.SetArray(res->m_nMannVsMachineWaveClassCounts[i]-1,i);
					if (res->m_nMannVsMachineWaveClassCounts[i] < 0) {
						res->m_nMannVsMachineWaveClassCounts.SetArray(0,i);
						res->m_bMannVsMachineWaveClassActive.SetArray(false,i);
					}
					found = true;
					break;
				}
			}
			
			if (!found)
				for (int i = 0; i < 12; ++i) {
					if (FStrEq(name,STRING(res->m_iszMannVsMachineWaveClassNames2[i]))) {
						res->m_nMannVsMachineWaveClassCounts2.SetArray(res->m_nMannVsMachineWaveClassCounts2[i]-1,i);
						if (res->m_nMannVsMachineWaveClassCounts2[i] < 0) {
							res->m_nMannVsMachineWaveClassCounts2.SetArray(0,i);
							res->m_bMannVsMachineWaveClassActive2.SetArray(false,i);
						}
					}
				}
		}
		
		DETOUR_MEMBER_CALL(CTFTankBoss_Event_Killed)(info);
	}

	DETOUR_DECL_MEMBER(bool, CTankSpawner_IsMiniBoss, int index)
	{
		auto tank = reinterpret_cast<CTankSpawner *>(this);
		
		SpawnerData *data = &spawners[tank];
		DevMsg("Tank data found mini boss%d", data != nullptr);
		if (data != nullptr) {
			return data->is_miniboss;
		}
		
		return DETOUR_MEMBER_CALL(CTankSpawner_IsMiniBoss)(index);
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
		
		return DETOUR_MEMBER_CALL(IPopulationSpawner_HasAttribute)(attr, index);
	}

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
			
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_GetClassIcon, "CTankSpawner::GetClassIcon");
			
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_IsMiniBoss, "CTankSpawner::IsMiniBoss");
			MOD_ADD_DETOUR_MEMBER(IPopulationSpawner_HasAttribute, "IPopulationSpawner::HasAttribute");
			MOD_ADD_DETOUR_MEMBER(CTFTankBoss_Event_Killed, "CTFTankBoss::Event_Killed");
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
