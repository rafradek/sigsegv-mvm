#include "mod.h"
#include "stub/tfbot.h"


namespace Mod::Debug::AirblastVuln
{
	ConVar cvar_trace("sig_debug_airblastvuln_trace", "0", FCVAR_NOTIFY,
		"Debug: enable debug trace statements");
	
	
	ConVar cvar_horz("sig_debug_airblastvuln_horz", "1.0", FCVAR_NOTIFY,
		"Debug: for MiniBoss TFBots, use this value for 'airblast_vulnerability_multiplier'");
	ConVar cvar_vert("sig_debug_airblastvuln_vert", "1.0", FCVAR_NOTIFY,
		"Debug: for MiniBoss TFBots, use this value for 'airblast_vertical_vulnerability_multiplier'");
	
	
	DETOUR_DECL_STATIC(float, CAttributeManager_AttribHookValue_float, float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
		auto result = DETOUR_STATIC_CALL(value, attr, ent, vec, b1);
		
		CTFBot *bot = ToTFBot(const_cast<CBaseEntity *>(ent));
		if (bot != nullptr && bot->IsMiniBoss()) {
			if (strcmp(attr, "airblast_vulnerability_multiplier") == 0) {
				float over = value * cvar_horz.GetFloat();
				
				if (cvar_trace.GetBool()) DevMsg("AirblastVuln: horz: was %.2f, overrode with %.2f\n", result, over);
				return over;
			}
			if (strcmp(attr, "airblast_vertical_vulnerability_multiplier") == 0) {
				float over = value * cvar_vert.GetFloat();
				
				if (cvar_trace.GetBool()) DevMsg("AirblastVuln: vert: was %.2f, overrode with %.2f\n", result, over);
				return over;
			}
		}
		
		return result;
	}
	
	
	ConVar cvar_show("sig_debug_airblastvuln_show", "0", FCVAR_NOTIFY,
		"Debug: enable visualizations");
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyGenericPushbackImpulse, const Vector& impulse, CTFPlayer *inflictor)
	{
#if 0
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		float attr1 = CAttributeManager::AttribHookValue<float>(1.0f, "airblast_vulnerability_multiplier", player);
		float attr2 = CAttributeManager::AttribHookValue<float>(1.0f, "airblast_vertical_vulnerability_multiplier", player);
		
		Vector dp = impulse;
		
		// do the z-minning BEFORE applying the regular vulnerability multiplier
		if ((player->GetFlags() & FL_ONGROUND) != 0) {
			dp.z = Min(dp.z, 268.32816f);
		}
		
		dp   *= attr1;
		dp.z *= attr2;
		
		player->m_fFlags &= ~FL_ONGROUND;
		
		player->m_Shared->AddCond(TF_COND_KNOCKED_INTO_AIR);
		
		player->ApplyAbsVelocityImpulse(dp);
#endif
		
		
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		Vector vBefore;
		player->GetVelocity(&vBefore, nullptr);
		
		DETOUR_MEMBER_CALL(impulse, inflictor);
		
		Vector vAfter;
		player->GetVelocity(&vAfter, nullptr);
		
		if (cvar_show.GetBool()) {
			Vector vDiff = vAfter - vBefore;
			
			float diff   = vDiff.Length();
			float diff_x = abs(vDiff.x);
			float diff_y = abs(vDiff.y);
			float diff_z = abs(vDiff.z);
			
			NDebugOverlay::EntityText(ENTINDEX(player), -1, CFmtStrN<256>("dV:   %6.1f", diff),   1.0f, 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(player),  1, CFmtStrN<256>("dV.x: %6.1f", diff_x), 1.0f, 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(player),  2, CFmtStrN<256>("dV.y: %6.1f", diff_y), 1.0f, 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(player),  3, CFmtStrN<256>("dV.z: %6.1f", diff_z), 1.0f, 0xff, 0xff, 0xff, 0xff);
		}
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:AirblastVuln")
		{
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_float, "CAttributeManager::AttribHookValue<float>");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyGenericPushbackImpulse, "CTFPlayer::ApplyGenericPushbackImpulse");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_airblastvuln", "0", FCVAR_NOTIFY,
		"Debug: enable easy testing of airblast vulnerability attributes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
