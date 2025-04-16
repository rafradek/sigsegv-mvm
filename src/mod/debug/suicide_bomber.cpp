#include "mod.h"
#include "stub/tfbot_behavior.h"
#include "util/scope.h"


// win syms:
// CTFBot::IsLineOfFireClear_vec
// 


namespace Mod::Debug::Suicide_Bomber
{
	const char *ActionResult_ToString(const ActionResult<CTFBot>& result)
	{
		static char buf[1024];
		
		switch (result.transition) {
		case ActionTransition::CONTINUE:
			snprintf(buf, sizeof(buf), "CONTINUE");
			break;
		case ActionTransition::CHANGE_TO:
			snprintf(buf, sizeof(buf), "CHANGE_TO: %s", result.action->GetName());
			break;
		case ActionTransition::SUSPEND_FOR:
			snprintf(buf, sizeof(buf), "SUSPEND_FOR: %s", result.action->GetName());
			break;
		case ActionTransition::DONE:
			snprintf(buf, sizeof(buf), "DONE");
			break;
		default:
			snprintf(buf, sizeof(buf), "???");
			break;
		}
		
		return buf;
	}
	
	
	void DrawOverlays(CTFBotMissionSuicideBomber *action, CTFBot *actor)
	{
		constexpr float dt = 0.1f;
		
		constexpr float target_cross_size = 5.0f;
		constexpr int   target_cross_r    = 0x00;
		constexpr int   target_cross_g    = 0xff;
		constexpr int   target_cross_b    = 0x00;
		
		constexpr float det_cross_size = 5.0f;
		constexpr int   det_cross_r    = 0xff;
		constexpr int   det_cross_g    = 0x00;
		constexpr int   det_cross_b    = 0x00;
		
		
		int line;
		char buf[1024];
		
		
		/* OVERLAYS: BOT */
		line = 0;
		
		auto pos = actor->GetAbsOrigin();
		snprintf(buf, sizeof(buf), "POS: (%4.0f, %4.0f, %4.0f)", pos.x, pos.y, pos.z);
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		snprintf(buf, sizeof(buf), "HEALTH: %d", actor->GetHealth());
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		CBaseEntity *target = action->m_hTarget;
		if (target == nullptr) {
			snprintf(buf, sizeof(buf), "TARGET: null");
		} else {
			const char *name;
			if (target->IsPlayer()) {
				name = static_cast<CBasePlayer *>(target)->GetPlayerName();
			} else {
				name = STRING(target->GetEntityName());
			}
			snprintf(buf, sizeof(buf), "TARGET: #%d '%s' '%s'",
				ENTINDEX(target), target->GetClassname(), name);
		}
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		snprintf(buf, sizeof(buf), "PATH FAILS: %d", action->m_nConsecutivePathFailures);
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		if (action->m_ctRecomputePath.HasStarted()) {
			snprintf(buf, sizeof(buf), "TIMER(path): %.1f / %.1f",
				action->m_ctRecomputePath.GetElapsedTime(),
				action->m_ctRecomputePath.GetCountdownDuration());
		} else {
			snprintf(buf, sizeof(buf), "TIMER(path): NOT STARTED");
		}
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		if (action->m_ctPlaySound.HasStarted()) {
			snprintf(buf, sizeof(buf), "TIMER(sound): %.1f / %.1f",
				action->m_ctPlaySound.GetElapsedTime(),
				action->m_ctPlaySound.GetCountdownDuration());
		} else {
			snprintf(buf, sizeof(buf), "TIMER(sound): NOT STARTED");
		}
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		if (action->m_ctDetonation.HasStarted()) {
			snprintf(buf, sizeof(buf), "TIMER(det): %.1f / %.1f",
				action->m_ctDetonation.GetElapsedTime(),
				action->m_ctDetonation.GetCountdownDuration());
		} else {
			snprintf(buf, sizeof(buf), "TIMER(det): NOT STARTED");
		}
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		snprintf(buf, sizeof(buf), "DETONATING: %s",
			(action->m_bDetonating ? "yes" : "no"));
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		snprintf(buf, sizeof(buf), "DET(reached_goal): %s",
			(action->m_bDetReachedGoal ? "yes" : "no"));
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		snprintf(buf, sizeof(buf), "DET(lost_all_health): %s",
			(action->m_bDetLostAllHealth ? "yes" : "no"));
		NDebugOverlay::EntityText(ENTINDEX(actor), line++, buf, dt, 255, 255, 255, 255);
		
		ConVarRef tf_bot_suicide_bomb_range("tf_bot_suicide_bomb_range");
		float det_range = tf_bot_suicide_bomb_range.GetFloat() / 3.0f;
		
		int det_sphere_r = 0xff;
		int det_sphere_g = 0x00;
		int det_sphere_b = 0x00;
		int det_sphere_a = 0x00;
		
		if (action->m_vecTargetPos.DistToSqr(actor->GetAbsOrigin()) < det_range * det_range) {
			Vector vec_clear = action->m_vecTargetPos;
			vec_clear.z += 18.0f;
			
			if (actor->IsLineOfFireClear(vec_clear)) {
				det_sphere_r = 0x00;
				det_sphere_g = 0xff;
				det_sphere_b = 0x00;
			} else {
				det_sphere_r = 0xff;
				det_sphere_g = 0xff;
				det_sphere_b = 0x00;
			}
		}
		
		NDebugOverlay::Sphere(actor->GetAbsOrigin(), vec3_angle, det_range,
			det_sphere_r, det_sphere_g, det_sphere_b, det_sphere_a, true, dt);
		
		
		/* OVERLAYS: TARGET POS */
		line = 0;
		
		snprintf(buf, sizeof(buf), "TARGET POS: (%4.0f, %4.0f, %4.0f)",
			action->m_vecTargetPos.x, action->m_vecTargetPos.y, action->m_vecTargetPos.z);
		NDebugOverlay::EntityTextAtPosition(action->m_vecTargetPos, line++, buf, dt, 255, 255, 255, 255);
		
		snprintf(buf, sizeof(buf), "DIST TO: %.1f", action->m_vecTargetPos.DistToSqr(actor->GetAbsOrigin()));
		
		NDebugOverlay::Cross3D(action->m_vecTargetPos,
			target_cross_size, target_cross_r, target_cross_g, target_cross_b, true, dt);
		
		
		/* OVERLAYS: DETONATE POS */
		line = 0;
		
		snprintf(buf, sizeof(buf), "DET POS: (%4.0f, %4.0f, %4.0f)",
			action->m_vecDetonatePos.x, action->m_vecDetonatePos.y, action->m_vecDetonatePos.z);
		NDebugOverlay::EntityTextAtPosition(action->m_vecDetonatePos, line++, buf, dt, 255, 255, 255, 255);
		
		NDebugOverlay::Cross3D(action->m_vecDetonatePos,
			det_cross_size, det_cross_r, det_cross_g, det_cross_b, true, dt);
	}
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMissionSuicideBomber_OnStart, CTFBot *actor, Action<CTFBot> *action)
	{
		auto me = reinterpret_cast<CTFBotMissionSuicideBomber *>(this);
		
		DevMsg("\n[OnStart] <== %s\n", (action != nullptr ? action->GetName() : "NULL"));
		auto result = DETOUR_MEMBER_CALL(actor, action);
		
		if (me != nullptr && actor != nullptr) {
			DrawOverlays(me, actor);
		}
		
		DevMsg("  %s\n", ActionResult_ToString(result));
		return result;
	}
	
	RefCount rc_CTFBotMissionSuicideBomber_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMissionSuicideBomber_Update, CTFBot *actor, float dt)
	{
		auto me = reinterpret_cast<CTFBotMissionSuicideBomber *>(this);
		
		SCOPED_INCREMENT(rc_CTFBotMissionSuicideBomber_Update);
		
		DevMsg("\n[Update]\n");
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		
		if (me != nullptr && actor != nullptr) {
			DrawOverlays(me, actor);
		}
		
		DevMsg("  %s\n", ActionResult_ToString(result));
		return result;
	}
	
	/* evil COMDAT folding */
//	DETOUR_DECL_MEMBER(void, CTFBotMissionSuicideBomber_OnEnd, CTFBot *actor, Action<CTFBot> *action)
//	{
//		DevMsg("\n[OnEnd] ==> %s\n", (action != nullptr ? action->GetName() : "NULL"));
//		DETOUR_MEMBER_CALL(actor, action);
//	}
	
	
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMissionSuicideBomber_OnStuck, CTFBot *actor)
	{
		DevMsg("\n[OnStuck]\n");
		auto result = DETOUR_MEMBER_CALL(actor);
		
		DevMsg("  result: %s\n", ActionResult_ToString(result));
		return result;
	}
	
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, CTFBotMissionSuicideBomber_OnKilled, CTFBot *actor, const CTakeDamageInfo& info)
	{
		DevMsg("\n[OnKilled]\n");
		auto result = DETOUR_MEMBER_CALL(actor, info);
		
		DevMsg("  result: %s\n", ActionResult_ToString(result));
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBotMissionSuicideBomber_StartDetonate, CTFBot *actor, bool reached_goal, bool lost_all_health)
	{
		DevMsg("  StartDetonate(reached_goal:%s lost_all_health:%s)\n",
			(reached_goal ? "YES" : "NO"),
			(lost_all_health ? "YES" : "NO"));
		DETOUR_MEMBER_CALL(actor, reached_goal, lost_all_health);
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotMissionSuicideBomber_Detonate, CTFBot *actor)
	{
		DevMsg("  Detonate()\n");
		DETOUR_MEMBER_CALL(actor);
	}
	
	
	DETOUR_DECL_MEMBER(bool, Path_Compute_vec_CTFBotPathCost, INextBot *nextbot, const Vector& vec, CTFBotPathCost& cost_func, float maxPathLength, bool b1)
	{
		auto result = DETOUR_MEMBER_CALL(nextbot, vec, cost_func, maxPathLength, b1);
		
		if (rc_CTFBotMissionSuicideBomber_Update > 0) {
			DevMsg("  Path::Compute: %s\n", (result ? "SUCCESS" : "FAIL"));
		}
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Suicide_Bomber")
		{
		//	MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_OnStart,       "CTFBotMissionSuicideBomber::OnStart");
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_Update,        "CTFBotMissionSuicideBomber::Update");
		//	MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_OnEnd,         "CTFBotMissionSuicideBomber::OnEnd");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_OnStuck,       "CTFBotMissionSuicideBomber::OnStuck");
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_OnKilled,      "CTFBotMissionSuicideBomber::OnKilled");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_StartDetonate, "CTFBotMissionSuicideBomber::StartDetonate");
			MOD_ADD_DETOUR_MEMBER(CTFBotMissionSuicideBomber_Detonate,      "CTFBotMissionSuicideBomber::Detonate");
			
			MOD_ADD_DETOUR_MEMBER(Path_Compute_vec_CTFBotPathCost, "Path::Compute_vec<CTFBotPathCost>");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_suicide_bomber", "0", FCVAR_NOTIFY,
		"Debug: enable debug statements for CTFBotMissionSuicideBomber",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


// findings:
// - CTFBotMissionSuicideBomber probably needs a OnMoveToFailure handler
//   (probably increment consec path failures and det, extending current behavior)
// - may also want to add a check so that if we think we're 

// questions:
// - what are all of the possible reasons why OnMoveToFailure could occur?
// - why isn't the NextBot stuck code triggering even when the SB is standing still due to repeated FAIL_FELL_OFF?

// todo:
// - figure out all possible cases in which m_vecTargetPos could end up as 0,0,0
//   already known:
//   - sentry buster spawns while sentry exists but is in blueprint mode
//   theoretical:
//   - any case in which actor->m_hSBTarget is invalidated at CTFBotMissionSuicideBomber::OnStart
//   
