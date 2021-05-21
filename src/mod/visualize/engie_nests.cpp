#include "mod.h"
#include "stub/entities.h"
#include "re/nextbot.h"
#include "util/scope.h"
#include "util/iterate.h"


namespace Mod::Visualize::Engie_Nests
{
	ConVar cvar_interval("sig_visualize_engie_nests_interval", "66", FCVAR_NOTIFY,
		"Visualization: frame interval");
	
	
	void DrawNests()
	{
		float duration = cvar_interval.GetInt() * gpGlobals->interval_per_tick;
		constexpr float radius   = 25.0f;
		constexpr float width    =  5.0f;
		constexpr float gap      =  1.0f;
		
		std::map<std::string, std::vector<CTFBotHintEngineerNest *>> hints_nest;
		ForEachEntityByRTTI<CTFBotHintEngineerNest>([&](CTFBotHintEngineerNest *hint){
			hints_nest[STRING(hint->GetEntityName())].push_back(hint);
		});
		
		std::map<std::string, std::vector<CTFBotHintSentrygun *>> hints_sentry;
		ForEachEntityByRTTI<CTFBotHintSentrygun>([&](CTFBotHintSentrygun *hint){
			hints_sentry[STRING(hint->GetEntityName())].push_back(hint);
		});
		
		std::map<std::string, std::vector<CTFBotHintTeleporterExit *>> hints_tele;
		ForEachEntityByRTTI<CTFBotHintTeleporterExit>([&](CTFBotHintTeleporterExit *hint){
			hints_tele[STRING(hint->GetEntityName())].push_back(hint);
		});
		
		for (const auto& pair : hints_nest) {
			const auto& name = pair.first;
			const auto& hints = pair.second;
			
			for (const auto& hint : hints) {
				NDebugOverlay::Sphere(hint->GetAbsOrigin(), hint->GetAbsAngles(), radius, 0xff, 0xff, 0x00, 0x80, false, duration);
				NDebugOverlay::EntityTextAtPosition(hint->GetAbsOrigin(), -1, "Nest:",      duration, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityTextAtPosition(hint->GetAbsOrigin(),  0, name.c_str(), duration, 0xff, 0xff, 0xff, 0xff);
				
				for (const auto& hint2 : hints_sentry[name]) {
					Vector from = hint ->GetAbsOrigin();
					Vector to   = hint2->GetAbsOrigin();
					
					Vector dir = (to - from).Normalized();
					
				//	from += dir * (gap + radius);
					to   -= dir * (gap + radius);
					
					NDebugOverlay::HorzArrow(from, to, width, 0xff, 0x00, 0x00, 0xff, true, duration);
				}
				
				for (const auto& hint2 : hints_tele[name]) {
					Vector from = hint ->GetAbsOrigin();
					Vector to   = hint2->GetAbsOrigin();
					
					Vector dir = (to - from).Normalized();
					
				//	from += dir * (gap + radius);
					to   -= dir * (gap + radius);
					
					NDebugOverlay::HorzArrow(from, to, width, 0x00, 0xff, 0x00, 0xff, true, duration);
				}
			}
		}
		
		for (const auto& pair : hints_sentry) {
			const auto& name = pair.first;
			const auto& hints = pair.second;
			
			for (const auto& hint : hints) {
				NDebugOverlay::Sphere(hint->GetAbsOrigin(), hint->GetAbsAngles(), radius, 0xff, 0x00, 0x00, 0x80, false, duration);
				NDebugOverlay::EntityTextAtPosition(hint->GetAbsOrigin(), -1, "Sentry:",      duration, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityTextAtPosition(hint->GetAbsOrigin(),  0, name.c_str(),   duration, 0xff, 0xff, 0xff, 0xff);
			}
		}
		
		for (const auto& pair : hints_tele) {
			const auto& name = pair.first;
			const auto& hints = pair.second;
			
			for (const auto& hint : hints) {
				NDebugOverlay::Sphere(hint->GetAbsOrigin(), hint->GetAbsAngles(), radius, 0x00, 0xff, 0x00, 0x80, false, duration);
				NDebugOverlay::EntityTextAtPosition(hint->GetAbsOrigin(), -1, "Tele:",      duration, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityTextAtPosition(hint->GetAbsOrigin(),  0, name.c_str(), duration, 0xff, 0xff, 0xff, 0xff);
			}
		}
	}
	
	
	RefCount rc_TeleSpawn_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMvMEngineerTeleportSpawn_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_TeleSpawn_Update);
		return DETOUR_MEMBER_CALL(CTFBotMvMEngineerTeleportSpawn_Update)(actor, dt);
	}
	
	RefCount rc_BuildSentry_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMvMEngineerBuildSentryGun_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_BuildSentry_Update);
		return DETOUR_MEMBER_CALL(CTFBotMvMEngineerBuildSentryGun_Update)(actor, dt);
	}
	
	RefCount rc_BuildTele_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMvMEngineerBuildTeleportExit_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_BuildTele_Update);
		return DETOUR_MEMBER_CALL(CTFBotMvMEngineerBuildTeleportExit_Update)(actor, dt);
	}
	
	
	RefCount rc_PushAllPlayersAway;
	DETOUR_DECL_MEMBER(void, CTFGameRules_PushAllPlayersAway, const Vector& origin, float radius, float force, int teamnum, CUtlVector<CTFPlayer *> *pushed_players)
	{
		SCOPED_INCREMENT(rc_PushAllPlayersAway);
		DETOUR_MEMBER_CALL(CTFGameRules_PushAllPlayersAway)(origin, radius, force, teamnum, pushed_players);
		
		constexpr float duration = 2.0f;
		
		bool draw = false;
		Color c(0xff, 0xff, 0xff, 0xff);
		
		if (rc_TeleSpawn_Update > 0) {
			draw = true;
			c = Color(0xff, 0xff, 0x00, 0xff);
		} else if (rc_BuildSentry_Update > 0) {
			draw = true;
			c = Color(0xff, 0x00, 0x00, 0xff);
		} else if (rc_BuildTele_Update > 0) {
			draw = true;
			c = Color(0x00, 0xff, 0x00, 0xff);
		}
		
		if (draw) {
			NDebugOverlay::Sphere(origin, vec3_angle, radius, c.r(), c.g(), c.b(), 0x40, true, duration);
			
			if (pushed_players != nullptr) {
				for (auto player : *pushed_players) {
					Vector eyepos = player->EyePosition();
					
					Vector2D dxy = (eyepos - origin).AsVector2D();
					dxy.NormalizeInPlace();
					
					Vector impulse = force * Vector(dxy.x, dxy.y, 1.0f);
					
					// might need a multiplier for the impulse vector here to make the arrow length look good
					NDebugOverlay::VertArrow(player->GetAbsOrigin(), player->GetAbsOrigin() + impulse,
						5.0f, c.r(), c.g(), c.b(), 0xff, true, duration);
				}
			}
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Engie_Nests")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerTeleportSpawn_Update,     "CTFBotMvMEngineerTeleportSpawn::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerBuildSentryGun_Update,    "CTFBotMvMEngineerBuildSentryGun::Update");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerBuildTeleportExit_Update, "CTFBotMvMEngineerBuildTeleportExit::Update");
			
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_PushAllPlayersAway, "CTFGameRules::PushAllPlayersAway");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % cvar_interval.GetInt() != 0) return;
			
			DrawNests();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_engie_nests", "0", FCVAR_NOTIFY,
		"Visualization: draw information about MvM engineer hints",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
