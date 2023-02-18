#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfbot_behavior.h"
#include "stub/tfentities.h"


//class CTFBotTacticalMonitor : public Action<CTFBot> {};


// please remove PleaseRemoveThis as a friend in mvm-reversed NextBotBehavior.h line 398
class PleaseRemoveThis
{
public:
	static char *BuildDecoratedName(Action<CTFBot> *action, char *buf)
	{
		V_strncat(buf, action->GetName(), 256);
		
		if (action->m_ActionChild != nullptr) {
			V_strncat(buf, "( ", 256);
			BuildDecoratedName(action->m_ActionChild, buf);
			V_strncat(buf, " )", 256);
		}
		
		if (action->m_ActionWeSuspended != nullptr) {
			V_strncat(buf, "<<", 256);
			BuildDecoratedName(action->m_ActionWeSuspended, buf);
		}
		
		return buf;
	}
	
	static char *DebugString(Action<CTFBot> *action)
	{
		static char buf[256];
		buf[0] = '\0';
		
		Action<CTFBot> *parent = action;
		while (parent->m_ActionParent != nullptr) {
			parent = parent->m_ActionParent;
		}
		
		return BuildDecoratedName(parent, buf);
	}
};


namespace Mod::Debug::OnKilled_Before_MainAction_Start
{
	ConVar cvar_overlay_duration("sig_debug_onkilled_before_mainaction_start_overlay_duration", "0.030", FCVAR_NOTIFY,
		"");
	
	
	ConVar cvar_fix_onstart("sig_debug_onkilled_before_mainaction_start_fix_onstart", "0", FCVAR_NOTIFY,
		"Debug: check for non-alive-ness in CTFBotMainAction::OnStart");
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_OnStart, CTFBot *actor, Action<CTFBot> *action)
	{
		if (!actor->IsAlive()) {
			DevMsg("\n\n\n");
			DevMsg("=========================================================================\n");
			DevMsg("CTFBotMainAction::OnStart (#%d): caught a dead bot!\n", ENTINDEX(actor));
			int m_lifeState = actor->m_lifeState;
			DevMsg("m_lifeState = %d\n", m_lifeState);
			DevMsg("=========================================================================\n");
			DevMsg("\n\n\n");
			
			if (cvar_fix_onstart.GetBool()) {
				return ActionResult<CTFBot>::ChangeTo(CTFBotDead::New(), "CAUGHT DEAD BOT IN CTFBotMainAction::OnStart");
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFBotMainAction_OnStart)(actor, action);
	}
	
	ConVar cvar_fix_update("sig_debug_onkilled_before_mainaction_start_fix_update", "0", FCVAR_NOTIFY,
		"Debug: check for non-alive-ness in CTFBotMainAction::Update");
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		if (!actor->IsAlive()) {
			DevMsg("\n\n\n");
			DevMsg("=========================================================================\n");
			DevMsg("CTFBotMainAction::Update (#%d): caught a dead bot!\n", ENTINDEX(actor));
			int m_lifeState = actor->m_lifeState;
			DevMsg("m_lifeState = %d\n", m_lifeState);
			DevMsg("=========================================================================\n");
			DevMsg("\n\n\n");
			
			if (cvar_fix_update.GetBool()) {
				return ActionResult<CTFBot>::ChangeTo(CTFBotDead::New(), "CAUGHT DEAD BOT IN CTFBotMainAction::Update");
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
	}
	
	
#if 0
	ConVar cvar_override("sig_debug_onkilled_before_mainaction_start_override", "0", FCVAR_NOTIFY,
		"Debug: try overriding OnKilled in a child action of CTFBotMainAction to see what happens");
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, Action_CTFBot_OnKilled, CTFBot *actor, const CTakeDamageInfo& info)
	{
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		if (rtti_cast<CTFBotTacticalMonitor *>(action) != nullptr) {
			if (RandomFloat(0.0f, 1.0f) < 0.5f) {
				return EventDesiredResult<CTFBot>::SuspendFor(CTFBotSeekAndDestroy::New(), "Doing something stupid");
			} else {
				return EventDesiredResult<CTFBot>::Sustain();
			}
		}
		
		return DETOUR_MEMBER_CALL(Action_CTFBot_OnKilled)(actor, info);
	}
#endif
	
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotFetchFlag_OnStart, CTFBot *actor, Action<CTFBot> *action)
	{
		if (!actor->IsAlive()) {
			DevMsg("\n\n\n");
			DevMsg("=========================================================================\n");
			DevMsg("CTFBotFetchFlag::OnStart (#%d): caught a dead bot!\n", ENTINDEX(actor));
			int m_lifeState = actor->m_lifeState;
			DevMsg("m_lifeState = %d\n", m_lifeState);
			DevMsg("=========================================================================\n");
			DevMsg("\n\n\n");
		}
		
		return DETOUR_MEMBER_CALL(CTFBotFetchFlag_OnStart)(actor, action);
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotFetchFlag_Update, CTFBot *actor, float dt)
	{
		if (!actor->IsAlive()) {
			DevMsg("\n\n\n");
			DevMsg("=========================================================================\n");
			DevMsg("CTFBotFetchFlag::Update (#%d): caught a dead bot!\n", ENTINDEX(actor));
			int m_lifeState = actor->m_lifeState;
			DevMsg("m_lifeState = %d\n", m_lifeState);
			DevMsg("=========================================================================\n");
			DevMsg("\n\n\n");
		}
		
		return DETOUR_MEMBER_CALL(CTFBotFetchFlag_Update)(actor, dt);
	}
	
	
	DETOUR_DECL_MEMBER(void, INextBot_DisplayDebugText, const char *text)
	{
		/* suppress on-bot overlay text */
	}
	
	
	// TODO:
	// do a constant text overlay with rows for each bot slots and columns for:
	// - teamnum
	// - isalive
	// - currently active NB action(s)
	// - has flag?
	// - where they are
	
	// maybe also an overlay for each flag
	// - is picked up?
	// - owner entity
	// - where it is
	
	
	void DrawOverlay_Bots()
	{
		float duration = cvar_overlay_duration.GetFloat();
		
		constexpr float base_x = 0.02f;
		constexpr float base_y = 0.02f;
		
		constexpr float col1_w = 0.04f; constexpr float col1_x = 0.00f;
		constexpr float col2_w = 0.04f; constexpr float col2_x = col1_x + col1_w;
		constexpr float col3_w = 0.04f; constexpr float col3_x = col2_x + col2_w;
		constexpr float col4_w = 0.10f; constexpr float col4_x = col3_x + col3_w;
		constexpr float col5_w = 0.12f; constexpr float col5_x = col4_x + col4_w;
		constexpr float col6_w = 0.00f; constexpr float col6_x = col5_x + col5_w;
		
		NDebugOverlay::ScreenText(base_x + col1_x, base_y, "ENTINDEX",  0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col2_x, base_y, "TEAMNUM",   0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col3_x, base_y, "ALIVE",     0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col4_x, base_y, "ABSORIGIN", 0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col5_x, base_y, "BOTNAME",   0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col6_x, base_y, "BEHAVIOR",  0xff, 0xff, 0xff, 0xff, duration);
		
		float line_y = 0.0f;
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(i));
			if (bot == nullptr) continue;
			
			line_y += 0.0140f;
			
			bool is_alive = bot->IsAlive();
			int teamnum = bot->GetTeamNumber();
			
			NDebugOverlay::ScreenText(base_x + col1_x, base_y + line_y, CFmtStrN<64>("#%d", i), 0xff, 0xff, 0xff, 0xff, duration);
			
			if (teamnum == TF_TEAM_BLUE) {
				NDebugOverlay::ScreenText(base_x + col2_x, base_y + line_y, "BLU", 0x40, 0x40, 0xff, 0xff, duration);
			} else if (teamnum == TEAM_SPECTATOR) {
				NDebugOverlay::ScreenText(base_x + col2_x, base_y + line_y, "SPEC", 0x80, 0x80, 0x80, 0xff, duration);
			} else {
				NDebugOverlay::ScreenText(base_x + col2_x, base_y + line_y, CFmtStrN<64>("%d", teamnum), 0xff, 0xff, 0xff, 0xff, duration);
			}
			
			if (is_alive) {
				NDebugOverlay::ScreenText(base_x + col3_x, base_y + line_y, "ALIVE", 0x40, 0xff, 0x40, 0xff, duration);
			} else {
				NDebugOverlay::ScreenText(base_x + col3_x, base_y + line_y, "DEAD", 0xff, 0x40, 0x40, 0xff, duration);
			}
			
			const Vector& absorigin = bot->GetAbsOrigin();
			NDebugOverlay::ScreenText(base_x + col4_x, base_y + line_y, CFmtStrN<256>("[ %+5.0f %+5.0f %+5.0f ]", absorigin.x, absorigin.y, absorigin.z), 0xff, 0xff, 0xff, 0xff, duration);
			
			NDebugOverlay::ScreenText(base_x + col5_x, base_y + line_y, bot->GetPlayerName(), 0xff, 0xff, 0xff, 0xff, duration);
			
			IIntention *intent = bot->GetIntentionInterface();
			auto behavior = rtti_cast<Behavior<CTFBot> *>(intent->FirstContainedResponder());
			if (behavior != nullptr) {
				auto action = rtti_cast<Action<CTFBot> *>(behavior->FirstContainedResponder());
				if (action != nullptr) {
					NDebugOverlay::ScreenText(base_x + col6_x, base_y + line_y, PleaseRemoveThis::DebugString(action), 0xff, 0xff, 0xff, 0xff, duration);
				} else {
					NDebugOverlay::ScreenText(base_x + col6_x, base_y + line_y, "(action is nullptr)", 0x80, 0x80, 0x80, 0xff, duration);
				}
			} else {
				NDebugOverlay::ScreenText(base_x + col6_x, base_y + line_y, "(behavior is nullptr)", 0x80, 0x80, 0x80, 0xff, duration);
			}
		}
	}
	
	void DrawOverlay_Flags()
	{
		float duration = cvar_overlay_duration.GetFloat();
		
		constexpr float base_x = 0.02f;
		constexpr float base_y = 0.36f;
		
		constexpr float col1_w = 0.04f; constexpr float col1_x = 0.00f;
		constexpr float col2_w = 0.04f; constexpr float col2_x = col1_x + col1_w;
		constexpr float col3_w = 0.04f; constexpr float col3_x = col2_x + col2_w;
		constexpr float col4_w = 0.00f; constexpr float col4_x = col3_x + col3_w;
		
		NDebugOverlay::ScreenText(base_x + col1_x, base_y, "FLAGIDX",   0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col2_x, base_y, "NAME",      0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col3_x, base_y, "OWNERIDX",  0xff, 0xff, 0xff, 0xff, duration);
		NDebugOverlay::ScreenText(base_x + col4_x, base_y, "OWNERNAME", 0xff, 0xff, 0xff, 0xff, duration);
		
		float line_y = 0.0f;
		for (int i = 0; i < ICaptureFlagAutoList::AutoList().Count(); ++i) {
			auto flag = rtti_cast<CCaptureFlag *>(ICaptureFlagAutoList::AutoList()[i]);
			if (flag == nullptr) continue;
			
			line_y += 0.0140f;
			
			NDebugOverlay::ScreenText(base_x + col1_x, base_y + line_y, CFmtStrN<64>("#%d", ENTINDEX(flag)), 0xff, 0xff, 0xff, 0xff, duration);
			
			NDebugOverlay::ScreenText(base_x + col2_x, base_y + line_y, STRING(flag->GetEntityName()), 0xff, 0xff, 0xff, 0xff, duration);
			
			CBaseEntity *owner = flag->GetOwnerEntity();
			if (owner == nullptr) {
				NDebugOverlay::ScreenText(base_x + col3_x, base_y + line_y, "none", 0x80, 0x80, 0x80, 0xff, duration);
			} else {
				NDebugOverlay::ScreenText(base_x + col3_x, base_y + line_y, CFmtStrN<256>("#%d", ENTINDEX(owner)), 0xff, 0xff, 0xff, 0xff, duration);
			}
			
			CBasePlayer *player_owner = ToBasePlayer(owner);
			if (player_owner != nullptr) {
				NDebugOverlay::ScreenText(base_x + col4_x, base_y + line_y, player_owner->GetPlayerName(), 0xff, 0xff, 0xff, 0xff, duration);
			}
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:OnKilled_Before_MainAction_Start")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_OnStart, "CTFBotMainAction::OnStart");
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update,  "CTFBotMainAction::Update");
			
//			MOD_ADD_DETOUR_MEMBER(Action_CTFBot_OnKilled,   "Action<CTFBot>::OnKilled");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotFetchFlag_OnStart, "CTFBotFetchFlag::OnStart");
			MOD_ADD_DETOUR_MEMBER(CTFBotFetchFlag_Update,  "CTFBotFetchFlag::Update");
			
			MOD_ADD_DETOUR_MEMBER(INextBot_DisplayDebugText, "INextBot::DisplayDebugText");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (frame++ % 2 == 0) {
				NDebugOverlay::Clear();
				
				DrawOverlay_Bots();
				DrawOverlay_Flags();
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_onkilled_before_mainaction_start", "0", FCVAR_NOTIFY,
		"Debug: investigate post-20160707a edge cases of bots not switching to spectator",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
