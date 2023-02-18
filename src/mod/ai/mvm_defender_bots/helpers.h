#ifndef _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_HELPERS_H_
#define _INCLUDE_SIGSEGV_MOD_AI_MVM_DEFENDER_BOTS_HELPERS_H_


#include "stub/gamerules.h"
#include "stub/tfentities.h"
#include "stub/tfbot.h"


inline void ForEachPlayer(const std::function<void(CBasePlayer *, bool&)>& functor)
{
	for (int i = 1; i < gpGlobals->maxClients; ++i) {
		CBasePlayer *player = UTIL_PlayerByIndex(i);
		if (player == nullptr)  continue;
		
		bool done = false;
		functor(player, done);
		if (done) break;
	}
}


inline void ForEachDefenderBot(const std::function<void(CTFBot *, bool&)>& functor)
{
	for (int i = 1; i < gpGlobals->maxClients; ++i) {
		CBasePlayer *player = UTIL_PlayerByIndex(i);
		if (player == nullptr) continue;
		
		if (player->GetTeamNumber() != TF_TEAM_RED) continue;
		
		CTFBot *bot = ToTFBot(player);
		if (bot == nullptr) continue;
		
		bool done = false;
		functor(bot, done);
		if (done) break;
	}
}


inline CCaptureZone *GetCaptureZone()
{
	/* return the first blu CCaptureZone (same as CTFBot::GetFlagCaptureZone) */
	
	for (auto elem : ICaptureZoneAutoList::AutoList()) {
		auto zone = static_cast<CCaptureZone *>(elem);
		if (zone == nullptr) continue;
		
		if (zone->GetTeamNumber() == TF_TEAM_BLUE) return zone;
	}
	
	return nullptr;
}


#endif
