#include "mod.h"
#include "stub/populators.h"
#include "stub/nextbot_cc.h"
#include "mod/pop/popmgr_extensions.h"
#include "mod/mvm/player_limit.h"
#include "util/value_override.h"
#include "util/scope.h"

namespace Mod::MvM::Sub_Wave_Spawn_At_Once
{
	int waveNum = 0;
    CValueOverride_ConVar<int> tf_mvm_max_invaders("tf_mvm_max_invaders");
	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		waveNum = 0;
		bool ret = DETOUR_MEMBER_CALL();
		auto manager = reinterpret_cast<CPopulationManager *>(this);
		int red, blu, spectators, robots;
		Mod::MvM::Player_Limit::GetSlotCounts(red, blu, spectators, robots);
		tf_mvm_max_invaders.Set(Max(tf_mvm_max_invaders.GetOriginalValue(), gpGlobals->maxClients - red - blu - 1));
		return ret;
	}

	DETOUR_DECL_MEMBER(bool, CWave_Parse, KeyValues *kv)
	{
		auto wave = reinterpret_cast<CWave *>(this);
		waveNum++;
		return DETOUR_MEMBER_CALL(kv);
	}
	DETOUR_DECL_MEMBER(bool, CWaveSpawnPopulator_Parse, KeyValues *kv)
	{
        bool result = DETOUR_MEMBER_CALL(kv);
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		wavespawn->m_waitForAllDead = "";
		wavespawn->m_waitForAllSpawned = "";
		wavespawn->m_waitBeforeStarting = 0;
		if (waveNum == 1) {
			g_pPopulationManager->m_nStartingCurrency += wavespawn->m_totalCurrency;
		}
        return result;
    }

	DETOUR_DECL_MEMBER(bool, CMissionPopulator_Parse, KeyValues *kv)
	{
		kv->SetInt("InitialCooldown", 0);
		return DETOUR_MEMBER_CALL(kv);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Sub_Wave_Spawn_At_Once")
		{
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_Parse,      "CWaveSpawnPopulator::Parse");
			MOD_ADD_DETOUR_MEMBER(CWave_Parse,      "CWave::Parse");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse,      "CPopulationManager::Parse");
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_Parse,      "CMissionPopulator::Parse");
		}

		virtual void OnDisable() override {
			tf_mvm_max_invaders.Reset();
		}
	};
	CMod s_Mod_Sub_Wave_Spawn_At_Once;
	
	
	ConVar sig_mvm_sub_wave_spawn_at_once("sig_mvm_sub_wave_spawn_at_once", "0", FCVAR_NOTIFY,
		"Mod: Spawn all subwaves at once",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod_Sub_Wave_Spawn_At_Once.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

namespace Mod::MvM::Wave_Spawn_At_Once
{
	int waveNum = 0;
	RefCount rc_CPopulationManager_Parse;
	
    CValueOverride_ConVar<int> tf_mvm_max_invaders("tf_mvm_max_invaders");
	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		SCOPED_INCREMENT(rc_CPopulationManager_Parse);
		waveNum = 0;
		auto ret = DETOUR_MEMBER_CALL();
		int red, blu, spectators, robots;
		Mod::MvM::Player_Limit::GetSlotCounts(red, blu, spectators, robots);
		tf_mvm_max_invaders.Set(Max(tf_mvm_max_invaders.GetOriginalValue(), Min(gpGlobals->maxClients - red - blu - 1, Mod::Pop::PopMgr_Extensions::GetMaxRobotLimit() * waveNum )));

		return ret;
	}

	RefCount rc_CPopulationManager_IsValidPopfile;
	DETOUR_DECL_MEMBER(bool, CPopulationManager_IsValidPopfile, CUtlString name)
	{
		SCOPED_INCREMENT(rc_CPopulationManager_IsValidPopfile);
		return DETOUR_MEMBER_CALL(name);
	}

	RefCount rc_Parse_Popfile;
    void Parse_Popfile(KeyValues* kv, IBaseFileSystem* filesystem)
    {
		SCOPED_INCREMENT(rc_Parse_Popfile);
		KeyValues *kvFirstWave = nullptr;
		std::vector<KeyValues *> del_kv;
		int waveNumParse = 0;
		FOR_EACH_SUBKEY(kv, subkey) {
			const char *name = subkey->GetName();
			if (FStrEq(name, "Wave")) {
				waveNumParse += 1;
				if (kvFirstWave == nullptr) {
					kvFirstWave = subkey;
				}
				else {
					FOR_EACH_SUBKEY(subkey, subkeyWave) {
						const char *nameSubkeyWave = subkeyWave->GetName();
						if (FStrEq(nameSubkeyWave, "WaveSpawn")) {
							FOR_EACH_SUBKEY(subkeyWave, subkeyWaveSpawn) {
								const char *nameSubkeyWaveSpawn = subkeyWaveSpawn->GetName();
								if (FStrEq(nameSubkeyWaveSpawn, "Name") || FStrEq(nameSubkeyWaveSpawn, "WaitForAllSpawned") || FStrEq(nameSubkeyWaveSpawn, "WaitForAllDead")) {
									subkeyWaveSpawn->SetString(nullptr, CFmtStr("%s-%d", subkeyWaveSpawn->GetString(), waveNumParse));
								}
							}
						}
						kvFirstWave->AddSubKey(subkeyWave->MakeCopy());
					}
					del_kv.push_back(subkey);
					
				}
			}
		}
		for (auto subkey : del_kv) {
		//	DevMsg("Deleting key \"%s\"\n", subkey->GetName());
			kv->RemoveSubKey(subkey);
			subkey->deleteThis();
		}
	}

	RefCount rc_KeyValues_LoadFromFile;
	DETOUR_DECL_MEMBER(bool, KeyValues_LoadFromFile, IBaseFileSystem *filesystem, const char *resourceName, const char *pathID, bool refreshCache)
	{
	//	DevMsg("KeyValues::LoadFromFile\n");
		
		++rc_KeyValues_LoadFromFile;
		
		auto result = DETOUR_MEMBER_CALL(filesystem, resourceName, pathID, refreshCache);
		--rc_KeyValues_LoadFromFile;
		
		if (result && rc_CPopulationManager_Parse > 0 && rc_KeyValues_LoadFromFile == 0 && rc_CPopulationManager_IsValidPopfile == 0 && rc_Parse_Popfile == 0) {
			auto kv = reinterpret_cast<KeyValues *>(this);

			Parse_Popfile(kv, filesystem);
		}
		
		return result;
	}

	DETOUR_DECL_MEMBER(bool, CWave_Parse, KeyValues *kv)
	{
		auto wave = reinterpret_cast<CWave *>(this);
		waveNum++;
		return DETOUR_MEMBER_CALL(kv);
	}
	DETOUR_DECL_MEMBER(bool, CWaveSpawnPopulator_Parse, KeyValues *kv)
	{
        bool result = DETOUR_MEMBER_CALL(kv);
		auto wavespawn = reinterpret_cast<CWaveSpawnPopulator *>(this);
		if (waveNum == 1) {
			g_pPopulationManager->m_nStartingCurrency += wavespawn->m_totalCurrency;
		}
        return result;
    }

	DETOUR_DECL_MEMBER(bool, CMissionPopulator_Parse, KeyValues *kv)
	{
		kv->SetInt("BeginAtWave", 1);
		return DETOUR_MEMBER_CALL(kv);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Wave_Spawn_At_Once")
		{
			MOD_ADD_DETOUR_MEMBER(CWaveSpawnPopulator_Parse,      "CWaveSpawnPopulator::Parse");
			MOD_ADD_DETOUR_MEMBER(CWave_Parse,      "CWave::Parse");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_Parse,      "CPopulationManager::Parse");
			MOD_ADD_DETOUR_MEMBER(KeyValues_LoadFromFile,      "KeyValues::LoadFromFile");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_IsValidPopfile,      "CPopulationManager::IsValidPopfile");
			MOD_ADD_DETOUR_MEMBER(CMissionPopulator_Parse,      "CMissionPopulator::Parse");
		}

		virtual void OnDisable() override {
			tf_mvm_max_invaders.Reset();
		}
	};
	CMod s_Mod_Wave_Spawn_At_Once;
	
	
	ConVar sig_mvm_wave_spawn_at_once("sig_mvm_wave_spawn_at_once", "0", FCVAR_NOTIFY,
		"Mod: Spawn all waves at once",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod_Wave_Spawn_At_Once.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
