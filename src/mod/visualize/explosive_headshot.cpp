#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"


namespace Mod::Visualize::Explosive_Headshot
{
	ConVar cvar_duration("sig_visualize_explosive_headshot_duration", "0.5", FCVAR_NOTIFY,
		"Visualization: EH sphere draw duration");
	
	ConVar cvar_color_r("sig_visualize_explosive_headshot_color_r", "255", FCVAR_NOTIFY,
		"Visualization: EH sphere color (red)");
	ConVar cvar_color_g("sig_visualize_explosive_headshot_color_g", "255", FCVAR_NOTIFY,
		"Visualization: EH sphere color (green)");
	ConVar cvar_color_b("sig_visualize_explosive_headshot_color_b", "255", FCVAR_NOTIFY,
		"Visualization: EH sphere color (blue)");
	ConVar cvar_color_a("sig_visualize_explosive_headshot_color_a", "0", FCVAR_NOTIFY,
		"Visualization: EH sphere color (alpha)");
	
	
	RefCount rc_CTFSniperRifle_ExplosiveHeadShot;
	DETOUR_DECL_MEMBER(void, CTFSniperRifle_ExplosiveHeadShot, CTFPlayer *player1, CTFPlayer *player2)
	{
		SCOPED_INCREMENT(rc_CTFSniperRifle_ExplosiveHeadShot);
		DETOUR_MEMBER_CALL(CTFSniperRifle_ExplosiveHeadShot)(player1, player2);
	}
	
	/* UTIL_EntitiesInSphere forwards call to partition->EnumerateElementsInSphere */
	RefCount rc_EnumerateElementsInSphere;
	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInSphere, SpatialPartitionListMask_t listMask, const Vector& origin, float radius, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		SCOPED_INCREMENT(rc_EnumerateElementsInSphere);
		if (rc_CTFSniperRifle_ExplosiveHeadShot > 0 && rc_EnumerateElementsInSphere <= 1) {
			for (int i = 0; i < 8; ++i) {
				NDebugOverlay::Sphere(origin, QAngle(0.0f, (float)i * 22.5f, 0.0f), radius,
					cvar_color_r.GetInt(),
					cvar_color_g.GetInt(),
					cvar_color_b.GetInt(),
					cvar_color_a.GetInt(),
					false,
					cvar_duration.GetFloat());
			}
		}
		
		return DETOUR_MEMBER_CALL(ISpatialPartition_EnumerateElementsInSphere)(listMask, origin, radius, coarseTest, pIterator);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Explosive_Headshot")
		{
			MOD_ADD_DETOUR_MEMBER(CTFSniperRifle_ExplosiveHeadShot,            "CTFSniperRifle::ExplosiveHeadShot");
			MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInSphere, "ISpatialPartition::EnumerateElementsInSphere");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_ApplyOnDamageModifyRules, "CTFGameRules::ApplyOnDamageModifyRules");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_explosive_headshot", "0", FCVAR_NOTIFY,
		"Visualization: draw sphere used for sniper's explosive headshot upgrade",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
