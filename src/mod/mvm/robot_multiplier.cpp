#include "mod.h"
#include "stub/populators.h"
#include "stub/nextbot_cc.h"
#include "mod/pop/popmgr_extensions.h"
#include "util/value_override.h"
#include "util/scope.h"

namespace Mod::MvM::Robot_Multiplier
{
    extern ConVar cvar_enable;
	ConVar sig_mvm_robot_multiplier_tank_hp("sig_mvm_robot_multiplier_tank_hp", "0.5", FCVAR_NOTIFY,
		"Tank HP multiplier for robot multiplier mode");
	ConVar sig_mvm_robot_multiplier_currency("sig_mvm_robot_multiplier_currency", "1.5", FCVAR_NOTIFY,
		"Cash multiplier for robot multiplier mode");
	ConVar sig_mvm_robot_multiplier_currency_start("sig_mvm_robot_multiplier_currency_start", "2", FCVAR_NOTIFY,
		"Starting cash multiplier for robot multiplier mode");


    CValueOverride_ConVar<int> tf_mvm_max_invaders("tf_mvm_max_invaders");
	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		bool ret = DETOUR_MEMBER_CALL();
		auto manager = reinterpret_cast<CPopulationManager *>(this);
		manager->m_nStartingCurrency = manager->m_nStartingCurrency * (manager->m_nStartingCurrency < 3000 ? sig_mvm_robot_multiplier_currency_start.GetFloat() : sig_mvm_robot_multiplier_currency.GetFloat());
		int realMult = cvar_enable.GetInt() * 22 / Mod::Pop::PopMgr_Extensions::GetMaxRobotLimit();
		
		tf_mvm_max_invaders.Set(realMult * Mod::Pop::PopMgr_Extensions::GetMaxRobotLimit());
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CWaveSpawnPopulator_Parse, KeyValues *kv)
	{
        bool result = DETOUR_MEMBER_CALL(kv);
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		int realMult = cvar_enable.GetInt() * 22 / Mod::Pop::PopMgr_Extensions::GetMaxRobotLimit();

		// Not a tank
		if (rtti_cast<CTankSpawner *>(wavespawn->m_Spawner) == nullptr) {
			float spawnMult = realMult;
			float timeMult = spawnMult / cvar_enable.GetInt();
			wavespawn->m_spawnCount = wavespawn->m_spawnCount * spawnMult;
			if (timeMult < 1) {
				wavespawn->m_waitBetweenSpawns *= MAX(0.25f, timeMult);
			}
			wavespawn->m_maxActive = wavespawn->m_maxActive * spawnMult;
		}
		// Tank
		else {
			wavespawn->m_maxActive *= cvar_enable.GetInt();
			if (wavespawn->m_waitBetweenSpawns < 5) {
				wavespawn->m_waitBetweenSpawns = 5;
			}
			else {
				wavespawn->m_waitBetweenSpawns /= cvar_enable.GetInt();
			}
		}
		wavespawn->m_totalCount *= realMult;
		
		wavespawn->m_totalCurrency *= sig_mvm_robot_multiplier_currency.GetFloat();
        return result;
    }
	
	DETOUR_DECL_MEMBER(bool, CTankSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
		auto result = DETOUR_MEMBER_CALL(where, ents);
		
		if (result && ents != nullptr && !ents->IsEmpty()) {
			auto tank = rtti_cast<CTFTankBoss *>(ents->Tail().Get());
			if (tank != nullptr) {
				tank->SetMaxHealth(tank->GetMaxHealth() * sig_mvm_robot_multiplier_tank_hp.GetFloat());
				tank->SetHealth(tank->GetHealth() * sig_mvm_robot_multiplier_tank_hp.GetFloat());
			}
		}
		return result;
	}

	DETOUR_DECL_MEMBER(bool, CMissionPopulator_Parse, KeyValues *kv)
	{
		static ConVarRef robotMax("tf_mvm_max_invaders");
		auto mission = reinterpret_cast<CMissionPopulator *>(this);
		int realMult = cvar_enable.GetInt() * 22 / Mod::Pop::PopMgr_Extensions::GetMaxRobotLimit();

		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "DesiredCount")) {
				subkey->SetInt(nullptr, subkey->GetInt() * realMult );
			}
		}
		
		return DETOUR_MEMBER_CALL(kv);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Robot_Multiplier")
		{
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_Parse,      "CWaveSpawnPopulator::Parse");
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_Spawn,      "CTankSpawner::Spawn");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse,      "CPopulationManager::Parse");
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_Parse,      "CMissionPopulator::Parse");
		}

		virtual void OnDisable() override {
			tf_mvm_max_invaders.Reset();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_robot_multiplier", "0", FCVAR_NOTIFY,
		"Mod: multiply robots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}