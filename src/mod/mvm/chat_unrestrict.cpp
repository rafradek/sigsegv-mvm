#include "mod.h"
#include "stub/baseplayer.h"
#include "stub/gamerules.h"


namespace Mod::MvM::Chat_Unrestrict
{
	DETOUR_DECL_MEMBER(bool, CTFPlayer_CanHearAndReadChatFrom, CBasePlayer *them)
	{
		if (TFGameRules()->IsMannVsMachineMode()) {
			/* allow chat from all players, but block chat from actual bots */
			if (them != nullptr && them->IsBot()) {
				return false;
			} else {
				return true;
			}
		}
		
		return DETOUR_MEMBER_CALL(them);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Chat_Unrestrict")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_CanHearAndReadChatFrom, "CTFPlayer::CanHearAndReadChatFrom");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_chat_unrestrict", "0", FCVAR_NOTIFY,
		"Mod: allow players on any team and with any life state to chat with each other",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
