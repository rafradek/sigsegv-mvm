#include "mod.h"
#include "stub/entities.h"
#include "stub/misc.h"
#include "util/misc.h"


namespace Mod::Perf::SmokeStack_FS_Thrash
{
	DETOUR_DECL_MEMBER(bool, CSmokeStack_KeyValue, const char *szKeyName, const char *szValue)
	{
		auto smokestack = reinterpret_cast<CSmokeStack *>(this);
		
		if (FStrEq(szKeyName, "SmokeMaterial")) {
			if (V_stristr(szValue, ".vmt") != nullptr) {
				smokestack->m_strMaterialModel = AllocPooledString(szValue);
			} else {
				char str[512];
				V_snprintf(str, sizeof(str), "%s.vmt", szValue);
				smokestack->m_strMaterialModel = AllocPooledString(str);
			}
			
			const char *pName = STRING((string_t)smokestack->m_strMaterialModel);
			smokestack->m_iMaterialModel = CBaseEntity::PrecacheModel(pName);
			
			return true;
		}
		
		return DETOUR_MEMBER_CALL(CSmokeStack_KeyValue)(szKeyName, szValue);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:SmokeStack_FS_Thrash")
		{
			MOD_ADD_DETOUR_MEMBER(CSmokeStack_KeyValue, "CSmokeStack::KeyValue");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_smokestack_fs_thrash", "0", FCVAR_NOTIFY,
		"Mod: fix map load delays caused by filesystem thrashing from env_smokestack entities",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
