#include "mod.h"
#include "stub/populators.h"
#include "stub/nextbot_cc.h"
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
	DETOUR_DECL_MEMBER(bool, CWaveSpawnPopulator_Parse, KeyValues *kv)
	{
		static ConVarRef robotMax("sig_mvm_robot_limit_override");
        bool result = DETOUR_MEMBER_CALL(CWaveSpawnPopulator_Parse)(kv);
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);

		// Not a tank
		if (rtti_cast<CTankSpawner *>(wavespawn->m_Spawner) == nullptr) {
			float spawnMult = MIN(robotMax.GetInt()/22, cvar_enable.GetInt());
			float timeMult = spawnMult / cvar_enable.GetInt();
			wavespawn->m_spawnCount = MIN(robotMax.GetInt(),ceil(wavespawn->m_spawnCount * spawnMult));
			if (timeMult < 1) {
				wavespawn->m_waitBetweenSpawns *= timeMult;
			}
			wavespawn->m_maxActive = MIN(robotMax.GetInt(),ceil(wavespawn->m_maxActive * spawnMult));
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
		wavespawn->m_totalCount *= cvar_enable.GetInt();
		
		wavespawn->m_totalCurrency *= sig_mvm_robot_multiplier_currency.GetFloat();
        return result;
    }
	
	DETOUR_DECL_MEMBER(bool, CTankSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CTankSpawner_Spawn)(where, ents);
		
		if (result && ents != nullptr && !ents->IsEmpty()) {
			auto tank = rtti_cast<CTFTankBoss *>(ents->Tail().Get());
			if (tank != nullptr) {
				tank->SetMaxHealth(tank->GetMaxHealth() * sig_mvm_robot_multiplier_tank_hp.GetFloat());
				tank->SetHealth(tank->GetHealth() * sig_mvm_robot_multiplier_tank_hp.GetFloat());
			}
		}
		return result;
	}

	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		bool ret = DETOUR_MEMBER_CALL(CPopulationManager_Parse)();
		auto manager = reinterpret_cast<CPopulationManager *>(this);
		manager->m_nStartingCurrency = manager->m_nStartingCurrency * (manager->m_nStartingCurrency < 3000 ? sig_mvm_robot_multiplier_currency_start.GetFloat() : sig_mvm_robot_multiplier_currency.GetFloat());
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CMissionPopulator_Parse, KeyValues *kv)
	{
		static ConVarRef robotMax("sig_mvm_robot_limit_override");
		auto mission = reinterpret_cast<CMissionPopulator *>(this);

		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			
			if (FStrEq(name, "DesiredCount")) {
				subkey->SetInt(nullptr, subkey->GetInt() * ceil(MIN(robotMax.GetInt()/22, cvar_enable.GetInt())) );
			}
		}
		
		return DETOUR_MEMBER_CALL(CMissionPopulator_Parse)(kv);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Sapper_Allow_Multiple_Active")
		{
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_Parse,      "CWaveSpawnPopulator::Parse");
			MOD_ADD_DETOUR_MEMBER(CTankSpawner_Spawn,      "CTankSpawner::Spawn");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse,      "CPopulationManager::Parse");
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_Parse,      "CMissionPopulator::Parse");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_robot_multiplier", "0", FCVAR_NOTIFY,
		"Mod: multiply robots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}