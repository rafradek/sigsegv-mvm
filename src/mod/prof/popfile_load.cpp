#include "mod.h"
#include "stub/baseentity.h"
#include "stub/entities.h"
#include "util/scope.h"
#include "util/stringpool.h"

class CEntityMapData;


namespace Mod::Prof::Popfile_Load
{
	StringPool pool(true);
	
	
	#define VPROF_CUSTOM(name) \
		VPROF_BUDGET(pool(name), pool("CUSTOM"))
	
	
	DETOUR_DECL_STATIC(void, tf_mvm_popfile, const CCommand& args)
	{
		float t1 = Plat_FloatTime();
		{
			VPROF_CUSTOM("tf_mvm_popfile");
			DETOUR_STATIC_CALL(tf_mvm_popfile)(args);
		}
		float t2 = Plat_FloatTime();
		float dt = (t2 - t1);
		Msg("[PROF] tf_mvm_popfile execution took %.2f ms.\n", 1000.0f * dt);
	}
	
	
	// NOTE: CPopulationManager::JumpToWave is going to be the real perf issue
	
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_ParseMapData, CEntityMapData *mapData)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (rtti_cast<CSmokeStack *>(ent) != nullptr) {
			DevMsg("\n> CSmokeStack #%d: CBaseEntity::ParseMapData\n", ENTINDEX(ent));
		}
		
		DETOUR_MEMBER_CALL(CBaseEntity_ParseMapData)(mapData);
	}
	
	RefCount rc_CSmokeStack_KeyValue;
	DETOUR_DECL_MEMBER(bool, CSmokeStack_KeyValue, const char *szKeyName, const char *szValue)
	{
		auto smokestack = reinterpret_cast<CSmokeStack *>(this);
		
		SCOPED_INCREMENT(rc_CSmokeStack_KeyValue);
		
		DevMsg("  > KeyValue \"%s\" \"%s\"\n", szKeyName, szValue);
		
		return DETOUR_MEMBER_CALL(CSmokeStack_KeyValue)(szKeyName, szValue);
	}
	
	DETOUR_DECL_MEMBER(bool, CBaseFileSystem_FileExists, const char *pFileName, const char *pPathID)
	{
		auto result = DETOUR_MEMBER_CALL(CBaseFileSystem_FileExists)(pFileName, pPathID);
		
		if (rc_CSmokeStack_KeyValue > 0) {
			DevMsg("    > FileExists \"%s\" \"%s\"  --> %s\n", pFileName, pPathID, (result ? "TRUE" : "FALSE"));
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Prof:Popfile_Load")
		{
			MOD_ADD_DETOUR_STATIC(tf_mvm_popfile, "tf_mvm_popfile");
			
		//	MOD_ADD_DETOUR_MEMBER(CBaseEntity_ParseMapData,   "CBaseEntity::ParseMapData");
		//	MOD_ADD_DETOUR_MEMBER(CSmokeStack_KeyValue,       "CSmokeStack::KeyValue");
		//	MOD_ADD_DETOUR_MEMBER(CBaseFileSystem_FileExists, "CBaseFileSystem::FileExists");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_prof_popfile_load", "0", FCVAR_NOTIFY,
		"Mod: profile MvM popfile load performance",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
