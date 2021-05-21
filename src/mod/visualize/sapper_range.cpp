#include "mod.h"
#include "stub/tfweaponbase.h"
#include "stub/objects.h"
#include "util/iterate.h"


namespace Mod::Visualize::Sapper_Range
{
	void DrawSapperRadius(CTFWeaponBuilder *sapper, float duration)
	{
		CTFPlayer *owner = sapper->GetTFPlayerOwner();
		if (owner == nullptr || !owner->IsPlayerClass(TF_CLASS_SPY)) return;
		
		static ConVarRef tf_obj_max_attach_dist("tf_obj_max_attach_dist");
		float max_dist = tf_obj_max_attach_dist.GetFloat();
		
		NDebugOverlay::Sphere(owner->GetAbsOrigin(), vec3_angle, max_dist, 0x00, 0xff, 0x00, 0x08, true, duration);
	}
	
	void DrawBuildPoints(CBaseObject *obj, float duration)
	{
		auto buildpoints = rtti_cast<IHasBuildPoints *>(obj);
		if (buildpoints == nullptr) return;
		
		int n_buildpoints = buildpoints->GetNumBuildPoints();
		for (int i = 0; i < n_buildpoints; ++i) {
			Vector vecOrigin;
			QAngle vecAngles;
			if (!buildpoints->GetBuildPoint(i, vecOrigin, vecAngles)) continue;
			
		//	NDebugOverlay::Cross3DOriented(vecOrigin, vecAngles, 5.0f, 0xff, 0xff, 0x00, true, duration);
			NDebugOverlay::Sphere(vecOrigin, vecAngles, 2.0f, 0xff, 0xff, 0x00, 0x80, true, duration);
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Sapper_Range") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 != 0) return;
			
			
			ForEachEntity([](CBaseEntity *ent){
				float duration = 0.030f;
				
				auto sapper = rtti_cast<CTFWeaponBuilder *>(ent);
				if (sapper != nullptr) {
					DrawSapperRadius(sapper, duration);
				}
				
				auto obj = rtti_cast<CBaseObject *>(ent);
				if (obj != nullptr) {
					DrawBuildPoints(obj, duration);
				}
			});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_sapper_range", "0", FCVAR_NOTIFY,
		"Visualization: draw sapper attachment range",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
