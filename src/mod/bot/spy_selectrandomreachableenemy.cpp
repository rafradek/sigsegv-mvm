#include "mod.h"
#include "stub/tfbot.h"
#include "stub/tf_shareddefs.h"
#include "stub/tfentities.h"
#include "util/iterate.h"
#include "stub/tfbot_behavior.h"


namespace Mod::Bot::Spy_SelectRandomReachableEnemy
{
////////////////////////////////////////////////////////////////////////////////
////// The actual implementation in the game ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//	CTFPlayer *CTFBot::SelectRandomReachableEnemy()
//	{
//		CUtlVector<CTFPlayer *> alive_enemies;
//		CollectPlayers(&alive_enemies, bot->GetOpposingTFTeamNumber(), true);
//		
//		CUtlVector<CTFPlayer *> alive_enemies_not_in_spawn;
//		FOR_EACH_VEC(alive_enemies, i) {
//			CTFPlayer *enemy = alive_enemies[i];
//			
//			if (!PointInRespawnRoom(enemy, enemy->WorldSpaceCenter(), false)) {
//				alive_enemies_not_in_spawn.AddToTail(enemy);
//			}
//		}
//		
//		if (!alive_enemies_not_in_spawn.IsEmpty()) {
//			return alive_enemies_not_in_spawn.Random();
//		} else {
//			return nullptr;
//		}
//	}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

	CTFBot *bot;
	DETOUR_DECL_MEMBER(CTFPlayer *, CTFBot_SelectRandomReachableEnemy)
	{
		bot = reinterpret_cast<CTFBot *>(this);
		auto result = DETOUR_MEMBER_CALL();
		bot = nullptr;

		return result;
	}

	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotAttackFlagDefenders_Update, CTFBot *actor, float dt)
	{
		
		auto action = reinterpret_cast<CTFBotAttackFlagDefenders *>(this);
		CTFPlayer *player = action->m_chasePlayer;
		if (player != nullptr && actor->GetVisionInterface()->IsIgnored(player))
			action->m_chasePlayer = nullptr;
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		return result;
	}

	DETOUR_DECL_STATIC(int, CollectPlayers_CTFPlayer, CUtlVector<CTFPlayer *> *playerVector, int team, bool isAlive, bool shouldAppend)
	{
		if (bot != nullptr) {
			CUtlVector<CTFPlayer *> tempVector;
			DETOUR_STATIC_CALL(&tempVector, team, isAlive, shouldAppend);
			
			for (auto player : tempVector) {
				if (!bot->GetVisionInterface()->IsIgnored(player))
					playerVector->AddToTail(player);
			}

			return playerVector->Count();
		}
		return DETOUR_STATIC_CALL(playerVector, team, isAlive, shouldAppend);
	}

	// ISSUE: the callers of SelectRandomReachableEnemy may hang onto the enemy
	// they choose for a while; so if a spy is visible when a bot begins an AI
	// action, and then goes invisible, they might still get chased
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:Spy_SelectRandomReachableEnemy")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBot_SelectRandomReachableEnemy, "CTFBot::SelectRandomReachableEnemy");
			MOD_ADD_DETOUR_MEMBER(CTFBotAttackFlagDefenders_Update,   "CTFBotAttackFlagDefenders::Update");
			MOD_ADD_DETOUR_STATIC(CollectPlayers_CTFPlayer,   "CollectPlayers<CTFPlayer>");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_spy_selectrandomreachableenemy", "0", FCVAR_NOTIFY,
		"Mod: debug/fix bad spy logic in CTFBot::SelectRandomReachableEnemy",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
