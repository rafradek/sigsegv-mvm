
#include "mod.h"
#include "stub/tfplayer.h"

namespace Mod::MvM::Spectator_Vote_Fix
{
	DETOUR_DECL_MEMBER_CALL_CONVENTION(__gcc_regcall, int, CVoteController_TryCastVote, int index, const char *issue)
	{
		auto player = ToTFPlayer(UTIL_PlayerByIndex(index));
        static ConVarRef sv_vote_allow_spectators("sv_vote_allow_spectators");
        if (sv_vote_allow_spectators.GetBool() && player != nullptr && player->GetTeamNumber() <= TEAM_SPECTATOR) {
            return 3;
        }
		auto result = DETOUR_MEMBER_CALL(CVoteController_TryCastVote)(index, issue);
        return result;
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Misc:SpectatorVoteFix")
		{
			MOD_ADD_DETOUR_MEMBER(CVoteController_TryCastVote, "CVoteController::TryCastVote [clone]");
		}
	};
	CMod s_Mod;

	ConVar cvar_enable("sig_etc_spectator_vote_fix", "0", FCVAR_NOTIFY,
		"Mod: Respect sv_vote_allow_spectators when counting votes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}