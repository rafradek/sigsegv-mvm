#include "mod.h"
#include "stub/tfplayer.h"
#include "util/scope.h"


namespace Mod::Debug::FeignDeath_Ragdoll
{
	RefCount rc_CreateFeignDeathRagdoll;
	DETOUR_DECL_MEMBER(void, CTFPlayer_CreateFeignDeathRagdoll, const CTakeDamageInfo& info, bool b1, bool b2, bool b3)
	{
		SCOPED_INCREMENT(rc_CreateFeignDeathRagdoll);
		DETOUR_MEMBER_CALL(info, b1, b2, b3);
	}
	
	DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
		auto result = DETOUR_STATIC_CALL(className, iForceEdictIndex);
		
		if (rc_CreateFeignDeathRagdoll > 0 && strcmp(className, "tf_ragdoll") == 0 && result != nullptr) {
			auto ragdoll = reinterpret_cast<CTFRagdoll *>(result);
			
			ragdoll->m_flHeadScale  = 1.0f;
			ragdoll->m_flTorsoScale = 1.0f;
			ragdoll->m_flHandScale  = 1.0f;
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:FeignDeath_Ragdoll")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CreateFeignDeathRagdoll, "CTFPlayer::CreateFeignDeathRagdoll");
			MOD_ADD_DETOUR_STATIC(CreateEntityByName,                "CreateEntityByName");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_feigndeath_ragdoll", "0", FCVAR_NOTIFY,
		"Debug: investigate spy croissant issues",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
