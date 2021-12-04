#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"


#if 0 // obsolete as of Inferno Update

namespace Mod::Visualize::Airblast_Vectors
{
	ConVar cvar_eye_r("sig_visualize_airblast_vectors_eye_r", "0x00", FCVAR_NOTIFY, "Visualization: eye vector color (red)");
	ConVar cvar_eye_g("sig_visualize_airblast_vectors_eye_g", "0xff", FCVAR_NOTIFY, "Visualization: eye vector color (green)");
	ConVar cvar_eye_b("sig_visualize_airblast_vectors_eye_b", "0xff", FCVAR_NOTIFY, "Visualization: eye vector color (blue)");
	ConVar cvar_eye_a("sig_visualize_airblast_vectors_eye_a", "0xff", FCVAR_NOTIFY, "Visualization: eye vector color (alpha)");
	
	ConVar cvar_dwsc_r("sig_visualize_airblast_vectors_dwsc_r", "0xff", FCVAR_NOTIFY, "Visualization: delta-WSC vector color (red)");
	ConVar cvar_dwsc_g("sig_visualize_airblast_vectors_dwsc_g", "0x00", FCVAR_NOTIFY, "Visualization: delta-WSC vector color (green)");
	ConVar cvar_dwsc_b("sig_visualize_airblast_vectors_dwsc_b", "0xff", FCVAR_NOTIFY, "Visualization: delta-WSC vector color (blue)");
	ConVar cvar_dwsc_a("sig_visualize_airblast_vectors_dwsc_a", "0xff", FCVAR_NOTIFY, "Visualization: delta-WSC vector color (alpha)");
	
	ConVar cvar_imp_main_r("sig_visualize_airblast_vectors_imp_main_r", "0xff", FCVAR_NOTIFY, "Visualization: main impulse vector color (red)");
	ConVar cvar_imp_main_g("sig_visualize_airblast_vectors_imp_main_g", "0x00", FCVAR_NOTIFY, "Visualization: main impulse vector color (green)");
	ConVar cvar_imp_main_b("sig_visualize_airblast_vectors_imp_main_b", "0x00", FCVAR_NOTIFY, "Visualization: main impulse vector color (blue)");
	ConVar cvar_imp_main_a("sig_visualize_airblast_vectors_imp_main_a", "0xff", FCVAR_NOTIFY, "Visualization: main impulse vector color (alpha)");
	
	ConVar cvar_imp_vert_r("sig_visualize_airblast_vectors_imp_vert_r", "0x00", FCVAR_NOTIFY, "Visualization: vertical impulse vector color (red)");
	ConVar cvar_imp_vert_g("sig_visualize_airblast_vectors_imp_vert_g", "0xff", FCVAR_NOTIFY, "Visualization: vertical impulse vector color (green)");
	ConVar cvar_imp_vert_b("sig_visualize_airblast_vectors_imp_vert_b", "0x00", FCVAR_NOTIFY, "Visualization: vertical impulse vector color (blue)");
	ConVar cvar_imp_vert_a("sig_visualize_airblast_vectors_imp_vert_a", "0xff", FCVAR_NOTIFY, "Visualization: vertical impulse vector color (alpha)");
	
	ConVar cvar_imp_sum_r("sig_visualize_airblast_vectors_imp_sum_r", "0xff", FCVAR_NOTIFY, "Visualization: impulse vector sum color (red)");
	ConVar cvar_imp_sum_g("sig_visualize_airblast_vectors_imp_sum_g", "0xff", FCVAR_NOTIFY, "Visualization: impulse vector sum color (green)");
	ConVar cvar_imp_sum_b("sig_visualize_airblast_vectors_imp_sum_b", "0x00", FCVAR_NOTIFY, "Visualization: impulse vector sum color (blue)");
	ConVar cvar_imp_sum_a("sig_visualize_airblast_vectors_imp_sum_a", "0xff", FCVAR_NOTIFY, "Visualization: impulse vector sum color (alpha)");
	
	
#define CVAR_COLOR(name) \
	std::strtol(cvar_ ## name ## _r.GetString(), nullptr, 0), \
	std::strtol(cvar_ ## name ## _g.GetString(), nullptr, 0), \
	std::strtol(cvar_ ## name ## _b.GetString(), nullptr, 0), \
	std::strtol(cvar_ ## name ## _a.GetString(), nullptr, 0)
	
	
	float attr__airblast_pushback_scale          = -1.0f;
	float attr__airblast_vertical_pushback_scale = -1.0f;
	DETOUR_DECL_STATIC(float, CAttributeManager_AttribHookValue_float, float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
		auto result = DETOUR_STATIC_CALL(CAttributeManager_AttribHookValue_float)(value, attr, ent, vec, b1);
		
		if (strcmp(attr, "airblast_pushback_scale") == 0) {
			attr__airblast_pushback_scale = result;
		}
		if (strcmp(attr, "airblast_vertical_pushback_scale") == 0) {
			attr__airblast_vertical_pushback_scale = result;
		}
		
		return result;
	}
	
	DETOUR_DECL_MEMBER(bool, CTFFlameThrower_DeflectPlayer, CTFPlayer *pVictim, CTFPlayer *pPyro, const Vector& vecEyeFwd, const Vector& vecBoxCenter, const Vector& vecBoxSize)
	{
		NDebugOverlay::Clear();
		
		auto result = DETOUR_MEMBER_CALL(CTFFlameThrower_DeflectPlayer)(pVictim, pPyro, vecEyeFwd, vecBoxCenter, vecBoxSize);
		
		Vector vecDeltaWSC = (pVictim->WorldSpaceCenter() - pPyro->WorldSpaceCenter());
		
		NDebugOverlay::VertArrow(pPyro->WorldSpaceCenter(), pPyro->WorldSpaceCenter() + (100.0f * vecEyeFwd),
			3.0f, CVAR_COLOR(eye), true, 3600.0f);
		NDebugOverlay::EntityTextAtPosition(pPyro->WorldSpaceCenter() + (100.0f * vecEyeFwd), -1,
			"xhair", 3600.0f, CVAR_COLOR(eye));
		
		NDebugOverlay::VertArrow(pPyro->WorldSpaceCenter(), pPyro->WorldSpaceCenter() + (100.0f * vecDeltaWSC.Normalized()),
			3.0f, CVAR_COLOR(dwsc), true, 3600.0f);
		NDebugOverlay::EntityTextAtPosition(pPyro->WorldSpaceCenter() + (100.0f * vecDeltaWSC.Normalized()), -1,
			"delta-WSC", 3600.0f, CVAR_COLOR(dwsc));
		
		// TODO: draw sphere: WSC of pyro
		// TODO: draw sphere: WSC of victim
		
		if (result) {
			Vector vA = pVictim->WorldSpaceCenter();
			Vector vB = vA + ((attr__airblast_pushback_scale          * vecDeltaWSC.Normalized()) / 3.0f);
			Vector vC = vB + ((attr__airblast_vertical_pushback_scale * Vector(0.0f, 0.0f, 1.0f)) / 3.0f);
			
			NDebugOverlay::VertArrow(vA, vB, 3.0f, CVAR_COLOR(imp_main), true, 3600.0f);
			NDebugOverlay::EntityTextAtPosition((vA + vB) / 2.0f, 0, "Delta-WSC", 3600.0f, CVAR_COLOR(imp_main));
			NDebugOverlay::EntityTextAtPosition((vA + vB) / 2.0f, 1, "Impulse", 3600.0f, CVAR_COLOR(imp_main));
			
			NDebugOverlay::VertArrow(vB, vC, 3.0f, CVAR_COLOR(imp_vert), true, 3600.0f);
			NDebugOverlay::EntityTextAtPosition((vB + vC) / 2.0f, 0, "Vertical", 3600.0f, CVAR_COLOR(imp_vert));
			NDebugOverlay::EntityTextAtPosition((vB + vC) / 2.0f, 1, "Impulse", 3600.0f, CVAR_COLOR(imp_vert));
			
			NDebugOverlay::VertArrow(vA, vC, 3.0f, CVAR_COLOR(imp_sum), true, 3600.0f);
			NDebugOverlay::EntityTextAtPosition((vA + vC) / 2.0f, 0, "Vector Sum", 3600.0f, CVAR_COLOR(imp_sum));
		}
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyGenericPushbackImpulse, const Vector& impulse, CTFPlayer * inflictor)
	{
		DevMsg("ApplyGenericPushbackImpulse: %6.1f [ %+7.1f %+7.1f %+7.1f ]\n",
			impulse.Length(), impulse.x, impulse.y, impulse.z);
		DETOUR_MEMBER_CALL(CTFPlayer_ApplyGenericPushbackImpulse)(impulse, inflictor);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Airblast_Vectors")
		{
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_float, "CAttributeManager::AttribHookValue<float>");
			MOD_ADD_DETOUR_MEMBER(CTFFlameThrower_DeflectPlayer,           "CTFFlameThrower::DeflectPlayer");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyGenericPushbackImpulse, "CTFPlayer::ApplyGenericPushbackImpulse");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_airblast_vectors", "0", FCVAR_NOTIFY,
		"Visualization: draw vectors used for airblast deflection of players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif
