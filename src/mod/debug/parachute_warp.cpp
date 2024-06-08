#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "util/trace.h"
#include "util/backtrace.h"


namespace Mod::Debug::Parachute_Warp
{
	// tracing results:
	// - it's happening somewhere inside of CTFBot::CTFBotIntention::Update
	
	
	const char *PrintAbsOrigin(CTFBot *bot)
	{
		static char buf[1024];
		
		if (bot != nullptr) {
			const Vector& origin = bot->GetAbsOrigin();
			
			snprintf(buf, sizeof(buf), "[ %+7.1f %+7.1f %+7.1f ]",
				origin.x, origin.y, origin.z);
		} else {
			snprintf(buf, sizeof(buf), "nullptr!");
		}
		
		return buf;
	}
	
	
	RefCount rc_INextBot_Update;
	DETOUR_DECL_MEMBER(void, INextBot_Update)
	{
		auto bot = rtti_cast<CTFBot *>(reinterpret_cast<INextBot *>(this));
		
		TRACE("%s", PrintAbsOrigin(bot));
		BACKTRACE();
		
		SCOPED_INCREMENT(rc_INextBot_Update);
		DETOUR_MEMBER_CALL();
		
		TRACE_EXIT("%s", PrintAbsOrigin(bot));
	}
	
	DETOUR_DECL_MEMBER(void, PlayerLocomotion_Update)
	{
		auto loco = reinterpret_cast<ILocomotion *>(this);
		auto bot = rtti_cast<CTFBot *>(loco->GetBot());
		
		TRACE("%s", PrintAbsOrigin(bot));
		DETOUR_MEMBER_CALL();
		TRACE_EXIT("%s", PrintAbsOrigin(bot));
	}
	
	DETOUR_DECL_MEMBER(void, IBody_Update)
	{
		auto body = reinterpret_cast<IBody *>(this);
		auto bot = rtti_cast<CTFBot *>(body->GetBot());
		
		TRACE("%s", PrintAbsOrigin(bot));
		DETOUR_MEMBER_CALL();
		TRACE_EXIT("%s", PrintAbsOrigin(bot));
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_CTFBotIntention_Update)
	{
		auto intent = reinterpret_cast<IIntention *>(this);
		auto bot = rtti_cast<CTFBot *>(intent->GetBot());
		
		TRACE("%s", PrintAbsOrigin(bot));
		DETOUR_MEMBER_CALL();
		TRACE_EXIT("%s", PrintAbsOrigin(bot));
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotVision_Update)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		auto bot = rtti_cast<CTFBot *>(vision->GetBot());
		
		TRACE("%s", PrintAbsOrigin(bot));
		DETOUR_MEMBER_CALL();
		TRACE_EXIT("%s", PrintAbsOrigin(bot));
	}
	
	
	RefCount rc_CTFBotMainAction_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_CTFBotMainAction_Update);
		
		TRACE("%s", PrintAbsOrigin(actor));
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		TRACE_EXIT("%s", PrintAbsOrigin(actor));
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotTacticalMonitor_Update, CTFBot *actor, float dt)
	{
		TRACE("%s", PrintAbsOrigin(actor));
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		TRACE_EXIT("%s", PrintAbsOrigin(actor));
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotScenarioMonitor_Update, CTFBot *actor, float dt)
	{
		TRACE("%s", PrintAbsOrigin(actor));
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		TRACE_EXIT("%s", PrintAbsOrigin(actor));
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotFetchFlag_Update, CTFBot *actor, float dt)
	{
		TRACE("%s", PrintAbsOrigin(actor));
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		TRACE_EXIT("%s", PrintAbsOrigin(actor));
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotAttackFlagDefenders_Update, CTFBot *actor, float dt)
	{
		TRACE("%s", PrintAbsOrigin(actor));
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		TRACE_EXIT("%s", PrintAbsOrigin(actor));
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotDeliverFlag_Update, CTFBot *actor, float dt)
	{
		TRACE("%s", PrintAbsOrigin(actor));
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		TRACE_EXIT("%s", PrintAbsOrigin(actor));
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_SetAbsOrigin, const Vector& absOrigin)
	{
		if (rc_CTFBotMainAction_Update > 0) {
			DevMsg("CBaseEntity::SetAbsOrigin [ %+7.1f %+7.1f %+7.1f ]\n",
				absOrigin.x, absOrigin.y, absOrigin.z);
			BACKTRACE();
		}
		
		DETOUR_MEMBER_CALL(absOrigin);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Parachute_Warp")
		{
		//	MOD_ADD_DETOUR_MEMBER(INextBot_Update,               "INextBot::Update");
		//	MOD_ADD_DETOUR_MEMBER(PlayerLocomotion_Update,       "PlayerLocomotion::Update");
		//	MOD_ADD_DETOUR_MEMBER(IBody_Update,                  "IBody::Update");
		//	MOD_ADD_DETOUR_MEMBER(CTFBot_CTFBotIntention_Update, "CTFBot::CTFBotIntention::Update");
		//	MOD_ADD_DETOUR_MEMBER(CTFBotVision_Update,           "CTFBotVision::Update");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update,          "CTFBotMainAction::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotTacticalMonitor_Update,     "CTFBotTacticalMonitor::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotScenarioMonitor_Update,     "CTFBotScenarioMonitor::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotFetchFlag_Update,           "CTFBotFetchFlag::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotAttackFlagDefenders_Update, "CTFBotAttackFlagDefenders::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotDeliverFlag_Update,         "CTFBotDeliverFlag::Update");
			
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_SetAbsOrigin, "CBaseEntity::SetAbsOrigin");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_parachute_warp", "0", FCVAR_NOTIFY,
		"Debug: parachute warp",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
