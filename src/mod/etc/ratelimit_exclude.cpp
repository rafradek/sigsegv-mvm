#include "mod.h"
#include "util/misc.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>


namespace Mod::Etc::RateLimit_Exclude
{
	std::set<std::string> cmd_strs;
	
	ConVar cvar_commands("sig_etc_ratelimit_exclude_commands", "", FCVAR_NOTIFY,
		"Mod: space-delimited list of commands to exclude from ratelimiting",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			std::string cvar_str(static_cast<ConVar *>(pConVar)->GetString());
			boost::split(cmd_strs, cvar_str, boost::is_any_of(" "), boost::token_compress_on);
			cmd_strs.erase(""s);
			
		//	DevMsg("RATELIMIT EXCLUSION COMMAND STRINGS [%zu]:\n", cmd_strs.size());
		//	for (const auto& str : cmd_strs) {
		//		DevMsg("- \"%s\"\n", str.c_str());
		//	}
		});
	
	
	DETOUR_DECL_MEMBER(bool, CBaseMultiplayerPlayer_ShouldRunRateLimitedCommand, const char *pszCommand)
	{
		/* bypass the 0.3-second ratelimit restriction if this command is in the list */
		for (const auto& str : cmd_strs) {
			if (V_stricmp(pszCommand, str.c_str()) == 0) return true;
		}
		
		return DETOUR_MEMBER_CALL(pszCommand);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:RateLimit_Exclude")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseMultiplayerPlayer_ShouldRunRateLimitedCommand, "CBaseMultiplayerPlayer::ShouldRunRateLimitedCommand [str]");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_ratelimit_exclude", "0", FCVAR_NOTIFY,
		"Mod: exclude selected client commands from server-side ratelimiting",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
