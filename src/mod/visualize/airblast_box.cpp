#include "mod.h"
#include "util/scope.h"


namespace Mod::Visualize::Airblast_Box
{
	ConVar cvar_clear("sig_visualize_airblast_box_clear", "1", FCVAR_NOTIFY,
		"Visualization: set 1 to clear overlays each time");
	ConVar cvar_duration("sig_visualize_airblast_box_duration", "0.1", FCVAR_NOTIFY,
		"Visualization: box draw duration");
	
	ConVar cvar_color_r("sig_visualize_airblast_box_color_r", "255", FCVAR_NOTIFY,
		"Visualization: box color (red)");
	ConVar cvar_color_g("sig_visualize_airblast_box_color_g", "255", FCVAR_NOTIFY,
		"Visualization: box color (green)");
	ConVar cvar_color_b("sig_visualize_airblast_box_color_b", "255", FCVAR_NOTIFY,
		"Visualization: box color (blue)");
	ConVar cvar_color_a("sig_visualize_airblast_box_color_a", "255", FCVAR_NOTIFY,
		"Visualization: box color (alpha)");
	
	
	RefCount rc_CTFWeaponBase_DeflectProjectiles;
	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_DeflectProjectiles)
	{
		SCOPED_INCREMENT(rc_CTFWeaponBase_DeflectProjectiles);
		return DETOUR_MEMBER_CALL();
	}
	
	/* UTIL_EntitiesInBox forwards call to partition->EnumerateElementsInBox */
	RefCount rc_EnumerateElementsInBox;
	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInBox, SpatialPartitionListMask_t listMask, const Vector& mins, const Vector& maxs, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		SCOPED_INCREMENT(rc_EnumerateElementsInBox);
		if (rc_CTFWeaponBase_DeflectProjectiles > 0 && rc_EnumerateElementsInBox <= 1) {
			if (cvar_clear.GetBool()) {
				NDebugOverlay::Clear();
			}
			
			NDebugOverlay::Box(vec3_origin, mins, maxs,
				cvar_color_r.GetInt(),
				cvar_color_g.GetInt(),
				cvar_color_b.GetInt(),
				cvar_color_a.GetInt(),
				cvar_duration.GetFloat());
		}
		
		return DETOUR_MEMBER_CALL(listMask, mins, maxs, coarseTest, pIterator);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Airblast_Box")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_DeflectProjectiles,         "CTFWeaponBase::DeflectProjectiles");
			MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInBox, "ISpatialPartition::EnumerateElementsInBox");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_airblast_box", "0", FCVAR_NOTIFY,
		"Visualization: draw box used for airblast deflection of projectiles",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
