#include "mod.h"
#include "stub/tf_shareddefs.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"
#include "stub/entities.h"
#include "util/rtti.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/clientmsg.h"

namespace Mod::Perf::Medigun_Shield_Pathfinding_ShouldHitEntity
{
	DETOUR_DECL_MEMBER(bool, NextBotTraversableTraceFilter_ShouldHitEntity, IHandleEntity *pEntity, int contentsMask)
	{
		auto result = DETOUR_MEMBER_CALL(NextBotTraversableTraceFilter_ShouldHitEntity)(pEntity, contentsMask);
		
		if (result) {
			CBaseEntity *ent = EntityFromEntityHandle(pEntity);
			if (ENTINDEX(ent) > 33) {
				return !ent->IsCombatItem();
			}
		}
		
		return result;
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Medigun_Shield_Pathfinding")
		{
			MOD_ADD_DETOUR_MEMBER(NextBotTraversableTraceFilter_ShouldHitEntity, "NextBotTraversableTraceFilter::ShouldHitEntity");

		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_medigunshield_obstruction_override_shouldhitentity", "0", FCVAR_NOTIFY,
		"Mod: Override NextBotTraversableTraceFilter::ShouldHitEntity",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

namespace Mod::Perf::Medigun_Shield_Pathfinding_IsEntityTraversable
{
	DETOUR_DECL_MEMBER(bool, CTFBotLocomotion_IsEntityTraversable, CBaseEntity *ent, ILocomotion::TraverseWhenType ttype)
	{
		auto result = DETOUR_MEMBER_CALL(CTFBotLocomotion_IsEntityTraversable)(ent, ttype);
		if (!result && ENTINDEX(ent) > 33) {
			return ent->IsCombatItem();
		}
		
		return result;
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Medigun_Shield_Pathfinding")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotLocomotion_IsEntityTraversable,          "CTFBotLocomotion::IsEntityTraversable");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_medigunshield_obstruction_override_isentitytraversable", "0", FCVAR_NOTIFY,
		"Mod: Override CTFBotLocomotion::IsEntityTraversable",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}