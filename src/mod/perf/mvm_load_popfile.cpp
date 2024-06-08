#ifdef SE_IS_TF2
#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tf_objective_resource.h"
#include "util/scope.h"
#include "util/backtrace.h"

namespace Mod::Perf::MvM_Load_Popfile
{
	RefCount rc_CPopulationManager_SetPopulationFilename;
	DETOUR_DECL_MEMBER(void, CPopulationManager_SetPopulationFilename, const char *filename)
    {
		SCOPED_INCREMENT(rc_CPopulationManager_SetPopulationFilename);
		TFObjectiveResource()->m_iszMvMPopfileName = AllocPooledString(filename);
        DETOUR_MEMBER_CALL(filename);
    }

	RefCount rc_tf_mvm_popfile;
	DETOUR_DECL_STATIC(void, tf_mvm_popfile, const CCommand& args)
	{
		SCOPED_INCREMENT(rc_tf_mvm_popfile);
		DETOUR_STATIC_CALL(args);
	}
	
	DETOUR_DECL_MEMBER(void, CPopulationManager_ResetMap)
	{
		if (rc_tf_mvm_popfile > 0 && rc_CPopulationManager_SetPopulationFilename == 0) return;
		DETOUR_MEMBER_CALL();
	}
	
	RefCount rc_CPopulationManager_JumpToWave;
	DETOUR_DECL_MEMBER(void, CPopulationManager_JumpToWave, unsigned int wave, float f1)
	{
		SCOPED_INCREMENT_IF(rc_CPopulationManager_JumpToWave, f1 == 0);
		DETOUR_MEMBER_CALL(wave, f1);
	}
	
	RefCount rc_CTFGameRules_SetupOnRoundStart;
	DETOUR_DECL_MEMBER(void, CTFGameRules_SetupOnRoundStart)
	{
		SCOPED_INCREMENT(rc_CTFGameRules_SetupOnRoundStart);
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(bool, CPopulationManager_Parse)
	{
		if (rc_CPopulationManager_JumpToWave > 0 && rc_CTFGameRules_SetupOnRoundStart == 0) return true;
		return DETOUR_MEMBER_CALL();
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:MvM_Load_Popfile")
		{
			MOD_ADD_DETOUR_STATIC(tf_mvm_popfile,              "tf_mvm_popfile");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CPopulationManager_ResetMap, "CPopulationManager::ResetMap", HIGHEST);
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_SetPopulationFilename, "CPopulationManager::SetPopulationFilename");
			MOD_ADD_DETOUR_MEMBER_PRIORITY(CPopulationManager_Parse, "CPopulationManager::Parse", HIGHEST);
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_SetupOnRoundStart, "CTFGameRules::SetupOnRoundStart");
			MOD_ADD_DETOUR_MEMBER(CPopulationManager_JumpToWave, "CPopulationManager::JumpToWave");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_mvm_load_popfile", "0", FCVAR_NOTIFY,
		"Mod: eliminate unnecessary duplication of parsing/init code during tf_mvm_popfile",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif