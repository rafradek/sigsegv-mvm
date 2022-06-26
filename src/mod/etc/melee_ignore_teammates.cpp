#include "mod.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"
#include "util/iterate.h"
#include "stub/gamerules.h"


namespace Mod::Etc::Melee_Ignore_Teammates
{
	CTFWeaponBaseMelee *melee = nullptr;
	CTFPlayer *attacker = nullptr;
	bool enemy_attack_pass = false;

	RefCount rc_CTFWeaponBaseMelee_DoSwingTraceInternal;
	RefCount rc_CTFWeaponBaseMelee_Smack;

	DETOUR_DECL_MEMBER(void, CTFWeaponBaseMelee_Smack )
	{
		SCOPED_INCREMENT(rc_CTFWeaponBaseMelee_Smack);
 		DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_Smack)();
	}
	DETOUR_DECL_MEMBER(bool, CTFWeaponBaseMelee_DoSwingTraceInternal, CGameTrace& tr, bool cleave_attack, CUtlVector<CGameTrace> *traces)
	{
		auto weapon = reinterpret_cast<CTFWeaponBaseMelee *>(this);
		
		auto team = weapon->GetOwner()->GetTeamNumber();

		bool result;
		//DevMsg("swing time %f \n",*(float *)(*((char *)weapon + 0x7dc) + /*offsets*/0x72c + 0)  );
				                    //                 *(undefined4 *)
                                    //(*(int *)(this + 0x7dc) + 0x70c + *(int *)(this + 0x6a8) * 0x40)
		
		// Make cleave attack target NPC such as tanks
		std::vector<CBaseEntity *> altered_flags;
		if (cleave_attack) {
			ForEachEntity([&](CBaseEntity *ent){
				int flags = ent->m_fFlags;
				if ((flags & FL_NPC || ent->IsCombatCharacter()) && !(flags & FL_OBJECT || flags & FL_CLIENT)) {
					ent->m_fFlags |= FL_OBJECT;
					altered_flags.push_back(ent);
				}
			});
		}

		bool revertMannVsMachine = false;
		if (team == TF_TEAM_BLUE && TFGameRules()->IsMannVsMachineMode()) {
			int whip = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, whip, speed_buff_ally);
			int friendly = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER(weapon, friendly, allow_friendly_fire);
			if (whip > 0 || friendly != 0) {
				TFGameRules()->Set_m_bPlayingMannVsMachine(false);
				revertMannVsMachine = true;
			}
		}
		if (!cleave_attack/* && team != TF_TEAM_BLUE*/) {
			attacker = weapon->GetTFPlayerOwner();
			//is_whip = (CAttributeManager::AttribHookValue<int>(0, "speed_buff_ally", weapon) > 0);
			SCOPED_INCREMENT(rc_CTFWeaponBaseMelee_DoSwingTraceInternal);
			result = DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_DoSwingTraceInternal)(tr, cleave_attack, traces);
			if (tr.m_pEnt != nullptr && tr.m_pEnt->GetTeamNumber() == attacker->GetTeamNumber()) {
				CGameTrace newtrace;

				enemy_attack_pass = true;
				bool newresult = DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_DoSwingTraceInternal)(newtrace, cleave_attack, traces);
				enemy_attack_pass = false;

				if(newtrace.m_pEnt != nullptr && newtrace.m_pEnt->GetTeamNumber() != attacker->GetTeamNumber()) {
					tr = newtrace;
				}
			}
			attacker = nullptr;
			//is_whip = false;
		}
		else {
			result = DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_DoSwingTraceInternal)(tr, cleave_attack, traces);
		}

		if (cleave_attack) {
			for (auto ent : altered_flags) {
				ent->m_fFlags &= ~FL_OBJECT;
			}
		}
		if (revertMannVsMachine) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
		}
		//weapon->GetOwner()->SetTeamNumber(team);

		/*bool result;
		if (!cleave_attack) {
			melee   = weapon;
			owner   = (weapon != nullptr ? weapon->GetTFPlayerOwner() : nullptr);
			is_whip = (CAttributeManager::AttribHookValue<int>(0, "speed_buff_ally", weapon) > 0);
			
			
			result = DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_DoSwingTraceInternal)(tr, cleave_attack, traces);
			
			melee   = nullptr;
			owner   = nullptr;
			is_whip = false;
		} else {
			result = DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_DoSwingTraceInternal)(tr, cleave_attack, traces);
		}*/
		
		return result;
	}

	DETOUR_DECL_MEMBER(bool, CTFWeaponBaseMelee_OnSwingHit, trace_t &trace )
	{
		if (trace.m_pEnt == nullptr)
			return false;
 		return DETOUR_MEMBER_CALL(CTFWeaponBaseMelee_OnSwingHit)(trace);
	}

	DETOUR_DECL_STATIC(int, CAttributeManager_AttribHookValue_int, int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
		if (rc_CTFWeaponBaseMelee_Smack > 0 && V_stricmp(attr, "melee_cleave_attack") == 0) {
			return DETOUR_STATIC_CALL(CAttributeManager_AttribHookValue_int)(value, "projectile_penetration", ent, vec, b1);
		}
		
		return DETOUR_STATIC_CALL(CAttributeManager_AttribHookValue_int)(value, attr, ent, vec, b1);
	}

	DETOUR_DECL_MEMBER(bool, CTraceFilterIgnoreFriendlyCombatItems_ShouldHitEntity, IHandleEntity *pServerEntity, int contentsMask)
	{
		if (rc_CTFWeaponBaseMelee_DoSwingTraceInternal && enemy_attack_pass) {
			CBaseEntity *entity = EntityFromEntityHandle(pServerEntity);
			if (entity->IsPlayer() && entity->GetTeamNumber() == attacker->GetTeamNumber())
				return false;
		}
		return DETOUR_MEMBER_CALL(CTraceFilterIgnoreFriendlyCombatItems_ShouldHitEntity)(pServerEntity, contentsMask);
			
	}


	/*RefCount rc_FindHullIntersection;
	DETOUR_DECL_STATIC(void, FindHullIntersection, const Vector& vecSrc, trace_t& tr, const Vector& mins, const Vector& maxs, CBaseEntity *pEntity)
	{
		SCOPED_INCREMENT(rc_FindHullIntersection);
		DETOUR_STATIC_CALL(FindHullIntersection)(vecSrc, tr, mins, maxs, pEntity);
	}
	
	
	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
		if (rc_CTFWeaponBaseMelee_DoSwingTraceInternal > 0) {
			// doing fallback stuff
			if (rc_FindHullIntersection > 0) {
				// if it's a CTraceFilterSimple and we're called from within FindHullIntersection, then we're doing fallback stuff
				// ...
			}
			
			// if it's a CTraceFilterIgnorePlayers, then we're doing the Homewrecker thing
			// and we should leave it alone
			
			// if it's a CTraceFilterIgnoreTeammates, then we're doing the regular trace for MvM robots
			// ...
			
			// if it's a CTraceFilterIgnoreFriendlyCombatItems, then we're doing the regular trace for non-MvM-robots
			// ...
			
			#warning TODO
			#warning TODO
			#warning TODO
		}
		
		DETOUR_MEMBER_CALL(IEngineTrace_TraceRay)(ray, fMask, pTraceFilter, pTrace);
	}
	*/
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Melee_Ignore_Teammates")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_DoSwingTraceInternal, "CTFWeaponBaseMelee::DoSwingTraceInternal");
			MOD_ADD_DETOUR_MEMBER(CTraceFilterIgnoreFriendlyCombatItems_ShouldHitEntity, "CTraceFilterIgnoreFriendlyCombatItems::ShouldHitEntity");
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_int, "CAttributeManager::AttribHookValue<int>");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_Smack, "CTFWeaponBaseMelee::Smack");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_OnSwingHit, "CTFWeaponBaseMelee::OnSwingHit");
			//MOD_ADD_DETOUR_MEMBER(CTFWeaponBaseMelee_GetSmackTime, "CTFWeaponBaseMelee::GetSmackTime");
			
			//MOD_ADD_DETOUR_STATIC(FindHullIntersection, "FindHullIntersection");
			
			//MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay, "IEngineTrace::TraceRay");
		}
	};
	CMod s_Mod;
	
	
ConVar cvar_enable("sig_etc_melee_ignore_teammates", "0", FCVAR_NOTIFY,
		"Mod: allow melee traces to pass through teammates (for anyone, not just MvM blu team players)",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
