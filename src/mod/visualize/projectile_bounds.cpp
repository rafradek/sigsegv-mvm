#include "mod.h"
#include "stub/baseplayer.h"
#include "stub/projectiles.h"
#include "util/iterate.h"


namespace Mod::Visualize::Projectile_Bounds
{
	void DrawVCollide(CBaseEntity *ent, float duration, int r, int g, int b, int a)
	{
		if (ent->GetModelIndex() <= 0) return;
		
		vcollide_t *vcollide = modelinfo->GetVCollide(ent->GetModelIndex());
		if (vcollide == nullptr) return;
		
//		matrix3x4_t temp;
		
		const matrix3x4_t& xform_ent_to_world = ent->EntityToWorldTransform();
//		const matrix3x4_t& xform_parent
		
#if 0
		for (unsigned short i = 0; i < vcollide->solidCount; ++i) {
			ICollisionQuery *query = physcollision->CreateQueryModel(vcollide->solids[i]);
			for (int j = 0; j < query->ConvexCount(); ++j) {
				for (int k = 0; k < query->TriangleCount(j); ++k) {
					Vector verts[3];
					query->GetTriangleVerts(j, k, verts);
					
					Vector wverts[3];
					for (int l = 0; l < 3; ++l) {
						VectorTransform(verts[l], xform_ent_to_world, wverts[l]);
					//	if (ent->GetMoveParent() != nullptr) {
					//		VectorTransform(verts[l], ent->)
					//	}
					}
					
					NDebugOverlay::Line(wverts[0], wverts[1], r, g, b, false, duration);
					NDebugOverlay::Line(wverts[1], wverts[2], r, g, b, false, duration);
					NDebugOverlay::Line(wverts[2], wverts[0], r, g, b, false, duration);
					
					if (a > 0) {
						NDebugOverlay::Triangle(wverts[0], wverts[1], wverts[2], r, g, b, a, false, duration);
						NDebugOverlay::Triangle(wverts[0], wverts[2], wverts[1], r, g, b, a, false, duration);
					}
				}
			}
			physcollision->DestroyQueryModel(query);
		}
#else
		#warning TODO: FINISH MOD Visualize:Projectile_Bounds
		// attempt to replicate how ::DebugDrawPhysCollide works (Source2007)
		// https://mail.google.com/mail/u/0/#inbox/162b75a8ab7283e9
#endif
	}
	
	void DrawEntityBounds(CBaseEntity *ent)
	{
		constexpr float duration = 1.0f;
		
		DrawVCollide(ent, duration, 0xff, 0x00, 0xff, 0x40);
		NDebugOverlay::EntityBounds(ent, 0x00, 0xff, 0x00, 0x20, duration);
	}
	
	
	class CMod : public IMod, public IFrameUpdateListener
	{
	public:
		CMod() : IMod("Visualize:Projectile_Bounds") {}
		
		virtual bool ShouldReceiveFrameEvents() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			NDebugOverlay::Clear();
			
			ForEachPlayer([](CBasePlayer *player){
				DrawEntityBounds(player);
			});
			
			for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
				auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
				assert(proj != nullptr);
				
				DrawEntityBounds(proj);
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_Projectile_Bounds", "0", FCVAR_NOTIFY,
		"Visualization: projectile bounds",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
