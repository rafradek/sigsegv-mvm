#include "mod.h"
#include "stub/tf_shareddefs.h"
#include "stub/tfweaponbase.h"
#include "util/scope.h"
#include "stub/entities.h"
#include "util/rtti.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/clientmsg.h"

namespace Mod::Perf::Medigun_Shield_Damage_Interval
{
	RefCount rc_ILocomotion_TraceHull;
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& inputInfo)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		if (inputInfo.GetDamageCustom() == TF_DMG_CUSTOM_PLASMA) {
			auto medigun = rtti_cast<CWeaponMedigun *>(inputInfo.GetWeapon());
			
			if (medigun != nullptr) {
				extern ConVar cvar_enable;
				int interval = Max(cvar_enable.GetInt(), 1);
				
				if (interval > 1) {
					int victim_entindex = ENTINDEX(ent);
					
					if ((victim_entindex % interval) == (gpGlobals->tickcount % interval)) {
						CTakeDamageInfo newInfo = inputInfo;
						newInfo.ScaleDamage(interval);
						
						return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(newInfo);
					} else {
						return 0;
					}
				}
			}
		}
		
		
		return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(inputInfo);
	}
	INextBot *bot_trace;
	DETOUR_DECL_MEMBER(CBaseEntity *, PathFollower_FindBlocker, INextBot *bot)
	{
		CBaseEntity* result;
		bot_trace = bot;
		result = DETOUR_MEMBER_CALL(PathFollower_FindBlocker)(bot);
		bot_trace = nullptr;
		return result;
	}

	DETOUR_DECL_MEMBER(Vector, PathFollower_Avoid, INextBot *bot, const Vector &goalPos, const Vector &forward, const Vector &left )
	{
		Vector result;
		bot_trace = bot;
		result = DETOUR_MEMBER_CALL(PathFollower_Avoid)(bot, goalPos, forward, left);
		bot_trace = nullptr;
		return result;
	}

	DETOUR_DECL_MEMBER(bool, PathFollower_Climbing, INextBot *bot, const void *goal, const Vector &forward, const Vector &right, float goalRange)
	{
		bool result;
		bot_trace = bot;
		result = DETOUR_MEMBER_CALL(PathFollower_Climbing)(bot, goal, forward, right, goalRange);
		bot_trace = nullptr;
		return result;
	}

	DETOUR_DECL_MEMBER(bool, PathFollower_Climbing0, INextBot *bot, const void *goal, const Vector &forward, const Vector &right, float goalRange)
	{
		bool result;
		bot_trace = bot;
		result = DETOUR_MEMBER_CALL(PathFollower_Climbing0)(bot, goal, forward, right, goalRange);
		bot_trace = nullptr;
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, ILocomotion_TraceHull, const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, ITraceFilter *filter, CGameTrace *trace)
	{
		// if (bot_trace) {
		// DevMsg("DetourStart\n");
		// DevMsg("DetourGetBot\n");
		// auto bot = rtti_cast<CTFBot *>(bot_trace);
		// DevMsg("DetourBotNullptrCheck\n");
		// if (bot != nullptr) {
		// 	DevMsg("Tracehull\n");
		// 	ClientMsgAll("ILocomotion::TraceHull(bot #%d)\n",
		// 		ENTINDEX(bot));
				
		// 	DevMsg("Last\n");
	//			"  start [ %+7.1f %+7.1f %+7.1f ]\n"
	//			"  end   [ %+7.1f %+7.1f %+7.1f ]\n"
	//			"  mins  [ %+7.1f %+7.1f %+7.1f ]\n"
	//			"  maxs  [ %+7.1f %+7.1f %+7.1f ]\n",
	//			start.x, start.y, start.z,
	//			end.x,   end.y,   end.z,
	//			mins.x,  mins.y,  mins.z,
	//			maxs.x,  maxs.y,  maxs.z);
		//}
		
		//NDebugOverlay::Clear();
		
	//	NDebugOverlay::Cross3D(start, 2.0f, 0xff, 0x00, 0x00, false, 0.1f);
	//	NDebugOverlay::EntityTextAtPosition(start, 1, "start", 0.1f, 0xff, 0x00, 0x00, 0xff);
		
	//	NDebugOverlay::Cross3D(end, 2.0f, 0xff, 0x00, 0x00, false, 0.1f);
	//	NDebugOverlay::EntityTextAtPosition(end, 1, "end", 0.1f, 0xff, 0x00, 0x00, 0xff);
		//}
		//DevMsg("Scope\n");
		SCOPED_INCREMENT(rc_ILocomotion_TraceHull);
		DETOUR_MEMBER_CALL(ILocomotion_TraceHull)(start, end, mins, maxs, mask, filter, trace);
	}
	
	
	ConVar cvar_override_trace("sig_debug_medigunshield_obstruction_override_trace", "0", FCVAR_NOTIFY,
		"Override NextBotTraversableTraceFilter::ShouldHitEntity");
	DETOUR_DECL_MEMBER(bool, NextBotTraversableTraceFilter_ShouldHitEntity, IHandleEntity *pEntity, int contentsMask)
	{
		auto result = DETOUR_MEMBER_CALL(NextBotTraversableTraceFilter_ShouldHitEntity)(pEntity, contentsMask);
		//ClientMsgAll ("ShouldHitEntity %d %d\n", rc_ILocomotion_TraceHull, ENTINDEX());
		
		if (result && cvar_override_trace.GetBool()) {
			CBaseEntity *ent = EntityFromEntityHandle(pEntity);
			if (ENTINDEX(ent) > 64) {
				auto shield = rtti_cast<CTFMedigunShield *>(ent);
				if (shield != nullptr) {
					/*ClientMsgAll("  NextBotTraversableTraceFilter::ShouldHitEntity: %5s for CTFMedigunShield #%d on team %d\n",
						(result ? "TRUE" : "FALSE"),
						ENTINDEX(shield),
						shield->GetTeamNumber());*/
						//ClientMsgAll("    [!!] overriding ShouldHitEntity result to FALSE\n");
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
		auto result = DETOUR_MEMBER_CALL(CTFBotLocomotion_IsEntityTraversable)(ent, ttype);
		if (!result && cvar_override_traversable.GetBool() && ENTINDEX(ent) > 64) {
			auto shield = rtti_cast<CTFMedigunShield *>(ent);
			if (shield != nullptr) {
				auto loco = reinterpret_cast<ILocomotion *>(this);
				auto bot = rtti_cast<CTFBot *>(loco->GetBot());
				if (bot != nullptr) {
					/*ClientMsgAll("CTFBotLocomotion::IsEntityTraversable(bot #%d): %5s for CTFMedigunShield #%d on team %d\n",
						ENTINDEX(bot),
						(result ? "TRUE" : "FALSE"),
						ENTINDEX(shield),
						shield->GetTeamNumber());*/
					
						return true;
					
				//	if (shield->GetTeamNumber() == bot->GetTeamNumber()) {
				//		return true;
				//	}
				}
			}
		}
		
		return result;
	}
	ConVar cvar_heal_rage_from_upgrades_ratio("sig_heal_rage_from_upgrades_ratio", "1", FCVAR_NOTIFY,
		"Heal rage from upgrades ratio");

	ConVar cvar_heal_rage_ratio("sig_heal_rage_ratio", "1", FCVAR_NOTIFY,
		"Heal rage ratio");
	RefCount rc_HandleRageGain;
	DETOUR_DECL_STATIC(void, HandleRageGain, CTFPlayer *pPlayer, unsigned int iRequiredBuffFlags, float flDamage, float fInverseRageGainScale)
	{
		SCOPED_INCREMENT_IF(rc_HandleRageGain, (0x10 /*kRageBuffFlag_OnHeal*/ & iRequiredBuffFlags) && !pPlayer->IsBot() && pPlayer->IsPlayerClass( TF_CLASS_MEDIC ) && 
			(cvar_heal_rage_from_upgrades_ratio.GetFloat() != 1.0f || cvar_heal_rage_ratio.GetFloat() != -1.0f));
		DETOUR_STATIC_CALL(HandleRageGain)(pPlayer, iRequiredBuffFlags, flDamage, fInverseRageGainScale);
	}

	DETOUR_DECL_MEMBER(void, CTFPlayerShared_ModifyRage, float delta)
	{
		if (rc_HandleRageGain > 0) {
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			CTFPlayer *player = shared->GetOuter();
			
			float healrate = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( player, healrate, mult_medigun_healrate );
			float bonus = Max(1.0f, healrate - 0.5f);

			int iHealingMastery = 0;
			CALL_ATTRIB_HOOK_INT_ON_OTHER( player, iHealingMastery, healing_mastery );
			if ( iHealingMastery )
			{
				bonus *= RemapValClamped( (float)iHealingMastery, 1.f, 4.f, 1.25f, 2.0f );
			}

			delta = (delta / bonus) * (1.0f + ((bonus - 1.0f) * cvar_heal_rage_from_upgrades_ratio.GetFloat())) * cvar_heal_rage_ratio.GetFloat();
		}
		
		DETOUR_MEMBER_CALL(CTFPlayerShared_ModifyRage)(delta);
	}

	ConVar cvar_ubersaw_uber_decrease("sig_ubersaw_uber_decrease", "0", FCVAR_NOTIFY,
		"Ubersaw uber on hit decrease if rage scale attribute is present");
	
	RefCount rc_CTFWeaponBase_ApplyOnHitAttributes;
	CBaseEntity *victim;
	DETOUR_DECL_MEMBER(void, CTFWeaponBase_ApplyOnHitAttributes, CBaseEntity *ent, CTFPlayer *player, const CTakeDamageInfo& info)
	{
		SCOPED_INCREMENT_IF(rc_CTFWeaponBase_ApplyOnHitAttributes, cvar_ubersaw_uber_decrease.GetBool());
		victim = ent;
		DETOUR_MEMBER_CALL(CTFWeaponBase_ApplyOnHitAttributes)(ent, player, info);
		victim = nullptr;
	}

	DETOUR_DECL_MEMBER(void, CWeaponMedigun_AddCharge, float val)
	{
		if (rc_CTFWeaponBase_ApplyOnHitAttributes && victim->IsPlayer()) {
			float ragescale = 1.0f;
			CALL_ATTRIB_HOOK_FLOAT_ON_OTHER( victim, ragescale, rage_giving_scale );
			val *= ragescale;
		}
		DETOUR_MEMBER_CALL(CWeaponMedigun_AddCharge)(val);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("Perf:Medigun_Shield_Damage_Interval")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage, "CBaseEntity::TakeDamage");
			//MOD_ADD_DETOUR_MEMBER(ILocomotion_TraceHull,                         "ILocomotion::TraceHull");
			MOD_ADD_DETOUR_MEMBER(NextBotTraversableTraceFilter_ShouldHitEntity, "NextBotTraversableTraceFilter::ShouldHitEntity");
			MOD_ADD_DETOUR_MEMBER(CTFBotLocomotion_IsEntityTraversable,          "CTFBotLocomotion::IsEntityTraversable");
			MOD_ADD_DETOUR_STATIC(HandleRageGain,          "HandleRageGain");
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_ModifyRage,          "CTFPlayerShared::ModifyRage");
			MOD_ADD_DETOUR_MEMBER(CTFWeaponBase_ApplyOnHitAttributes,          "CTFWeaponBase::ApplyOnHitAttributes");
			MOD_ADD_DETOUR_MEMBER(CWeaponMedigun_AddCharge,          "CWeaponMedigun::AddCharge");

		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_perf_medigun_shield_damage_interval", "0", FCVAR_NOTIFY,
		"Mod: change the medigun shield damage interval to values greater than every single tick",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
