#include "mod.h"
#include "factory.h"
#include "stub/baseanimating.h"
#include "stub/baseentity_cl.h"
#include "util/iterate.h"


// just disable this on Linux for the time being
// until we fix the compile errors
#ifdef _WINDOWS

namespace Mod::Visualize::Hitboxes
{
	bool IsClient() { return (ClientFactory() != nullptr); }
	
	
	ConVar cvar_clear("sig_visualize_hitboxes_clear", "0", FCVAR_NOTIFY,
		"Visualization: clear between draws?");
	ConVar cvar_interval("sig_visualize_hitboxes_interval", "2", FCVAR_NOTIFY,
		"Visualization: frame interval");
	ConVar cvar_server("sig_visualize_hitboxes_server", "1", FCVAR_NOTIFY,
		"Visualization: show server hitboxes");
	ConVar cvar_client("sig_visualize_hitboxes_client", "1", FCVAR_NOTIFY,
		"Visualization: show client hitboxes");
	
	
	const Color hullcolor[8] = {
		Color(0xff, 0xff, 0xff), // white
		Color(0xff, 0x80, 0x80), // red
		Color(0x80, 0xff, 0x80), // green
		Color(0xff, 0xff, 0x80), // yellow
		Color(0x80, 0x80, 0xff), // blue
		Color(0xff, 0x80, 0xff), // magenta
		Color(0x80, 0xff, 0xff), // cyan
		Color(0xff, 0xff, 0xff), // white
	};
	
	
	const Color color_sv(0x80, 0xff, 0x80, 0x20);
	const Color color_cl(0xff, 0x80, 0xff, 0x20);
	
	
	void DrawServerHitboxes(CBaseAnimating *anim, float duration)
	{
#ifdef _WINDOWS // slower, easier method
		const studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel(modelinfo->GetModel(anim->GetModelIndex()));
#else
		const studiohdr_t *pStudioHdr = [&]() -> const studiohdr_t * {
			CStudioHdr *pCStudioHdr = anim->GetModelPtr();
			if (pCStudioHdr == nullptr) return nullptr;
			return pCStudioHdr->GetRenderHdr();
		}();
#endif
		if (pStudioHdr == nullptr) return;
		
		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet(anim->GetHitboxSet());
		if (set == nullptr) return;
		
		for (int i = 0; i < set->numhitboxes; ++i) {
			mstudiobbox_t *box = set->pHitbox(i);
			if (box->bone >= pStudioHdr->numbones) continue;
			
			matrix3x4_t bone_xform;
			anim->GetBoneTransform(box->bone, bone_xform);
			
			Vector bone_pos; QAngle bone_ang;
			MatrixAngles(bone_xform, bone_ang, bone_pos);
			
		//	int j = (box->group % 8);
		//	
		//	int r = hullcolor[j].r();
		//	int g = hullcolor[j].g();
		//	int b = hullcolor[j].b();
		//	int a = 0x20; // TODO
			
			int r = color_sv.r();
			int g = color_sv.g();
			int b = color_sv.b();
			int a = color_sv.a();
			
			NDebugOverlay::BoxAngles(bone_pos, box->bbmin * anim->GetModelScale(), box->bbmax * anim->GetModelScale(), bone_ang, r, g, b, a, duration);
		}
	}
	
	
	void DrawClientHitboxes(C_BaseAnimating *anim_cl, float duration)
	{
#ifdef _WINDOWS // slower, easier method
		const studiohdr_t *pStudioHdr = modelinfo->GetStudiomodel(anim_cl->GetModel());
#else
		const studiohdr_t *pStudioHdr = [&]() -> const studiohdr_t * {
			CStudioHdr *pCStudioHdr = anim_cl->GetModelPtr();
			if (pCStudioHdr == nullptr) return nullptr;
			return pCStudioHdr->GetRenderHdr();
		}();
#endif
		if (pStudioHdr == nullptr) return;
		
		mstudiohitboxset_t *set = pStudioHdr->pHitboxSet(anim_cl->GetHitboxSet());
		if (set == nullptr) return;
		
		for (int i = 0; i < set->numhitboxes; ++i) {
			mstudiobbox_t *box = set->pHitbox(i);
			if (box->bone >= pStudioHdr->numbones) continue;
			
			matrix3x4_t bone_xform;
			anim_cl->GetBoneTransform(box->bone, bone_xform);
			
			Vector bone_pos; QAngle bone_ang;
			MatrixAngles(bone_xform, bone_ang, bone_pos);
			
		//	int j = (box->group % 8);
		//	
		//	int r = hullcolor[j].r();
		//	int g = hullcolor[j].g();
		//	int b = hullcolor[j].b();
		//	int a = 0x20; // TODO
			
			int r = color_cl.r();
			int g = color_cl.g();
			int b = color_cl.b();
			int a = color_cl.a();
			
			NDebugOverlay::BoxAngles(bone_pos, box->bbmin * anim_cl->GetModelScale(), box->bbmax * anim_cl->GetModelScale(), bone_ang, r, g, b, a, duration);
		}
	}
	
	
	void DrawHitboxes(CBaseAnimating *anim)
	{
		if (!anim->IsPlayer() && !anim->IsBaseObject()) return;
		if (anim->GetTeamNumber() == TEAM_SPECTATOR)    return;
		if (!anim->IsAlive())                           return;
		
		float duration;
		if (cvar_clear.GetBool()) {
			duration = 10.0f;
		} else {
			duration = cvar_interval.GetInt() * gpGlobals->interval_per_tick;
		}
	//	anim->DrawServerHitboxes(duration, false);
		
		if (cvar_server.GetBool()) {
			DrawServerHitboxes(anim, duration);
		}
		
		// for now we're just finding the corresponding client entity for this server entity and draw its hitboxes
		// in the future we ought to split this up and actually iterate all client entities separately
		// because this approach will only work on listen servers
		if (cvar_client.GetBool() && IsClient()) {
			int entindex = anim->entindex();
			auto anim_cl = rtti_cast<C_BaseAnimating *>(GetClientEntityByIndex(entindex));
			
			if (anim_cl != nullptr) {
				DrawClientHitboxes(anim_cl, duration);
			}
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Hitboxes") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0; ++frame;
			if (cvar_interval.GetInt() != 0 && frame % cvar_interval.GetInt() != 0) return;
			
			if (cvar_clear.GetBool()) {
				NDebugOverlay::Clear();
			}
			
			ForEachEntityByRTTI<CBaseAnimating>([](CBaseAnimating *anim){
				DrawHitboxes(anim);
			});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_hitboxes", "0", FCVAR_NOTIFY,
		"Visualization: draw server-side hitboxes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif
