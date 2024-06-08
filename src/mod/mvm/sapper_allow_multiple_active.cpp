#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tf_shareddefs.h"
#include "util/scope.h"


namespace Mod::MvM::Sapper_Allow_Multiple_Active
{
	RefCount rc_CTFPlayer_CanBuild__sapper;
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall,int, CTFPlayer_CanBuild, int iObjectType, int iObjectMode)
	{
		SCOPED_INCREMENT_IF(rc_CTFPlayer_CanBuild__sapper, iObjectType == OBJ_ATTACHMENT_SAPPER);
		return DETOUR_MEMBER_CALL(iObjectType, iObjectMode);
	}
	
	
	DETOUR_DECL_MEMBER(int, CTFPlayer_GetNumObjects, int iObjectType, int iObjectMode)
	{
		if (iObjectType == OBJ_ATTACHMENT_SAPPER && iObjectMode == -1 && rc_CTFPlayer_CanBuild__sapper > 0 && TFGameRules()->IsMannVsMachineMode()) {
			return 0;
		}
		
		return DETOUR_MEMBER_CALL(iObjectType, iObjectMode);
		
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Sapper_Allow_Multiple_Active")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanBuild,      "CTFPlayer::CanBuild [clone]");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetNumObjects, "CTFPlayer::GetNumObjects");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_sapper_allow_multiple_active", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: allow multiple sappers to exist per player simultaneously in MvM mode",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
