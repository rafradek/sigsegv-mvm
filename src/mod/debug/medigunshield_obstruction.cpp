#include "mod.h"
#include "util/scope.h"
#include "stub/tfentities.h"
#include "util/rtti.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/clientmsg.h"


namespace Mod::Debug::MedigunShield_Obstruction
{
	// - PathFollower::Avoid
	//   - ILocomotion::TraceHull
	
	// NextBotTraversableTraceFilter
	
	
	// PathFollower::m_vecAvoidMins.x = body->GetHullWidth() * -0.25f
	// PathFollower::m_vecAvoidMins.y = body->GetHullWidth() * -0.25f
	// PathFollower::m_vecAvoidMins.z = loco->GetStepHeight() + 0.1f
	
	// PathFollower::m_vecAvoidMaxs.x = body->GetHullWidth() * 0.25f
	// PathFollower::m_vecAvoidMaxs.y = body->GetHullWidth() * 0.25f
	// PathFollower::m_vecAvoidMaxs.z = body->GetCrouchHullHeight()
	
	
	// PathFollower::m_vecAvoid1Start.x = feet->x + (norm->x * ((body->GetHullWidth() * 0.25f) + 2.0f))
	// PathFollower::m_vecAvoid1Start.y = feet->y + (norm->y * ((body->GetHullWidth() * 0.25f) + 2.0f))
	// PathFollower::m_vecAvoid1Start.z = feet->z + (norm->z * ((body->GetHullWidth() * 0.25f) + 2.0f))
	
	// PathFollower::m_vecAvoid1End.x = m_vecAvoid1Start.x + (dir->x * m_flModelScale * (loco->IsRunning() ? 50.0f : 30.0f))
	// PathFollower::m_vecAvoid1End.y = m_vecAvoid1Start.y + (dir->y * m_flModelScale * (loco->IsRunning() ? 50.0f : 30.0f))
	// PathFollower::m_vecAvoid1End.z = m_vecAvoid1Start.z + (dir->z * m_flModelScale * (loco->IsRunning() ? 50.0f : 30.0f))
	
	
	// PathFollower::m_vecAvoid2Start.x = feet->x - (norm->x * ((body->GetHullWidth() * 0.25f) + 2.0f))
	// PathFollower::m_vecAvoid2Start.y = feet->y - (norm->y * ((body->GetHullWidth() * 0.25f) + 2.0f))
	// PathFollower::m_vecAvoid2Start.z = feet->z - (norm->z * ((body->GetHullWidth() * 0.25f) + 2.0f))
	
	// PathFollower::m_vecAvoid2End.x = m_vecAvoid2Start.x + (dir->x * m_flModelScale * (loco->IsRunning() ? 50.0f : 30.0f))
	// PathFollower::m_vecAvoid2End.y = m_vecAvoid2Start.y + (dir->y * m_flModelScale * (loco->IsRunning() ? 50.0f : 30.0f))
	// PathFollower::m_vecAvoid2End.z = m_vecAvoid2Start.z + (dir->z * m_flModelScale * (loco->IsRunning() ? 50.0f : 30.0f))
	
	
	RefCount rc_ILocomotion_TraceHull;
	DETOUR_DECL_MEMBER(void, ILocomotion_TraceHull, const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, ITraceFilter *filter, CGameTrace *trace)
	{
		auto loco = reinterpret_cast<ILocomotion *>(this);
		auto bot = rtti_cast<CTFBot *>(loco->GetBot());
		if (bot != nullptr) {
			ClientMsgAll("ILocomotion::TraceHull(bot #%d)\n",
				ENTINDEX(bot));
	//			"  start [ %+7.1f %+7.1f %+7.1f ]\n"
	//			"  end   [ %+7.1f %+7.1f %+7.1f ]\n"
	//			"  mins  [ %+7.1f %+7.1f %+7.1f ]\n"
	//			"  maxs  [ %+7.1f %+7.1f %+7.1f ]\n",
	//			start.x, start.y, start.z,
	//			end.x,   end.y,   end.z,
	//			mins.x,  mins.y,  mins.z,
	//			maxs.x,  maxs.y,  maxs.z);
		}
		
		//NDebugOverlay::Clear();
		
	//	NDebugOverlay::Cross3D(start, 2.0f, 0xff, 0x00, 0x00, false, 0.1f);
	//	NDebugOverlay::EntityTextAtPosition(start, 1, "start", 0.1f, 0xff, 0x00, 0x00, 0xff);
		
	//	NDebugOverlay::Cross3D(end, 2.0f, 0xff, 0x00, 0x00, false, 0.1f);
	//	NDebugOverlay::EntityTextAtPosition(end, 1, "end", 0.1f, 0xff, 0x00, 0x00, 0xff);
		
		SCOPED_INCREMENT(rc_ILocomotion_TraceHull);
		DETOUR_MEMBER_CALL(start, end, mins, maxs, mask, filter, trace);
	}
	
	
	ConVar cvar_override_trace("sig_debug_medigunshield_obstruction_override_trace", "0", FCVAR_NOTIFY,
		"Override NextBotTraversableTraceFilter::ShouldHitEntity");
	DETOUR_DECL_MEMBER(bool, NextBotTraversableTraceFilter_ShouldHitEntity, IHandleEntity *pEntity, int contentsMask)
	{
		auto result = DETOUR_MEMBER_CALL(pEntity, contentsMask);
		
		if (rc_ILocomotion_TraceHull > 0) {
			auto shield = rtti_cast<CTFMedigunShield *>(pEntity);
			if (shield != nullptr) {
				ClientMsgAll("  NextBotTraversableTraceFilter::ShouldHitEntity: %5s for CTFMedigunShield #%d on team %d\n",
					(result ? "TRUE" : "FALSE"),
					ENTINDEX(shield),
					shield->GetTeamNumber());
				
				if (result && cvar_override_trace.GetBool()) {
					ClientMsgAll("    [!!] overriding ShouldHitEntity result to FALSE\n");
					return false;
				}
			}
		}
		
		return result;
	}
	
	
	ConVar cvar_override_traversable("sig_debug_medigunshield_obstruction_override_traversable", "0", FCVAR_NOTIFY,
		"Override CTFBotLocomotion::IsEntityTraversable");
	DETOUR_DECL_MEMBER(bool, CTFBotLocomotion_IsEntityTraversable, CBaseEntity *ent, ILocomotion::TraverseWhenType ttype)
	{
		auto result = DETOUR_MEMBER_CALL(ent, ttype);
		
		auto shield = rtti_cast<CTFMedigunShield *>(ent);
		if (shield != nullptr) {
			auto loco = reinterpret_cast<ILocomotion *>(this);
			auto bot = rtti_cast<CTFBot *>(loco->GetBot());
			if (bot != nullptr) {
				ClientMsgAll("CTFBotLocomotion::IsEntityTraversable(bot #%d): %5s for CTFMedigunShield #%d on team %d\n",
					ENTINDEX(bot),
					(result ? "TRUE" : "FALSE"),
					ENTINDEX(shield),
					shield->GetTeamNumber());
				
				if (!result && cvar_override_traversable.GetBool()) {
					ClientMsgAll("  [!!] overriding IsEntityTraversable result to TRUE\n");
					return true;
				}
				
			//	if (shield->GetTeamNumber() == bot->GetTeamNumber()) {
			//		return true;
			//	}
			}
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:MedigunShield_Obstruction")
		{
			MOD_ADD_DETOUR_MEMBER(ILocomotion_TraceHull,                         "ILocomotion::TraceHull");
			MOD_ADD_DETOUR_MEMBER(NextBotTraversableTraceFilter_ShouldHitEntity, "NextBotTraversableTraceFilter::ShouldHitEntity");
			MOD_ADD_DETOUR_MEMBER(CTFBotLocomotion_IsEntityTraversable,          "CTFBotLocomotion::IsEntityTraversable");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_medigunshield_obstruction", "0", FCVAR_NOTIFY,
		"Debug: find out why friendly medigun shields are seen as path obstructions by bots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
