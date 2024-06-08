#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/tf_shareddefs.h"
#include "stub/tfentities.h"


namespace Mod::Debug::Deflect_Angle_v2
{
	void DrawEntityBounds(CBaseEntity *ent, float duration, int r, int g, int b, int a)
	{
		if (rtti_cast<CTFMedigunShield *>(ent) != nullptr) {
		//	DrawVCollide(ent, duration, r, g, b, a);
		} else {
			NDebugOverlay::EntityBounds(ent, r, g, b, a, duration);
		}
	}
	
	
	RefCount rc_CTFWeaponBase_DeflectEntity;
	CBaseEntity *deflect_ent = nullptr;
	DETOUR_DECL_MEMBER(bool, CTFWeaponBase_DeflectEntity, CBaseEntity *ent, CTFPlayer *player, Vector& v1)
	{
		deflect_ent = ent;
		SCOPED_INCREMENT(rc_CTFWeaponBase_DeflectEntity);
		return DETOUR_MEMBER_CALL(ent, player, v1);
	}
	
	
	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
		DETOUR_MEMBER_CALL(ray, fMask, pTraceFilter, pTrace);
		
		if (rc_CTFWeaponBase_DeflectEntity > 0 && fMask == 0x200400b) {
		//	NDebugOverlay::Line(pTrace->startpos, pTrace->endpos, 0x00, 0xff, 0x00, false, 2.0f);
			NDebugOverlay::VertArrow(pTrace->startpos, pTrace->endpos, 2.5f, 0x00, 0xff, 0x00, 0xff, true, 2.0f);
			
			NDebugOverlay::EntityTextAtPosition(pTrace->startpos, -2, "Trace Start", 2.0f, 0x80, 0xff, 0x80, 0xff);
			
			if (pTrace->m_pEnt != nullptr) {
				bool is_world      = (ENTINDEX(pTrace->m_pEnt) == 0);
				bool is_player     = (pTrace->m_pEnt->IsPlayer());
				bool is_obj        = (pTrace->m_pEnt->IsBaseObject());
				bool is_medishield = (rtti_cast<CTFMedigunShield *>(pTrace->m_pEnt) != nullptr);
				
				char buf[1024];
				if (is_player) {
					CBasePlayer *player = ToBasePlayer(pTrace->m_pEnt);
					snprintf(buf, sizeof(buf), "Hit:%s", player->GetPlayerName());
				} else if (is_obj) {
					snprintf(buf, sizeof(buf), "Hit:Building");
				} else if (is_medishield) {
					snprintf(buf, sizeof(buf), "Hit:MediShield");
				} else if (is_world) {
					snprintf(buf, sizeof(buf), "Hit:World");
				} else {
					snprintf(buf, sizeof(buf), "Hit:\"%s\"", pTrace->m_pEnt->GetClassname());
				}
				
				NDebugOverlay::EntityTextAtPosition(pTrace->endpos, -3, "Trace End", 2.0f, 0x80, 0xff, 0x80, 0xff);
				NDebugOverlay::EntityTextAtPosition(pTrace->endpos, -2, buf,         2.0f, 0x80, 0xff, 0x80, 0xff);
				
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
				
			//	NDebugOverlay::Sphere(pTrace->endpos, vec3_angle, 3.0f, 0xff, 0xff, 0xff, 0x80, false, 2.0f);
				
			//	NDebugOverlay::Line(deflect_ent->WorldSpaceCenter(), pTrace->endpos, 0xff, 0xff, 0x00, false, 2.0f);
				NDebugOverlay::VertArrow(deflect_ent->WorldSpaceCenter(), pTrace->endpos, 2.5f, 0xff, 0xff, 0x00, 0xff, true, 2.0f);
				
				NDebugOverlay::EntityTextAtPosition(deflect_ent->WorldSpaceCenter(), 1, "Projectile", 2.0f, 0xff, 0xff, 0x80, 0xff);
				NDebugOverlay::EntityTextAtPosition(deflect_ent->WorldSpaceCenter(), 2, "Source",     2.0f, 0xff, 0xff, 0x80, 0xff);
				
				NDebugOverlay::EntityTextAtPosition(pTrace->endpos, 1, "Projectile", 2.0f, 0xff, 0xff, 0x80, 0xff);
				NDebugOverlay::EntityTextAtPosition(pTrace->endpos, 2, "Target",     2.0f, 0xff, 0xff, 0x80, 0xff);
				
			//	NDebugOverlay::EntityTextAtPosition(pTrace->endpos, 1,
			//		CFmtStrN<256>("classname \"%s\"", pTrace->m_pEnt->GetClassname()),
			//		3.0f, 0xff, 0xff, 0xff, 0xff);
			}
		}
	}
	
	
	ConVar cvar_fix("sig_debug_deflect_angle_v2_fix", "0", FCVAR_NOTIFY,
		"Debug: make CTraceFilterDeflection ignore all players, regardless of team");
	
	DETOUR_DECL_MEMBER(bool, CTraceFilterDeflection_ShouldHitEntity, IHandleEntity *pHandleEntity, int contentsMask)
	{
		if (cvar_fix.GetBool()) {
			CBaseEntity *pEntity = EntityFromEntityHandle(pHandleEntity);
			if (pEntity == nullptr) return false;
			
			if (pEntity->IsPlayer()) return false;
		}
		
		return DETOUR_MEMBER_CALL(pHandleEntity, contentsMask);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Deflect_Angle_v2")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_DeflectEntity, "CTFWeaponBase::DeflectEntity");
			MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay,       "IEngineTrace::TraceRay");
			
			MOD_ADD_DETOUR_MEMBER(CTraceFilterDeflection_ShouldHitEntity, "CTraceFilterDeflection::ShouldHitEntity");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_deflect_angle_v2", "0", FCVAR_NOTIFY,
		"Debug: projectile deflection angle bug (post-MYM-update)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
