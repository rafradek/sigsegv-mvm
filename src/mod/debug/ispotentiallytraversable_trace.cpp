#include "mod.h"
#include "re/nextbot.h"
#include "util/scope.h"


namespace Mod::Debug::IsPotentiallyTraversable_Trace
{
	// do the opposite of what Ray_t::Init does, to recover the original input values
	void Ray_UnInit(const Ray_t& ray, Vector& start, Vector& end, Vector& mins, Vector& maxs)
	{
		// m_Start       = start + ((mins + maxs) / 2);
		// m_Delta       = (end - start);
		// m_StartOffset = (-maxs - mins) / 2;
		// m_Extents     = ( maxs - mins) / 2;
		
		start = (ray.m_Start + ray.m_StartOffset);
		end   = (ray.m_Start + ray.m_StartOffset) + ray.m_Delta;
		
		mins = (-ray.m_Extents - ray.m_StartOffset);
		maxs = ( ray.m_Extents - ray.m_StartOffset);
	}
	
	
	void SanityCheck()
	{
		int n_pass  = 0;
		int n_total = 0;
		
		for (int i = 0; i < 100; ++i) {
			Vector in[4];
			Ray_t ray;
			Vector out[4];
			
			for (int j = 0; j < 4; ++j) {
				for (int k = 0; k < 3; ++k) {
					in[j][k] = RandomFloat(-1000.0f, 1000.0f);
				}
			}
			
			ray.Init(in[0], in[1], in[2], in[3]);
			Ray_UnInit(ray, out[0], out[1], out[2], out[3]);
			
			bool pass = true;
			
			for (int j = 0; j < 4; ++j) {
				if (!VectorsAreEqual(in[j], out[j], 0.01f)) {
					pass = false;
					break;
				}
			}
			
			if (pass) {
				++n_pass;
			}
			
			++n_total;
		}
		
		DevMsg("[SANITY CHECK] %d/%d passed\n", n_pass, n_total);
	}
	
	
	RefCount rc_ILocomotion_IsPotentiallyTraversable;
	ILocomotion *loco = nullptr;
	DETOUR_DECL_MEMBER(bool, ILocomotion_IsPotentiallyTraversable, const Vector& from, const Vector& to, int when, float *pFraction)
	{
		SCOPED_INCREMENT(rc_ILocomotion_IsPotentiallyTraversable);
		loco = reinterpret_cast<ILocomotion *>(this);
		return DETOUR_MEMBER_CALL(from, to, when, pFraction);
	}
	
	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
		if (rc_ILocomotion_IsPotentiallyTraversable > 0) {
			SanityCheck();
			
			Vector start, end, mins, maxs;
			Ray_UnInit(ray, start, end, mins, maxs);
			
			DevMsg("ILocomotion::IsPotentiallyTraversable\n");
			DevMsg(" from:    [ % 6.1f % 6.1f % 6.1f ]\n", VectorExpand(start));
			DevMsg(" to:      [ % 6.1f % 6.1f % 6.1f ]\n", VectorExpand(end));
			DevMsg(" vecMins: [ % 6.1f % 6.1f % 6.1f ]\n", VectorExpand(mins));
			DevMsg(" vecMaxs: [ % 6.1f % 6.1f % 6.1f ]\n", VectorExpand(maxs));
			DevMsg(" step_height:        %.1f\n", loco->GetStepHeight());
			DevMsg(" hull_width:         %.1f\n", loco->GetBot()->GetBodyInterface()->GetHullWidth());
			DevMsg(" crouch_hull_height: %.1f\n", loco->GetBot()->GetBodyInterface()->GetCrouchHullHeight());
			DevMsg("\n");
		}
		
		DETOUR_MEMBER_CALL(ray, fMask, pTraceFilter, pTrace);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:IsPotentiallyTraversable_Trace")
		{
			MOD_ADD_DETOUR_MEMBER(ILocomotion_IsPotentiallyTraversable, "ILocomotion::IsPotentiallyTraversable");
			MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay,                "IEngineTrace::TraceRay");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_ispotentiallytraversable_trace", "0", FCVAR_NOTIFY,
		"Debug: spew info about ILocomotion::IsPotentiallyTraversable's ray trace",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
