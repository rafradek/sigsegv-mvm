#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/gamerules.h"
#include "stub/tf_shareddefs.h"


namespace Mod::MvM::ChangeClass_AnyTime
{
	/* this mod merely bypasses the server-side restriction in
	 * CTFPlayer::HandleCommand_JoinClass;
	 * 
	 * the client has an additional check in changeclass, but it can be avoided
	 * by using joinclass or join_class in console, which sends a command
	 * directly to the server
	 *
	 * annoyingly, both checks print the same message, so it's easy to get the
	 * two confused with each other
	 */
	
	
	RefCount rc_CTFPlayer_HandleCommand_JoinClass;
	DETOUR_DECL_MEMBER(void, CTFPlayer_HandleCommand_JoinClass, const char *pClassName, bool b1)
	{
//		DevMsg("CTFPlayer::HandleCommand_JoinClass [pClassName: \"%s\"] [b1: %s]\n",
//			pClassName, (b1 ? "true" : "false"));
		
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		SCOPED_INCREMENT_IF(rc_CTFPlayer_HandleCommand_JoinClass,
			TFGameRules()->IsMannVsMachineMode() &&
			player->GetTeamNumber() == TF_TEAM_RED);
//		DevMsg("- IsMannVsMachineMode: %s\n", (TFGameRules()->IsMannVsMachineMode() ? "true" : "false"));
//		DevMsg("- GetTeamNumber: %d\n", player->GetTeamNumber());
		
		DETOUR_MEMBER_CALL(pClassName, b1);
	}
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsReadyToPlay)
	{
//		DevMsg("CTFPlayer::IsReadyToPlay\n");
		
		if (rc_CTFPlayer_HandleCommand_JoinClass > 0) {
//			DevMsg("- override: return false\n");
			return false;
		}
		
		auto result = DETOUR_MEMBER_CALL();
//		DevMsg("- passthru: return %s\n", (result ? "true" : "false"));
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:ChangeClass_AnyTime")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_HandleCommand_JoinClass, "CTFPlayer::HandleCommand_JoinClass");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsReadyToPlay,           "CTFPlayer::IsReadyToPlay");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_changeclass_anytime", "0", FCVAR_NOTIFY,
		"Mod: allow changing class during waves",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
