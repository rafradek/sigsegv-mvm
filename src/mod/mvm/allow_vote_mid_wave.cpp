
#include "mod.h"
#include "stub/gamerules.h"

namespace Mod::MvM::Allow_Vote_Mid_Wave
{
	DETOUR_DECL_MEMBER(int, CKickIssue_RequestCallVote, int var1, const char *var2, vote_create_failed_t &fail, int &var4)
	{
		bool restoreRound = false;
		if (TFGameRules()->IsMannVsMachineMode() && TFGameRules()->State_Get() == GR_STATE_RND_RUNNING) {
			restoreRound = true;
			TFGameRules()->State_SetDirect(GR_STATE_BETWEEN_RNDS);
		}
		auto result = DETOUR_MEMBER_CALL(CKickIssue_RequestCallVote)(var1, var2, fail, var4);
		if (restoreRound) {
			TFGameRules()->State_SetDirect(GR_STATE_RND_RUNNING);
		}
        return result;
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Misc:AllowVoteMidWave")
		{
			MOD_ADD_DETOUR_MEMBER(CKickIssue_RequestCallVote, "CKickIssue::RequestCallVote");
		}
	};
	CMod s_AllowVoteMidWave;

	ConVar sig_mvm_allow_vote_mid_wave("sig_mvm_allow_vote_mid_wave", "0", FCVAR_NOTIFY,
		"Mod: Allow kick voting mid wave",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_AllowVoteMidWave.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}