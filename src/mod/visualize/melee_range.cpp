#include "mod.h"
#include "stub/tfweaponbase.h"
#include "stub/tfplayer.h"


namespace Mod::Visualize::Melee_Range
{
	// when we do a melee attack, draw these things:
	// - hull box for all players
	// - dot at attacker's eye pos
	// - text: melee range
	// - text: melee bounds
	// - line along ray trace center
	// - series of boxes along hull trace
	//   (color the boxes based on if they are before the hit, at the hit, or after the hit)
	
	
	// how DoSwingTrace operates:
	// - does a line trace first to prioritize things you are directly aiming at
	//   (will return the first thing hit along the ray)
	// - if the line trace didn't hit: does a hull trace
	//   (will return the first thing hit by the hull extruded along the ray)
	
	// so the overall priority for hits is:
	//   direct hits (near)
	//   ...
	//   direct hits (far)
	//   forgiving hits (near)
	//   ...
	//   forgiving hits (far)
	
	
#if 0
	bool got_ray;
	Ray_t saved_ray;
	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
		if (fMask == MASK_SOLID && ray.)
		
		got_ray = true;
		memcpy(&saved_ray, &ray, sizeof(ray));
		
		DETOUR_MEMBER_CALL(ray, fMask, pTraceFilter, pTrace);
	}
#endif
	
	
	DETOUR_DECL_MEMBER(bool, CTFWeaponBaseMelee_DoSwingTraceInternal, CGameTrace *pTrace, bool cleave_attack, CUtlVector<CGameTrace> *v1)
	{
		auto result = DETOUR_MEMBER_CALL(pTrace, cleave_attack, v1);
		
		auto melee = reinterpret_cast<CTFWeaponBaseMelee *>(this);
		CTFPlayer *owner = ToTFPlayer(melee->GetOwner());
		
		if (!cleave_attack && owner != nullptr) {
			float scale = std::min(1.0f, owner->GetModelScale());
			
			float mult_range  = CAttributeManager::AttribHookValue<float>(1.0f, "melee_range_multiplier",  melee);
			float mult_bounds = CAttributeManager::AttribHookValue<float>(1.0f, "melee_bounds_multiplier", melee);
			
			float range = melee->GetSwingRange() * scale * mult_range;
			Vector mins = Vector(-18.0f, -18.0f, -18.0f) * scale * mult_range;
			Vector maxs = -mins;
			
			Vector fwd;
			AngleVectors(owner->EyeAngles(), &fwd);
			
			Vector begin = owner->Weapon_ShootPosition();
			Vector end   = begin + (fwd * range);
			
			if (pTrace->fraction < 1.0f) {
				NDebugOverlay::Line(begin, pTrace->endpos, 0x00, 0xff, 0x00, true, 10.0f);
				NDebugOverlay::Line(pTrace->endpos, end,   0xff, 0x00, 0x00, true, 10.0f);
			} else {
				NDebugOverlay::Line(begin, end, 0xff, 0x00, 0x00, true, 10.0f);
			}
			
			
			// ...
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Melee_Range")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_DoSwingTraceInternal, "CTFWeaponBaseMelee::DoSwingTraceInternal");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_melee_range", "0", FCVAR_NOTIFY,
		"Visualization: melee range and bounds",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
