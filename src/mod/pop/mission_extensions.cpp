#include "mod.h"
#include "mod/pop/kv_conditional.h"
#include "stub/populators.h"
#include "stub/objects.h"
#include "util/scope.h"


namespace Mod::Pop::Mission_Extensions
{
	struct MissionData
	{
		bool suppress_sentrybuster_model = false;
		bool target_blu_sentries = false;
	};
	
	
	std::map<CMissionPopulator *, MissionData> missions;
	
	
	DETOUR_DECL_MEMBER(void, CMissionPopulator_dtor0)
	{
		auto mission = reinterpret_cast<CMissionPopulator *>(this);
		
	//	DevMsg("CMissionPopulator %08x: dtor0\n", (uintptr_t)mission);
		missions.erase(mission);
		
		DETOUR_MEMBER_CALL(CMissionPopulator_dtor0)();
	}
	
	DETOUR_DECL_MEMBER(void, CMissionPopulator_dtor2)
	{
		auto mission = reinterpret_cast<CMissionPopulator *>(this);
		
	//	DevMsg("CMissionPopulator %08x: dtor2\n", (uintptr_t)mission);
		missions.erase(mission);
		
		DETOUR_MEMBER_CALL(CMissionPopulator_dtor2)();
	}
	
	
	DETOUR_DECL_MEMBER(bool, CMissionPopulator_Parse, KeyValues *kv)
	{
		auto mission = reinterpret_cast<CMissionPopulator *>(this);
		
		DevMsg("CMissionPopulator::Parse\n");
		
		std::vector<KeyValues *> del_kv;
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			bool del = true;
			if (FStrEq(name, "SuppressSentryBusterModel")) {
			//	DevMsg("Got \"SuppressSentryBusterModel\" = %d\n", subkey->GetBool());
				missions[mission].suppress_sentrybuster_model = subkey->GetBool();
			} else if (FStrEq(name, "TargetBluSentries")) {
			//	DevMsg("Got \"SuppressSentryBusterModel\" = %d\n", subkey->GetBool());
				missions[mission].target_blu_sentries = subkey->GetBool();
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
		
		return DETOUR_MEMBER_CALL(CMissionPopulator_Parse)(kv);
	}
	
	
	RefCount rc_CMissionPopulator_UpdateMissionDestroySentries;
	CMissionPopulator *current_mission = nullptr;
	DETOUR_DECL_MEMBER(bool, CMissionPopulator_UpdateMissionDestroySentries)
	{
		auto mission = reinterpret_cast<CMissionPopulator *>(this);
		
		std::vector<CBaseObject *> sentriesToRestoreTeam;
		auto it = missions.find(mission);
		if (it != missions.end()) {
			MissionData& data = (*it).second;
			if (data.target_blu_sentries) {
				for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
					auto obj = rtti_scast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
					if (obj != nullptr && obj->GetType() == OBJ_SENTRYGUN && obj->GetTeamNumber() == TF_TEAM_BLUE) {
						sentriesToRestoreTeam.push_back(obj);
						obj->SetTeamNumber(TF_TEAM_RED);
					}
				}
			}
		}

		SCOPED_INCREMENT(rc_CMissionPopulator_UpdateMissionDestroySentries);
		current_mission = mission;

		auto ret = DETOUR_MEMBER_CALL(CMissionPopulator_UpdateMissionDestroySentries)();

		for (auto obj : sentriesToRestoreTeam) {
			obj->SetTeamNumber(obj->GetTeamNumber() == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED);
		}
		return ret;
		
	//	DevMsg("CMissionPopulator::UpdateMissionDestroySentries %08x\n", (uintptr_t)this);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayerClassShared_SetCustomModel, const char *s1, bool b1)
	{
		if (rc_CMissionPopulator_UpdateMissionDestroySentries > 0 && current_mission != nullptr && s1 != nullptr && FStrEq(s1, "models/bots/demo/bot_sentry_buster.mdl")) {
			auto it = missions.find(current_mission);
			if (it != missions.end()) {
				MissionData& data = (*it).second;
				
				if (data.suppress_sentrybuster_model) {
					DevMsg("Suppressing automatic mission populator sentry buster model change (%08x)\n", (uintptr_t)current_mission);
					return;
				}
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayerClassShared_SetCustomModel)(s1, b1);
	}

	
	class CMod : public IMod, public IModCallbackListener
	{
	public:
		CMod() : IMod("Pop:Mission_Extensions")
		{
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_dtor0, "CMissionPopulator::~CMissionPopulator [D0]");
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_dtor2, "CMissionPopulator::~CMissionPopulator [D2]");
			
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_Parse, "CMissionPopulator::Parse");
			
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_UpdateMissionDestroySentries, "CMissionPopulator::UpdateMissionDestroySentries");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayerClassShared_SetCustomModel, "CTFPlayerClassShared::SetCustomModel");
		}
		
		virtual void OnUnload() override
		{
			missions.clear();
		}
		
		virtual void OnDisable() override
		{
			missions.clear();
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void LevelInitPreEntity() override
		{
			missions.clear();
		}
		
		virtual void LevelShutdownPostEntity() override
		{
			missions.clear();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_mission_extensions", "0", FCVAR_NOTIFY,
		"Mod: enable extended KV in CMissionPopulator::Parse",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CKVCond_Mission : public IKVCond
	{
	public:
		virtual bool operator()() override
		{
			return s_Mod.IsEnabled();
		}
	};
	CKVCond_Mission cond;
}
