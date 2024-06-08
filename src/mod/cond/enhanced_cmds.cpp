#include "mod.h"
#include "stub/tfplayer.h"
#include "util/misc.h"


namespace Mod::Cond::Enhanced_Cmds
{
	// existing addcond syntax:
	// addcond [condnum] [duration] [player name substring]
	
	// existing removecond syntax:
	// removecond [condnum] [player name substring]
	
	
	// desired features:
	// - allow hex cond number input
	// - allow condition name input
	// - regex selection of target player(s)
	// - apply to multiple players at once if regex matches multiple
	// - print what was done to console (with both cond num and name, plus duration for addcond)
	// - on invalid syntax or bad cond etc, print Warning message
	// - have feature to print list of conds (number + name): maybe "listconds"
	
	
	// ETFCond GetTFConditionFromName(const char *name);
	// const char *GetTFConditionName(ETFCond cond);
	
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ClientCommand, const CCommand& args)
	{
		if (FStrEq(args[0], "addcond")) {
		//	int iMaxCondNum = GetNumberOfTFConds() - 1;
			
			#warning TODO: Cond:Enhanced_Cmds
			// ...
			
			return true;
		}
		
		if (FStrEq(args[0], "removecond")) {
		//	int iMaxCondNum = GetNumberOfTFConds() - 1;
			
			#warning TODO: Cond:Enhanced_Cmds
			// ...
			
			return true;
		}
		
		if (FStrEq(args[0], "listconds")) {
		//	int iMaxCondNum = GetNumberOfTFConds() - 1;
			
			#warning TODO: Cond:Enhanced_Cmds
			
			for (int i = 0; IsValidTFConditionNumber(i); ++i) {
				// ...
			}
			
			return true;
		}
		
		return DETOUR_MEMBER_CALL(args);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Cond:Enhanced_Cmds")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ClientCommand, "CTFPlayer::ClientCommand");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_cond_enhanced_cmds", "0", FCVAR_NOTIFY,
		"Mod: enhance addcond and removecond",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


// existing implementation
#if 0
bool CTFPlayer::ClientCommand(const CCommand& args)
{
	// ...
	
	if (FStrEq(args[0], "addcond")) {
		if (sv_cheats.GetBool() && args.ArgC() >= 2) {
			CTFPlayer *pTarget = this;
			if (args.ArgC() >= 4) {
				for (int i = 1; i <= gpGlobals->maxClients; ++i) {
					CTFPlayer *pPlayer = ToTFPlayer(UTIL_PlayerByIndex(i));
					if (pPlayer == nullptr) continue;
					
					if (V_strstr(pPlayer->GetPlayerName(), args[3]) != nullptr) {
						pTarget = pPlayer;
						break;
					}
				}
			}
			
			int iCond = Clamp(strtol(args[1], nullptr, 10), 0, TF_COND_LAST - 1));
			float flDuration = (args.ArgC() >= 3 ? strtod(args[2], nullptr) : -1.0f);
			pTarget->m_Shared.AddCond(iCond, flDuration);
		}
		return true;
	}
	
	if (FStrEq(args[0], "removecond")) {
		if (sv_cheats.GetBool() && args.ArgC() >= 2) {
			CTFPlayer *pTarget = this;
			if (args.ArgC() >= 3) {
				for (int i = 1; i <= gpGlobals->maxClients; ++i) {
					CTFPlayer *pPlayer = ToTFPlayer(UTIL_PlayerByIndex(i));
					if (pPlayer == nullptr) continue;
					
					if (V_strstr(pPlayer->GetPlayerName(), args[3]) != nullptr) {
						pTarget = pPlayer;
						break;
					}
				}
			}
			
			int iCond = Clamp(strtol(args[1], nullptr, 10), 0, TF_COND_LAST - 1));
			pTarget->m_Shared.RemoveCond(iCond);
		}
		return true;
	}
	
	// ...
}
#endif
