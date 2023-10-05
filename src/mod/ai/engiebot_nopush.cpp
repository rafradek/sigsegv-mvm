#include "mod.h"
#include "util/scope.h"
#include "re/nextbot.h"

#include "stub/tfbot.h"

namespace Mod::AI::EngieBot_NoPush
{
	RefCount rc_TeleSpawn_Update;
	CTFBot *pusher_engineer;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMvMEngineerTeleportSpawn_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_TeleSpawn_Update);
		pusher_engineer = actor;
		return DETOUR_MEMBER_CALL(CTFBotMvMEngineerTeleportSpawn_Update)(actor, dt);
	}
	
	RefCount rc_BuildSentry_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>,CTFBotMvMEngineerBuildSentryGun_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_BuildSentry_Update);
		pusher_engineer = actor;
		return DETOUR_MEMBER_CALL(CTFBotMvMEngineerBuildSentryGun_Update)(actor, dt);
	}
	
	RefCount rc_BuildTele_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMvMEngineerBuildTeleportExit_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_BuildTele_Update);
		pusher_engineer = actor;
		return DETOUR_MEMBER_CALL(CTFBotMvMEngineerBuildTeleportExit_Update)(actor, dt);
	}

	ConVar cvar_reducerange("sig_ai_engiebot_pushrange", "0", FCVAR_NOTIFY | FCVAR_GAMEDLL,
		"Mod: reduce push range");

	/* would prefer to detour CTFGameRules::PushAllPlayersAway, but it's hard to reliably locate on Windows */
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyAbsVelocityImpulse, const Vector *v1)
	{
		if ((rc_TeleSpawn_Update > 0 || rc_BuildSentry_Update > 0 || rc_BuildTele_Update > 0) &&cvar_reducerange.GetFloat() == 0.0f) {
			return;
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_ApplyAbsVelocityImpulse)(v1);
	}
	
	DETOUR_DECL_MEMBER(void, CTFGameRules_PushAllPlayersAway, const Vector& vFromThisPoint, float flRange, float flForce, int nTeam, CUtlVector< CTFPlayer* > *pPushedPlayers)
	{
		if (rc_TeleSpawn_Update > 0 || rc_BuildSentry_Update > 0 || rc_BuildTele_Update > 0) {
			if (pusher_engineer->GetTeamNumber() == TF_TEAM_RED)
				nTeam = TF_TEAM_BLUE;
		}
		if ((rc_TeleSpawn_Update > 0 || rc_BuildSentry_Update > 0 || rc_BuildTele_Update > 0) && cvar_reducerange.GetFloat() > 0.0f) {
			flRange = cvar_reducerange.GetFloat();

		}
		
		DETOUR_MEMBER_CALL(CTFGameRules_PushAllPlayersAway)(vFromThisPoint, flRange, flForce, nTeam, pPushedPlayers);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:EngieBot_NoPush")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerTeleportSpawn_Update,     "CTFBotMvMEngineerTeleportSpawn::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerBuildSentryGun_Update,    "CTFBotMvMEngineerBuildSentryGun::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerBuildTeleportExit_Update, "CTFBotMvMEngineerBuildTeleportExit::Update");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyAbsVelocityImpulse,         "CTFPlayer::ApplyAbsVelocityImpulse");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_PushAllPlayersAway,         "CTFGameRules::PushAllPlayersAway");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_engiebot_nopush", "0", FCVAR_NOTIFY,
		"Mod: remove MvM engiebots' push force when spawning and building",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
