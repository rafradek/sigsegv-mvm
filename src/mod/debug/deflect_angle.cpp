#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"
#include "stub/entities.h"
#include "util/iterate.h"


namespace Mod::Debug::Deflect_Angle
{
#if 0
	class CTraceFilterProjectileDeflection : public CTraceFilterSimple
	{
	public:
		CTraceFilterProjectileDeflection(const IHandleEntity *passentity, int collisionGroup, int iIgnoreTeam) :
			CTraceFilterSimple(passentity, collisionGroup), m_iIgnoreTeam(iIgnoreTeam) {}
		
		virtual bool ShouldHitEntity(IHandleEntity *pServerEntity, int contentsMask) override
		{
			CBaseEntity *pEntity = EntityFromEntityHandle(pServerEntity);
			
			if (pEntity != nullptr && (pEntity->IsPlayer() || pEntity->IsBaseObject() || pEntity->IsCombatItem()) &&
				(this->m_iIgnoreTeam == TEAM_ANY || pEntity->GetTeamNumber() == this->m_iIgnoreTeam)) {
				return false;
			}
			
			return CTraceFilterSimple::ShouldHitEntity(pServerEntity, contentsMask);
		}
		
	private:
		int m_iIgnoreTeam;
	};
#endif
	
	
	void DrawVCollide(CBaseEntity *ent, float duration, int r, int g, int b, int a)
	{
		vcollide_t *vcollide = modelinfo->GetVCollide(ent->GetModelIndex());
		if (vcollide == nullptr) {
			Warning("vcollide == nullptr\n");
			return;
		}
		
		DevMsg("%d solids\n", vcollide->solidCount);
		for (unsigned short i = 0; i < vcollide->solidCount; ++i) {
			CPhysCollide *physcollide = vcollide->solids[i];
			
//			CPhysConvex *convexes[16];
//			int num_convexes = physcollision->GetConvexesUsedInCollideable(physcollide, convexes, 16);
			
			ICollisionQuery *query = physcollision->CreateQueryModel(physcollide);
			
			DevMsg("  %d convexes\n", query->ConvexCount());
			for (int j = 0; j < query->ConvexCount(); ++j) {
				DevMsg("    %d triangles\n", query->TriangleCount(j));
				
				for (int k = 0; k < query->TriangleCount(j); ++k) {
					Vector verts[3];
					query->GetTriangleVerts(j, k, verts);
					
					Vector wverts[3];
					for (int l = 0; l < 3; ++l) {
						VectorTransform(verts[l], ent->EntityToWorldTransform(), wverts[l]);
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
	}
	
	void DrawEntityBounds(CBaseEntity *ent, float duration, int r, int g, int b, int a)
	{
		if (rtti_cast<CTFMedigunShield *>(ent) != nullptr) {
			DrawVCollide(ent, duration, r, g, b, a);
		} else {
			NDebugOverlay::EntityBounds(ent, r, g, b, a, duration);
		}
	}
	
	
	ConVar cvar_fix("sig_debug_deflect_angle_fix", "0", FCVAR_NOTIFY,
		"Debug: apply fix for deflection angle problem");
	
	
	RefCount rc_CTFWeaponBase_DeflectEntity;
	CBaseEntity *deflect_ent = nullptr;
	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_DeflectEntity, CBaseEntity *ent, CTFPlayer *player, Vector& v1)
	{
		deflect_ent = ent;
		SCOPED_INCREMENT(rc_CTFWeaponBase_DeflectEntity);
		return DETOUR_MEMBER_CALL(CTFWeaponBase_DeflectEntity)(ent, player, v1);
	}
	
	
	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
#if 0
		/* this is structured weird for scoping reasons */
		if (rc_CTFWeaponBase_DeflectEntity > 0) {
			CTraceFilterSimple *oldfilter = rtti_cast<CTraceFilterSimple *>(pTraceFilter);
			if (oldfilter != nullptr) {
				CTFPlayer *player = ToTFPlayer(EntityFromEntityHandle(const_cast<IHandleEntity *>(oldfilter->GetPassEntity())));
				if (player != nullptr) {
					CTraceFilterProjectileDeflection filter(oldfilter->GetPassEntity(), 0, player->GetTeamNumber());
					
					DETOUR_MEMBER_CALL(IEngineTrace_TraceRay)(ray, fMask, &filter, pTrace);
					return;
				}
			}
		}
#endif
		
		DETOUR_MEMBER_CALL(IEngineTrace_TraceRay)(ray, fMask, pTraceFilter, pTrace);
		
		if (rc_CTFWeaponBase_DeflectEntity > 0 && fMask == 0x200400b) {
			NDebugOverlay::Line(pTrace->startpos, pTrace->endpos, 0x00, 0xff, 0x00, false, 2.0f);
			
			if (pTrace->m_pEnt != nullptr) {
				bool is_world      = (ENTINDEX(pTrace->m_pEnt) == 0);
				bool is_player     = (pTrace->m_pEnt->IsPlayer());
				bool is_obj        = (pTrace->m_pEnt->IsBaseObject());
				bool is_medishield = (rtti_cast<CTFMedigunShield *>(pTrace->m_pEnt) != nullptr);
				
				char buf[1024];
				if (is_player) {
					CBasePlayer *player = ToBasePlayer(pTrace->m_pEnt);
					snprintf(buf, sizeof(buf), "%s", player->GetPlayerName());
				} else if (is_obj) {
					snprintf(buf, sizeof(buf), "Building");
				} else if (is_medishield) {
					snprintf(buf, sizeof(buf), "MediShield");
				} else if (is_world) {
					snprintf(buf, sizeof(buf), "World");
				} else {
					snprintf(buf, sizeof(buf), "\"%s\"", pTrace->m_pEnt->GetClassname());
				}
				
				NDebugOverlay::EntityTextAtPosition(pTrace->endpos, 0, buf, 2.0f, 0xff, 0xff, 0xff, 0xff);
				
				if (!is_world) {
					int r = 0xff;
					int g = 0xff;
					int b = 0xff;
					if (pTrace->m_pEnt->GetTeamNumber() == TF_TEAM_RED) {
						r = 0xff;
						g = 0xc0;
						b = 0xc0;
					} else if (pTrace->m_pEnt->GetTeamNumber() == TF_TEAM_BLUE) {
						r = 0xc0;
						g = 0xc0;
						b = 0xff;
					}
					
					DrawEntityBounds(pTrace->m_pEnt, 2.0f, r, g, b, 0x20);
				}
				
				NDebugOverlay::Sphere(pTrace->endpos, vec3_angle, 1.0f, 0xff, 0x00, 0x00, 0x80, false, 2.0f);
				
				NDebugOverlay::Line(deflect_ent->WorldSpaceCenter(), pTrace->endpos, 0xff, 0xff, 0x00, false, 2.0f);
				
			//	NDebugOverlay::EntityTextAtPosition(pTrace->endpos, 1,
			//		CFmtStrN<256>("classname \"%s\"", pTrace->m_pEnt->GetClassname()),
			//		3.0f, 0xff, 0xff, 0xff, 0xff);
			}
		}
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTraceFilterSimple_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	{
		if (rc_CTFWeaponBase_DeflectEntity > 0 && cvar_fix.GetBool()) {
			auto filter = reinterpret_cast<CTraceFilterSimple *>(this);
			
			CTFPlayer *player = ToTFPlayer(EntityFromEntityHandle(const_cast<IHandleEntity *>(filter->GetPassEntity())));
			if (player != nullptr) {
				CBaseEntity *ent = EntityFromEntityHandle(pServerEntity);
				if (ent != nullptr && (ent->IsPlayer() || ent->IsBaseObject() || ent->IsCombatItem()) &&
					ent->GetTeamNumber() == player->GetTeamNumber()) {
					return false;
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(CTraceFilterSimple_ShouldHitEntity)(pServerEntity, contentsMask);
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Deflect_Angle")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_DeflectEntity,        "CTFWeaponBase::DeflectEntity");
			MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay,              "IEngineTrace::TraceRay");
			MOD_ADD_DETOUR_MEMBER(CTraceFilterSimple_ShouldHitEntity, "CTraceFilterSimple::ShouldHitEntity");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			++frame;
			
			
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_deflect_angle", "0", FCVAR_NOTIFY,
		"Debug: projectile deflection angle bug",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_debug_deflect_angle_draw_bboxes, "")
	{
		ForEachEntity([](CBaseEntity *ent){
			if (!ent->IsPlayer() && !ent->IsBaseObject() && !ent->IsCombatItem()) return;
			
			/* exclude the "local player" */
			if (ENTINDEX(ent) == 1) return;
			
			int r = 0xff;
			int g = 0xff;
			int b = 0xff;
			if (ent->GetTeamNumber() == TF_TEAM_RED) {
				r = 0xff;
				g = 0xc0;
				b = 0xc0;
			} else if (ent->GetTeamNumber() == TF_TEAM_BLUE) {
				r = 0xc0;
				g = 0xc0;
				b = 0xff;
			}
			
			DrawEntityBounds(ent, 3600.0f, r, g, b, 0x00);
		});
	}
}
