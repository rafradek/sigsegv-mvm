#include "mod.h"
#include "util/scope.h"
#include "stub/populators.h"
#include "stub/gamerules.h"
#include "stub/trace.h"
#include <gamemovement.h>
#include "util/clientmsg.h"

namespace Mod::Bot::IsSpaceToSpawnHere_Scale
{
	/* real implementation in 20151007a */
#if 0
	bool IsSpaceToSpawnHere(const Vector& pos)
	{
		Vector vecMins = VEC_HULL_MIN + Vector(-5.0f, -5.0f, 0.0f);
		Vector vecMaxs = VEC_HULL_MAX + Vector( 5.0f,  5.0f, 5.0f);
		
		trace_t tr;
		UTIL_TraceHull(pos, pos, vecMins, vecMaxs, MASK_PLAYERSOLID, nullptr, COLLISION_GROUP_PLAYER_MOVEMENT, &tr);
		
		static ConVarRef tf_debug_placement_failure("tf_debug_placement_failure");
		if (tf_debug_placement_failure.GetBool() && tr.fraction < 1.0f) {
			NDebugOverlay::Cross3D(pos, 5.0f, 0xff, 0x64, 0x00, true, 1.0e+5f);
		}
		
		return (tr.fraction >= 1.0f);
	}
#endif
	
	// 1. take model scale into account
	// 2. do a sanity check here... are they doing the trace properly
	// 3. take this example and build a proof-of-concept for an engiebot push force fix!
	
	
	ConVar cvar_debug("sig_bot_isspacetospawnhere_scale_debug", "0", FCVAR_NOTIFY,
		"Mod: Spew debug information when doing spawn trace");
	
	ConVar cvar_botspawnfix("sig_bot_isspacetospawnhere_scale_bot_spawn", "0", FCVAR_NOTIFY,
		"Mod: prevent spawning miniboss bots in places where its not applicable");
	
	CTFBotSpawner *the_spawner = nullptr;
	float the_bot_scale = 1.0f;
	RefCount rc_CTFBotSpawner_Spawn;
	DETOUR_DECL_MEMBER(int, CTFBotSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		static ConVarRef tf_mvm_miniboss_scale("tf_mvm_miniboss_scale");
		
		auto spawner = reinterpret_cast<CTFBotSpawner *>(this);
		
		the_spawner   = spawner;
		the_bot_scale = (spawner->IsMiniBoss(-1) ? tf_mvm_miniboss_scale.GetFloat() : 1.0f);
		if (spawner->m_flScale != -1.0f) the_bot_scale = spawner->m_flScale;
	
		SCOPED_INCREMENT_IF(rc_CTFBotSpawner_Spawn, the_bot_scale != 1.0f && cvar_botspawnfix.GetBool());
		
		return DETOUR_MEMBER_CALL(where, ents);
	}
	
	
	CTFBot *the_spy = nullptr;
	float the_spy_scale = 1.0f;
	RefCount rc_TeleportNearVictim;
	DETOUR_DECL_STATIC(bool, TeleportNearVictim, CTFBot *spy, CTFPlayer *victim, int dist)
	{
		the_spy       = spy;
		the_spy_scale = spy->GetModelScale();
		
		SCOPED_INCREMENT_IF(rc_TeleportNearVictim, the_spy_scale != 1.0f);
		return DETOUR_STATIC_CALL(spy, victim, dist);
	}
	
	
	const Vector *the_pos;
	RefCount rc_IsSpaceToSpawnHere;
	DETOUR_DECL_STATIC(bool, IsSpaceToSpawnHere, const Vector& pos)
	{
		SCOPED_INCREMENT(rc_IsSpaceToSpawnHere);
		the_pos = &pos;
		
		if (cvar_debug.GetBool()) DevMsg("\nIsSpaceToSpawnHere BEGIN @ %.7f\n", Plat_FloatTime());
		auto result = DETOUR_STATIC_CALL(pos);
		if (cvar_debug.GetBool()) DevMsg("IsSpaceToSpawnHere END   @ %.7f\n\n", Plat_FloatTime());
		return result;
	}
	
	CBasePlayer *stuck_player = nullptr;
	CMoveData *stuck_player_move = nullptr;
	bool stuck_player_red = false;
	bool isbotmvm = false;

	DETOUR_DECL_MEMBER(int, CTFGameMovement_CheckStuck)
	{
		CBasePlayer *player = reinterpret_cast<CGameMovement *>(this)->player;
		
		if (player->IsAlive() && player->GetModelScale() > 1.0f) {
			stuck_player = player;
			stuck_player_move = reinterpret_cast<CGameMovement *>(this)->GetMoveData();
		}

		bool isbotmvm = player->IsBot() && TFGameRules()->IsMannVsMachineMode();

		int result = DETOUR_MEMBER_CALL();

		if (isbotmvm)
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
		isbotmvm = false;

		stuck_player = nullptr;

		return result;
	}
	
	std::map<CHandle<CTFPlayer>, float> old_scale_map;
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnRemoveHalloweenTiny)
	{
		CTFPlayerShared *shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		CTFPlayer *player = shared->GetOuter();
		
		player->GetAttributeList()->RemoveAttribute(GetItemSchema()->GetAttributeDefinitionByName("head scale"));
		player->GetAttributeList()->RemoveAttribute(GetItemSchema()->GetAttributeDefinitionByName("voice pitch scale"));

		float old_scale = 1.0f;

		for (auto it = old_scale_map.begin(); it != old_scale_map.end(); ) {
			//Clear old entries
			if (it->first == nullptr || !(it->first->IsAlive())) {
				it = old_scale_map.erase(it);	
			}
			else if (it->first == player) {
				old_scale = it->second;
				it = old_scale_map.erase(it);
			}
			else {
				it++;
			}
		}

		player->SetModelScale(old_scale);

		const Vector& vOrigin = player->GetAbsOrigin();
		const QAngle& qAngle = player->GetAbsAngles();
		const Vector& vHullMins = (player->GetFlags() & FL_DUCKING ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN) * player->GetModelScale();
		const Vector& vHullMaxs = (player->GetFlags() & FL_DUCKING ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX) * player->GetModelScale();

		trace_t result;
		CTraceFilterIgnoreTeammates filter(player, COLLISION_GROUP_NONE);
		UTIL_TraceHull(vOrigin, vOrigin, vHullMins, vHullMaxs, MASK_PLAYERSOLID, &filter, &result);
		// am I stuck? try to resolve it
		if ( result.DidHit() )
		{
			float flPlayerHeight = vHullMaxs.z - vHullMins.z;
			float flExtraHeight = 10;
			float flSpaceMove = old_scale * 24;

			static Vector vTest[] =
			{
				Vector( flSpaceMove, flSpaceMove, flExtraHeight ),
				Vector( -flSpaceMove, -flSpaceMove, flExtraHeight ),
				Vector( -flSpaceMove, flSpaceMove, flExtraHeight ),
				Vector( flSpaceMove, -flSpaceMove, flExtraHeight ),
				Vector( 0, 0, flPlayerHeight * 0.5 + flExtraHeight ),
				Vector( 0, 0, -flPlayerHeight - flExtraHeight )
			};
			for ( int i=0; i<ARRAYSIZE( vTest ); ++i )
			{
				Vector vTestPos = vOrigin + vTest[i];
				UTIL_TraceHull(vTestPos, vTestPos, vHullMins, vHullMaxs, MASK_PLAYERSOLID, &filter, &result);
				if (!result.DidHit())
				{
					player->Teleport(&vTestPos, &qAngle, NULL);
					return;
				}
			}

			player->CommitSuicide( false, true );
		}
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_OnAddHalloweenTiny)
	{
		CTFPlayerShared *shared = reinterpret_cast<CTFPlayerShared *>(this);
		
		CTFPlayer *player = shared->GetOuter();
		
		old_scale_map[player] = player->GetModelScale();

		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
		if (rc_IsSpaceToSpawnHere > 0 && the_pos != nullptr) {
			CTraceFilterIgnorePlayers filter(nullptr, COLLISION_GROUP_PLAYER_MOVEMENT);
			pTraceFilter = &filter;
			if (rc_CTFBotSpawner_Spawn > 0 && the_spawner != nullptr) {
				Vector vecMins = (VEC_HULL_MIN * the_bot_scale) + Vector(-5.0f, -5.0f, 0.0f);
				Vector vecMaxs = (VEC_HULL_MAX * the_bot_scale) + Vector( 5.0f,  5.0f, 5.0f);
				
				Ray_t ray_alt;
				ray_alt.Init(*the_pos, *the_pos, vecMins, vecMaxs);
				
				if (cvar_debug.GetBool()) {
					DevMsg("IsSpaceToSpawnHere via CTFBotSpawner::Spawn\n");
					DevMsg("  from spawner @ 0x%08x with icon \"%s\" and scale %f\n", (uintptr_t)the_spawner, STRING(the_spawner->m_strClassIcon), the_spawner->m_flScale);
					DevMsg("    and the_pos [ %+6.1f %+6.1f %+6.1f ]\n", the_pos->x, the_pos->y, the_pos->z);
					
					DevMsg("  fixed-up mins and maxs:\n");
					DevMsg("    vecMins: [ %+6.1f %6.1f %6.1f ]\n", vecMins.x, vecMins.y, vecMins.z);
					DevMsg("    vecMaxs: [ %+6.1f %6.1f %6.1f ]\n", vecMaxs.x, vecMaxs.y, vecMaxs.z);
					
					DevMsg("  would have used this ray:\n");
					DevMsg("    m_Start:       [ %+6.1f %6.1f %6.1f ]\n", ray.m_Start.x,       ray.m_Start.y,       ray.m_Start.z);
					DevMsg("    m_Delta:       [ %+6.1f %6.1f %6.1f ]\n", ray.m_Delta.x,       ray.m_Delta.y,       ray.m_Delta.z);
					DevMsg("    m_StartOffset: [ %+6.1f %6.1f %6.1f ]\n", ray.m_StartOffset.x, ray.m_StartOffset.y, ray.m_StartOffset.z);
					DevMsg("    m_Extents:     [ %+6.1f %6.1f %6.1f ]\n", ray.m_Extents.x,     ray.m_Extents.y,     ray.m_Extents.z);
					
					DevMsg("  actually using this ray:\n");
					DevMsg("    m_Start:       [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_Start.x,       ray_alt.m_Start.y,       ray_alt.m_Start.z);
					DevMsg("    m_Delta:       [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_Delta.x,       ray_alt.m_Delta.y,       ray_alt.m_Delta.z);
					DevMsg("    m_StartOffset: [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_StartOffset.x, ray_alt.m_StartOffset.y, ray_alt.m_StartOffset.z);
					DevMsg("    m_Extents:     [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_Extents.x,     ray_alt.m_Extents.y,     ray_alt.m_Extents.z);
					
					NDebugOverlay::Box(*the_pos, vecMins, vecMaxs, 0xff, 0xff, 0xff, 0x40, 10.0f);
				}
				
				DETOUR_MEMBER_CALL(ray_alt, fMask, pTraceFilter, pTrace);
				return;
			}
			
			if (rc_TeleportNearVictim > 0 && the_spy != nullptr) {
				Vector vecMins = (VEC_HULL_MIN * the_spy_scale) + Vector(-5.0f, -5.0f, 0.0f);
				Vector vecMaxs = (VEC_HULL_MAX * the_spy_scale) + Vector( 5.0f,  5.0f, 5.0f);
				
				Ray_t ray_alt;
				ray_alt.Init(*the_pos, *the_pos, vecMins, vecMaxs);
				
				if (cvar_debug.GetBool()) {
					DevMsg("\nIsSpaceToSpawnHere via TeleportNearVictim\n");
					DevMsg("  from spy #%d with name \"%s\" and scale %f\n", ENTINDEX(the_spy), the_spy->GetPlayerName(), the_spy->GetModelScale());
					DevMsg("    and the_pos [ %+6.1f %+6.1f %+6.1f ]\n", the_pos->x, the_pos->y, the_pos->z);
					
					DevMsg("  fixed-up mins and maxs:\n");
					DevMsg("    vecMins: [ %+6.1f %6.1f %6.1f ]\n", vecMins.x, vecMins.y, vecMins.z);
					DevMsg("    vecMaxs: [ %+6.1f %6.1f %6.1f ]\n", vecMaxs.x, vecMaxs.y, vecMaxs.z);
					
					DevMsg("  would have used this ray:\n");
					DevMsg("    m_Start:       [ %+6.1f %6.1f %6.1f ]\n", ray.m_Start.x,       ray.m_Start.y,       ray.m_Start.z);
					DevMsg("    m_Delta:       [ %+6.1f %6.1f %6.1f ]\n", ray.m_Delta.x,       ray.m_Delta.y,       ray.m_Delta.z);
					DevMsg("    m_StartOffset: [ %+6.1f %6.1f %6.1f ]\n", ray.m_StartOffset.x, ray.m_StartOffset.y, ray.m_StartOffset.z);
					DevMsg("    m_Extents:     [ %+6.1f %6.1f %6.1f ]\n", ray.m_Extents.x,     ray.m_Extents.y,     ray.m_Extents.z);
					
					DevMsg("  actually using this ray:\n");
					DevMsg("    m_Start:       [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_Start.x,       ray_alt.m_Start.y,       ray_alt.m_Start.z);
					DevMsg("    m_Delta:       [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_Delta.x,       ray_alt.m_Delta.y,       ray_alt.m_Delta.z);
					DevMsg("    m_StartOffset: [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_StartOffset.x, ray_alt.m_StartOffset.y, ray_alt.m_StartOffset.z);
					DevMsg("    m_Extents:     [ %+6.1f %6.1f %6.1f ]\n", ray_alt.m_Extents.x,     ray_alt.m_Extents.y,     ray_alt.m_Extents.z);
					
					NDebugOverlay::Box(*the_pos, vecMins, vecMaxs, 0xff, 0xff, 0xff, 0x40, 10.0f);
				}
				
				DETOUR_MEMBER_CALL(ray_alt, fMask, pTraceFilter, pTrace);
				return;
			}
		}

		if (stuck_player != nullptr) {
			DETOUR_MEMBER_CALL(ray, fMask, pTraceFilter, pTrace);

			//Hit world entity, teleporting player away if possible
			if (pTrace->startsolid && (pTrace->m_pEnt == nullptr || ENTINDEX(pTrace->m_pEnt) == 0)) {
				float flPlayerHeight = ray.m_Extents.z * 2.0f;
				float flExtraHeight = 10;
				Ray_t ray_alt = ray;
				static Vector vTest[] =
				{
					Vector( 40, 40, flExtraHeight ),
					Vector( -40, -40, flExtraHeight ),
					Vector( -40, 40, flExtraHeight ),
					Vector( 40, -40, flExtraHeight ),
					Vector( 0, 0, flPlayerHeight + flExtraHeight ),
					Vector( 0, 0, -flPlayerHeight - flExtraHeight )
				};
				for ( int i=0; i<ARRAYSIZE( vTest ); ++i )
				{
					Vector vTestPos = stuck_player->GetAbsOrigin() + vTest[i];
					ray_alt.m_Start = ray.m_Start + vTest[i];
					
					trace_t trace_alt;
					DETOUR_MEMBER_CALL(ray_alt, fMask, pTraceFilter, &trace_alt);

					if ( !(trace_alt.startsolid && (trace_alt.m_pEnt == nullptr || ENTINDEX(trace_alt.m_pEnt) == 0)) )
					{
						// The real offset of abs origin of CMoveData is 4 bytes away
						Vector *move_ptr = (Vector *)((uintptr_t)(&stuck_player_move->GetAbsOrigin())+4);

						*move_ptr += vTest[i];

						*pTrace = trace_alt;
						return;
					}
					else
					{

					}
				}
			}

			stuck_player = nullptr;
			if (isbotmvm)
				TFGameRules()->Set_m_bPlayingMannVsMachine(false);
			return;
		}

		DETOUR_MEMBER_CALL(ray, fMask, pTraceFilter, pTrace);

		if (isbotmvm)
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Bot:IsSpaceToSpawnHere_Scale")
		{
			// If bots spawn cant fit a miniboss, its more of a map issue
			// MOD_ADD_DETOUR_MEMBER(CTFBotSpawner_Spawn, "CTFBotSpawner::Spawn");
			
			MOD_ADD_DETOUR_STATIC(TeleportNearVictim, "TeleportNearVictim");
			
			MOD_ADD_DETOUR_STATIC(IsSpaceToSpawnHere, "IsSpaceToSpawnHere");

			MOD_ADD_DETOUR_MEMBER(CTFGameMovement_CheckStuck, "CTFGameMovement::CheckStuck");

			// Fix tiny spell being too suicidal, also restore to the original scale
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnRemoveHalloweenTiny, "CTFPlayerShared::OnRemoveHalloweenTiny");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_OnAddHalloweenTiny, "CTFPlayerShared::OnAddHalloweenTiny");
			
			MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay, "IEngineTrace::TraceRay");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_bot_isspacetospawnhere_scale", "0", FCVAR_NOTIFY,
		"Mod: make IsSpaceToSpawnHere take bots' model scale into account",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
