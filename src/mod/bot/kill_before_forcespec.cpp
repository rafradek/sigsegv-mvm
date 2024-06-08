#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Bot::Kill_Before_ForceSpec
{
	ConVar cvar_force("sig_bot_kill_before_forcespec_force", "1", FCVAR_NOTIFY,
		"Mod: value to pass for bForce parameter of CommitSuicide");
	
	
	DETOUR_DECL_MEMBER(int, CTFPlayer_OnTakeDamage, const CTakeDamageInfo& info)
	{
		auto bot = ToTFBot(reinterpret_cast<CTFPlayer *>(this));
		if (bot != nullptr) {
			DevMsg("CTFPlayer::OnTakeDamage [bot #%d] [team %d] [health %d] [isalive %s]\n",
				ENTINDEX(bot), bot->GetTeamNumber(), bot->GetHealth(), (bot->IsAlive() ? "true" : "false"));
		}
		
		return DETOUR_MEMBER_CALL(info);
	}
	
	
	// called from:
	// CTFBotMissionSuicideBomber::Detonate
	DETOUR_DECL_MEMBER(void, CTFPlayer_ForceChangeTeam, int iTeamNum, bool b1)
	{
		auto bot = ToTFBot(reinterpret_cast<CTFPlayer *>(this));
		if (bot != nullptr) {
			DevMsg("CTFPlayer::ForceChangeTeam [bot #%d] [from team %d] [to team %d] [health %d] [isalive %s]\n",
				ENTINDEX(bot), bot->GetTeamNumber(), iTeamNum, bot->GetHealth(), (bot->IsAlive() ? "true" : "false"));
			
			if (iTeamNum == TEAM_SPECTATOR && bot->GetTeamNumber() != TEAM_SPECTATOR && bot->IsAlive()) {
				DevMsg("- Making bot commit suicide...\n");
				bot->CommitSuicide(false, cvar_force.GetBool());
				DevMsg("- Post-suicide info:\n");
				DevMsg("  bot->IsAlive():       %s\n", (bot->IsAlive() ? "true" : "false"));
				DevMsg("  bot->GetHealth():     %d\n", bot->GetHealth());
				DevMsg("  bot->GetTeamNumber(): %d\n", bot->GetTeamNumber());
				return;
			}
		}
		
		DETOUR_MEMBER_CALL(iTeamNum, b1);
	}
	
	
	// called from:
	// CWaveSpawnPopulator::ForceFinish
	// CTFGameRules::FireGameEvent
	// CSquadSpawner::Spawn
	// this one causes bad things to happen when wave failure occurs
	DETOUR_DECL_MEMBER(void, CTFBot_ChangeTeam, int iTeamNum, bool bAutoTeam, bool bSilent, bool b3)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		if (iTeamNum == TEAM_SPECTATOR && bot->GetTeamNumber() != TEAM_SPECTATOR && bot->IsAlive()) {
			bot->CommitSuicide(false, true);
			return;
		}
		
		DETOUR_MEMBER_CALL(iTeamNum, bAutoTeam, bSilent, b3);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:Kill_Before_ForceSpec")
		{
		//	MOD_ADD_DETOUR_MEMBER(CTFPlayer_OnTakeDamage, "CTFPlayer::OnTakeDamage");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ForceChangeTeam, "CTFPlayer::ForceChangeTeam");
			
		//	MOD_ADD_DETOUR_MEMBER(CTFBot_ChangeTeam,         "CTFBot::ChangeTeam");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_kill_before_forcespec", "0", FCVAR_NOTIFY,
		"Mod: fix bugs caused by bots not being killed before being switched to spectator",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
