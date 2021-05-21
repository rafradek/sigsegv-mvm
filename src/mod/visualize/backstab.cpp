#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "util/iterate.h"


namespace Mod::Visualize::Backstab
{
	ConVar cvar_dotproduct("sig_visualize_backstab_dotproduct", "0", FCVAR_NOTIFY,
		"Visualization: show dot products instead of lines and stuff");
	ConVar cvar_dotproduct_vectors("sig_visualize_backstab_dotproduct_vectors", "0", FCVAR_NOTIFY,
		"Visualization: also draw the relevant vectors");
	
	
	void DrawDotProductFigures(CTFPlayer *spy, CTFPlayer *victim)
	{
		Vector2D wsc_spy_to_victim = (victim->WorldSpaceCenter() - spy->WorldSpaceCenter()).AsVector2D();
		wsc_spy_to_victim.NormalizeInPlace();
		
		Vector temp1; spy->EyeVectors(&temp1);
		Vector2D eye_spy = temp1.AsVector2D();
		eye_spy.NormalizeInPlace();
		
		Vector temp2; victim->EyeVectors(&temp2);
		Vector2D eye_victim = temp2.AsVector2D();
		eye_victim.NormalizeInPlace();
		
		
		float dot0 = DotProduct2D(wsc_spy_to_victim, eye_victim);
		float dot1 = DotProduct2D(wsc_spy_to_victim, eye_spy);
		float dot2 = DotProduct2D(eye_spy,           eye_victim);
		
		float ang0 = RAD2DEG(acos(dot0));
		float ang1 = RAD2DEG(acos(dot1));
		float ang2 = RAD2DEG(acos(dot2));
		
		bool ok0 = (dot0 >  0.0f);
		bool ok1 = (dot1 >  0.5f);
		bool ok2 = (dot2 > -0.3f);
		
		int r0 = (ok0 ? 0x20 : 0xff); int g0 = (ok0 ? 0xff : 0x20);
		int r1 = (ok1 ? 0x20 : 0xff); int g1 = (ok1 ? 0xff : 0x20);
		int r2 = (ok2 ? 0x20 : 0xff); int g2 = (ok2 ? 0xff : 0x20);
		
		
#if 1
		float yt = 0.06f;
		float y0 = 0.10f;
		float y1 = 0.14f;
		float y2 = 0.18f;
#else
		float yt = 0.78f;
		float y0 = 0.82f;
		float y1 = 0.86f;
		float y2 = 0.90f;
#endif
		
		
		// Column 1
		NDebugOverlay::ScreenText(0.03f, y0, "VictimView-to-DeltaPosition",
			0xc0, 0xc0, 0xc0, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.03f, y1, "   SpyView-to-DeltaPosition",
			0xc0, 0xc0, 0xc0, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.03f, y2, "      SpyView-to-VictimView",
			0xc0, 0xc0, 0xc0, 0xff, 0.032f);
		
		// Column 2
		NDebugOverlay::ScreenText(0.28f, yt, "DotProduct",
			0xff, 0xff, 0xff, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.28f, y0, CFmtStr("     % 5.2f", dot0),
			r0, g0, 0x20, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.28f, y1, CFmtStr("     % 5.2f", dot1),
			r1, g1, 0x20, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.28f, y2, CFmtStr("     % 5.2f", dot2),
			r2, g2, 0x20, 0xff, 0.032f);
		
		// Column 3
		NDebugOverlay::ScreenText(0.40f, yt, " Angle",
			0xff, 0xff, 0xff, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.40f, y0, CFmtStr("%5.1f\u00b0", ang0),
			r0, g0, 0x20, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.40f, y1, CFmtStr("%5.1f\u00b0", ang1),
			r1, g1, 0x20, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.40f, y2, CFmtStr("%5.1f\u00b0", ang2),
			r2, g2, 0x20, 0xff, 0.032f);
		
		// Column 4
		NDebugOverlay::ScreenText(0.47f, y0, "Behind the victim?               Must be < 90\u00b0",
			0xc0, 0xc0, 0xc0, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.47f, y1, "Looking toward the victim?       Must be < 60\u00b0",
			0xc0, 0xc0, 0xc0, 0xff, 0.032f);
		NDebugOverlay::ScreenText(0.47f, y2, "Facing same direction as victim? Must be < ~107.5\u00b0",
			0xc0, 0xc0, 0xc0, 0xff, 0.032f);
		
		
		if (cvar_dotproduct_vectors.GetBool()) {
			NDebugOverlay::HorzArrow(spy->EyePosition(), spy->EyePosition() + (150.0f * Vector(Vector2DExpand(eye_spy), 0.0f)),
				3.0f, 0xff, 0xff, 0x00, 0xff, true, 0.032f);
			NDebugOverlay::HorzArrow(victim->EyePosition(), victim->EyePosition() + (150.0f * Vector(Vector2DExpand(eye_victim), 0.0f)),
				3.0f, 0xff, 0x00, 0xff, 0xff, true, 0.032f);
			
			Vector v1 = spy->WorldSpaceCenter(); v1.z = spy->EyePosition().z;
			Vector v2 = victim->WorldSpaceCenter(); v2.z = victim->EyePosition().z;
			
			NDebugOverlay::HorzArrow(v1, v2,
				3.0f, 0x00, 0xff, 0xff, 0xff, true, 0.032f);
		}
	}
	
	
	void DrawEyeXYLine(CTFPlayer *player, Color c, float length, float duration)
	{
		Vector vecEyePos = player->EyePosition();
		Vector vecEyeDir; player->EyeVectors(&vecEyeDir);
		
		vecEyeDir.z = 0.0f;
		
		NDebugOverlay::Line(vecEyePos, vecEyePos + (vecEyeDir * length), c[0], c[1], c[2], false, duration);
	}
	
	
	void DrawEyeVectors()
	{
		ForEachTFPlayer([](CTFPlayer *player){
			if (player->GetTeamNumber() < FIRST_GAME_TEAM) return;
			if (!player->IsAlive())                        return;
			
			if (cvar_dotproduct.GetBool()) return;
			
			DrawEyeXYLine(player, Color(0xff, 0xff, 0xff, 0xff), 2000.0f, 0.032f);
		});
	}
	
	void DrawPlayerHulls()
	{
		ForEachTFPlayer([](CTFPlayer *player){
			if (player->GetTeamNumber() < FIRST_GAME_TEAM) return;
			if (!player->IsAlive())                        return;
			
			if (cvar_dotproduct.GetBool() && !cvar_dotproduct_vectors.GetBool() && player->IsPlayerClass(TF_CLASS_SPY)) return;
			
			Vector vecOrigin = player->GetAbsOrigin();
			Vector vecMins   = player->CollisionProp()->OBBMins();
			Vector vecMaxs   = player->CollisionProp()->OBBMaxs();
			
			NDebugOverlay::Box(vecOrigin, vecMins, vecMaxs, 0xc0, 0xc0, 0xc0, 00, 0.032f);
		});
	}
	
	void DrawKnifeLines()
	{
		ForEachTFPlayer([](CTFPlayer *player){
			if (player->GetTeamNumber() < FIRST_GAME_TEAM) return;
			if (!player->IsAlive())                        return;
			
			auto knife = rtti_cast<CTFKnife *>(player->GetActiveTFWeapon());
			if (knife == nullptr) return;
			
			if (cvar_dotproduct.GetBool()) {
				ForEachTFPlayer([=](CTFPlayer *victim){
					if (victim->GetTeamNumber() < FIRST_GAME_TEAM) return;
					if (!victim->IsAlive())                        return;
					
					if (ENTINDEX(victim) == ENTINDEX(player)) return;
					
					DrawDotProductFigures(player, victim);
				});
			} else {
				trace_t tr;
				if (!knife->DoSwingTrace(tr)) return;
				
				ForEachTFPlayer([&](CTFPlayer *victim){
					if (victim->GetTeamNumber() < FIRST_GAME_TEAM) return;
					if (!victim->IsAlive())                        return;
					
					if (ENTINDEX(victim) == ENTINDEX(player)) return;
					
					if (tr.m_pEnt == nullptr || ENTINDEX(tr.m_pEnt) != ENTINDEX(victim)) return;
					
					bool bBehind = knife->IsBehindAndFacingTarget(victim);
					
					int r = (bBehind ? 0x00 : 0xff);
					int g = (bBehind ? 0xff : 0x00);
					
				//	Ray_t ray;
				//	ray.Init(tr.startpos, tr.startpos + (100.0f * (tr.endpos - tr.startpos).Normalized()));
				//	enginetrace->ClipRayToEntity(ray, CONTENTS_SOLID, tr.m_pEnt, &tr);
					
					NDebugOverlay::Line(tr.startpos, tr.endpos, r, g, 0x00, false, 3600.0f);
				});
			}
		});
	}
	
	
	// realization: there are two tests going on here (besides the melee trace itself)
	// 1. is the spy BEHIND the victim
	// 2. is the spy FACING the same direction as the victim
	// 
	// ...
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Backstab") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (frame++ % 2 != 0) return;
			
			DrawEyeVectors();
			DrawPlayerHulls();
			DrawKnifeLines();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_backstab", "0", FCVAR_NOTIFY,
		"Visualization: draw information about backstab angles",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


#if 0
// reverse engineered from ServerLinux 20151007a

bool CTFKnife::IsBehindAndFacingTarget(CTFPlayer *pVictim)
{
	CTFPlayer *pSpy = ToTFPlayer(this->GetPlayerOwner());
	if (pSpy == nullptr) return false;
	
	Vector2D wsc_spy_to_victim = (pVictim->WorldSpaceCenter() - pSpy->WorldSpaceCenter()).AsVector2D();
	wsc_spy_to_victim.NormalizeInPlace();
	
	Vector temp1; pSpy->EyeVectors(&temp1);
	Vector2D eye_spy = temp1.AsVector2D();
	eye_spy.NormalizeInPlace();
	
	Vector temp2; pVictim->EyeVectors(&temp2);
	Vector2D eye_victim = temp2.AsVector2D();
	eye_victim.NormalizeInPlace();
	
	if (DotProduct2D(wsc_spy_to_victim, eye_victim) <=  0.0f) return false;
	if (DotProduct2D(wsc_spy_to_victim, eye_spy)    <=  0.5f) return false;
	if (DotProduct2D(eye_spy,           eye_victim) <= -0.3f) return false;
	
	return true;
}
#endif
