#include "mod.h"
#include "stub/objects.h"


namespace Mod::Visualize::Dispenser_Radius
{
	// for ammo:
	// - CObjectDispenser::DispenseThink
	//   - UTIL_EntitiesInSphere
	
	// for health:
	// box, 128x128x128 HU, CDispenserTouchTrigger "dispenser_touch_trigger"
	
	
	ConVar cvar_duration("sig_visualize_dispenser_radius_duration", "5.00", FCVAR_NOTIFY,
		"Visualization: duration");
	ConVar cvar_clear("sig_visualize_dispenser_radius_clear", "1", FCVAR_NOTIFY,
		"Visualization: clear overlays each time?");
	
	
	DETOUR_DECL_MEMBER(void, CObjectDispenser_DispenseThink)
	{
		DETOUR_MEMBER_CALL();
		
		static int i = 0;
		if (i++ % 10 != 0) {
			return;
		}
		
		auto dispenser = reinterpret_cast<CObjectDispenser *>(this);
		float radius = dispenser->GetDispenserRadius();
		
		if (cvar_clear.GetBool()) {
			NDebugOverlay::Clear();
		}
		
		for (int i = 0; i <= 1; ++i) {
			QAngle angles = {
				0.0f,
				(float)i * 45.0f,
				0.0f,
			};
			NDebugOverlay::Sphere(dispenser->GetAbsOrigin() + Vector(0.0f, 0.0f, 32.0f), angles, radius, 0x00, 0xff, 0x00, 0x80, false, cvar_duration.GetFloat());
		}
		
		NDebugOverlay::Box(dispenser->GetAbsOrigin(), Vector(-radius, -radius, -radius), Vector(radius, radius, radius), 0xff, 0x00, 0x00, 0x80, cvar_duration.GetFloat());
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Dispenser_Radius")
		{
			MOD_ADD_DETOUR_MEMBER(CObjectDispenser_DispenseThink, "CObjectDispenser::DispenseThink");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_dispenser_radius", "0", FCVAR_NOTIFY,
		"Visualization: dispenser radius",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
