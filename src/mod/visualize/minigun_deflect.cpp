#include "mod.h"
#include "util/scope.h"


#ifdef EXPERIMENTAL

namespace Mod::Visualize::Minigun_Deflect
{
	ConVar cvar_duration("sig_visualize_minigun_deflect_duration", "0.1", FCVAR_NOTIFY,
		"Visualization: cylinder draw duration");
	
	ConVar cvar_color_r("sig_visualize_minigun_deflect_color_r", "255", FCVAR_NOTIFY,
		"Visualization: cylinder color (red)");
	ConVar cvar_color_g("sig_visualize_minigun_deflect_color_g", "255", FCVAR_NOTIFY,
		"Visualization: cylinder color (green)");
	ConVar cvar_color_b("sig_visualize_minigun_deflect_color_b", "255", FCVAR_NOTIFY,
		"Visualization: cylinder color (blue)");
	ConVar cvar_color_a("sig_visualize_minigun_deflect_color_a", "0", FCVAR_NOTIFY,
		"Visualization: cylinder color (alpha)");
	
	ConVar cvar_sphere_enable("sig_visualize_minigun_deflect_sphere_enable", "0", FCVAR_NOTIFY,
		"Visualization: sphere enable");
	ConVar cvar_sphere_color_r("sig_visualize_minigun_deflect_sphere_color_r", "255", FCVAR_NOTIFY,
		"Visualization: sphere color (red)");
	ConVar cvar_sphere_color_g("sig_visualize_minigun_deflect_sphere_color_g", "255", FCVAR_NOTIFY,
		"Visualization: sphere color (green)");
	ConVar cvar_sphere_color_b("sig_visualize_minigun_deflect_sphere_color_b", "255", FCVAR_NOTIFY,
		"Visualization: sphere color (blue)");
	ConVar cvar_sphere_color_a("sig_visualize_minigun_deflect_sphere_color_a", "0", FCVAR_NOTIFY,
		"Visualization: sphere color (alpha)");
	
	
	RefCount rc_CTFMinigun_AttackProjectiles;
	DETOUR_DECL_MEMBER(void, CTFMinigun_AttackEnemyProjectiles)
	{
		SCOPED_INCREMENT(rc_CTFMinigun_AttackProjectiles);
		DETOUR_MEMBER_CALL();
	}
	
	bool is_MiniBoss;
	DETOUR_DECL_MEMBER(bool, CTFPlayer_IsMiniBoss)
	{
		bool result = DETOUR_MEMBER_CALL();
		is_MiniBoss = result;
		return result;
	}
	
	/* UTIL_EntitiesInSphere forwards call to partition->EnumerateElementsInSphere */
	RefCount rc_EnumerateElementsInSphere;
	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInSphere, SpatialPartitionListMask_t listMask, const Vector& origin, float radius, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		SCOPED_INCREMENT(rc_EnumerateElementsInSphere);
		if (rc_CTFMinigun_AttackProjectiles > 0 && rc_EnumerateElementsInSphere <= 1 && cvar_sphere_enable.GetBool()) {
			NDebugOverlay::Sphere(origin, QAngle(0.0f, 0.0f, 0.0f), radius,
				cvar_sphere_color_r.GetInt(),
				cvar_sphere_color_g.GetInt(),
				cvar_sphere_color_b.GetInt(),
				cvar_sphere_color_a.GetInt(),
				false,
				cvar_duration.GetFloat());
			
			NDebugOverlay::Sphere(origin, QAngle(0.0f, 30.0f, 0.0f), radius,
				cvar_sphere_color_r.GetInt(),
				cvar_sphere_color_g.GetInt(),
				cvar_sphere_color_b.GetInt(),
				cvar_sphere_color_a.GetInt(),
				false,
				cvar_duration.GetFloat());
			
			NDebugOverlay::Sphere(origin, QAngle(0.0f, 60.0f, 0.0f), radius,
				cvar_sphere_color_r.GetInt(),
				cvar_sphere_color_g.GetInt(),
				cvar_sphere_color_b.GetInt(),
				cvar_sphere_color_a.GetInt(),
				false,
				cvar_duration.GetFloat());
		}
		
		return DETOUR_MEMBER_CALL(listMask, origin, radius, coarseTest, pIterator);
	}
	
	DETOUR_DECL_STATIC(float, CalcDistanceToLineSegment, const Vector& P, const Vector& vLineA, const Vector& vLineB, float *t)
	{
		float dist = DETOUR_STATIC_CALL(P, vLineA, vLineB, t);
		
		if (rc_CTFMinigun_AttackProjectiles > 0) {
			float radius = (is_MiniBoss ? 56.0f : 38.0f);
			bool is_inside = (dist <= radius);
			
			NDebugOverlay::Cross3D(P, 10.0f,
				(is_inside ? 0 : 255),
				(is_inside ? 255 : 0),
				0,
				true,
				cvar_duration.GetFloat());
			
			static int lastframe = -1;
			if (gpGlobals->framecount != lastframe) {
				lastframe = gpGlobals->framecount;
				
				/*NDebugOverlay::Line(vLineA, vLineB,
					cvar_color_r.GetFloat(),
					cvar_color_g.GetFloat(),
					cvar_color_b.GetFloat(),
					true,
					cvar_duration.GetFloat());*/
				
				QAngle ang;
				VectorAngles(vLineA - vLineB, ang);
				
				for (int i = 0; i <= 10; ++i) {
					float pA = ((float)i / 10.0f);
					float pB = (1.0f - pA);
					
					NDebugOverlay::Circle((pA * vLineA) + (pB * vLineB), ang, radius,
						cvar_color_r.GetFloat(),
						cvar_color_g.GetFloat(),
						cvar_color_b.GetFloat(),
						cvar_color_a.GetFloat(),
						false,
						cvar_duration.GetFloat());
				}
			}
		}
		
		return dist;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Minigun_Deflect")
		{
			MOD_ADD_DETOUR_MEMBER(CTFMinigun_AttackEnemyProjectiles,           "CTFMinigun::AttackEnemyProjectiles");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_IsMiniBoss,                        "CTFPlayer::IsMiniBoss");
			MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInSphere, "ISpatialPartition::EnumerateElementsInSphere");
			MOD_ADD_DETOUR_STATIC(CalcDistanceToLineSegment,                   "CalcDistanceToLineSegment");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_minigun_deflect", "0", FCVAR_NOTIFY,
		"Visualization: draw cylinder used for heavy's projectile deflection upgrade",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
	
#endif
