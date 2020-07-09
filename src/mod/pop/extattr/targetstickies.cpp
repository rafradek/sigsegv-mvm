#include "mod.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/projectiles.h"
#include "util/scope.h"

#define TARGET_STICKIES_MOD
#if defined TARGET_STICKIES_MOD

namespace Mod::Pop::ExtAttr::TargetStickies
{
	class CTFBotChaseStickies : public Action<CTFBot>
	{
	public:
		CTFBotChaseStickies()
		{
			DevMsg("CTFBotChaseStickies::CTFBotChaseStickies\n");
			
			// TODO
		}
		
		virtual ~CTFBotChaseStickies()
		{
			DevMsg("CTFBotChaseStickies::~CTFBotChaseStickies\n");
			
			// TODO
		}
		
		
		virtual QueryResponse ShouldHurry(const INextBot *nextbot) const override
		{
			return QueryResponse::YES;
		}
		
		virtual QueryResponse ShouldRetreat(const INextBot *nextbot) const override
		{
			return QueryResponse::NO;
		}
		
		virtual QueryResponse ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const override
		{
			/* defer to CTFBotMainAction::ShouldAttack (for stuff like not-attacking-while-in-spawn) */
			return QueryResponse::YES;
		}
		
		virtual const CKnownEntity *SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const override
		{
			// TODO: prefer closer stickies
			// TODO: prefer critical stickies
			// TODO: prefer the sticky we're chasing
			
			return nullptr;
		}
		
		
		virtual const char *GetName() const override { return "ChaseStickies"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			DevMsg("CTFBotChaseStickies::OnStart(#%d)\n", ENTINDEX(actor));
			
			this->m_PathFollower.SetMinLookAheadDistance(actor->GetDesiredPathLookAheadRange());
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			DevMsg("CTFBotChaseStickies::Update(#%d)\n", ENTINDEX(actor));
			
			bool should_retarget = this->m_itChoseTarget.IsGreaterThen(5.0f);
			bool should_repath   = this->m_ctRecomputePath.IsElapsed();
			
			if (this->m_hTarget == nullptr) {
				should_retarget = true;
			}
			
			if (should_retarget) {
				DevMsg("  retargeting\n");
				
				CBaseEntity *new_target = this->ChooseTarget();
				if (new_target == nullptr) {
					DevMsg("  no target possible!\n");
					
					//actor->Taunt(1, 0x5c);
					
					
					// TODO: taunt and suicide
					return ActionResult<CTFBot>::Continue();
				}
				this->m_hTarget = new_target;
				
				DevMsg("  got new target, will repath\n");
				this->m_itChoseTarget.Start();
				should_repath = true;
			}
			
			INextBot *nextbot = rtti_cast<INextBot *>(actor);
			
			if (should_repath) {
				DevMsg("  repathing\n");
				this->m_ctRecomputePath.Start(RandomFloat(0.5f, 1.0f));
				
				Vector dest = this->m_hTarget->GetAbsOrigin();
				DevMsg("  target @ %f %f %f\n", dest.x, dest.y, dest.z);
				
				CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
				bool success = this->m_PathFollower.Compute<CTFBotPathCost>(nextbot, dest, cost_func, 0.0f, true);
				
				DevMsg("  path result: %s\n", (success ? "SUCCESS" : "FAIL"));
				
				// TODO: handle failed path
			}
			
			IVision *vision = actor->GetVisionInterface();
			
			if (vision->IsAbleToSee(this->m_hTarget, IVision::FieldOfViewCheckType::DISREGARD_FOV, nullptr)) {
				DevMsg("  able to see target; will look at it\n");
				actor->GetBodyInterface()->AimHeadTowards(this->m_hTarget, IBody::LookAtPriorityType::OVERRIDE_ALL, 0.1f, nullptr, "Looking at target sticky");
			} else {
				DevMsg("  unable to see target\n");
			}
			
			// if m_hTarget is valid and is a sticky, chase it
			// (only repath at a limited interval though)
			
			// otherwise, choose a new target
			// - maybe pick random stickies
			// - maybe prioritize known stickies (IVision::CollectKnownEntities)
			// - maybe prioritize close stickies
			// - maybe prioritize crit stickies
			
			// after chasing one sticky for a certain period of time, timeout and chose another
			
			// might want to force bot to look toward the target once within a certain range of it
			
			// when no stickies remain:
			// - first taunt
			// - then suicide
			
			Vector actor_to_goal = this->m_hTarget->GetAbsOrigin() - actor->GetAbsOrigin();
			
			if (actor_to_goal.IsLengthGreaterThan(500.0f) || !vision->IsAbleToSee(this->m_hTarget, IVision::FieldOfViewCheckType::USE_FOV, nullptr)) {
				this->m_PathFollower.Update(nextbot);
			}
			
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override
		{
			DevMsg("CTFBotChaseStickies::OnEnd(#%d)\n", ENTINDEX(actor));
			
			// TODO
		}
		
	private:
		CBaseEntity *ChooseTarget()
		{
			CUtlVector<CBaseProjectile *> stickies;
			
			for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
				auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
				assert(proj != nullptr);
				
				if (strcmp(proj->GetClassname(), "tf_projectile_pipe_remote") == 0) {
					stickies.AddToTail(proj);
				}
			}
			
			if (stickies.Count() == 0) {
				return nullptr;
			}
			
			return stickies[RandomInt(0, stickies.Count() - 1)];
		}
		
		CHandle<CBaseEntity> m_hTarget;
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
		IntervalTimer m_itChoseTarget;
	};
	
	
#if 0
	constexpr int MISSION_PUSHSTICKIES = 7;
	
	
	RefCount rc_CMissionPopulator_Parse;
	DETOUR_DECL_MEMBER(bool, CMissionPopulator_Parse, KeyValues *kv)
	{
		DevMsg("CMissionPopulator::Parse: this = %08x\n", (uintptr_t)this);
		
		SCOPED_INCREMENT(rc_CMissionPopulator_Parse);
		return DETOUR_MEMBER_CALL(CMissionPopulator_Parse)(kv);
	}
	
	DETOUR_DECL_MEMBER(bool, CSpawnLocation_Parse, KeyValues *kv)
	{
		if (rc_CMissionPopulator_Parse > 0) {
			DevMsg("CSpawnLocation::Parse: this = %08x\n", (uintptr_t)this);
			DevMsg("CSpawnLocation::Parse: pop  = %08x\n", (uintptr_t)this - offsetof(CMissionPopulator, m_Where));
			DevMsg("CSpawnLocation::Parse: obj  = %08x\n", (uintptr_t)this - offsetof(CMissionPopulator, m_Where) + offsetof(CMissionPopulator, m_Objective));
			
			if (V_stricmp(kv->GetName(), "Mod_Objective") == 0 &&
				V_stricmp(kv->GetString(), "PushStickies") == 0) {
				DevMsg("Found 'Objective PushStickies'.\n");
				
				int *p_m_Objective = (int *)((uintptr_t)this - offsetof(CMissionPopulator, m_Where) + offsetof(CMissionPopulator, m_Objective));
				
				*p_m_Objective = MISSION_PUSHSTICKIES;
				return true;
			}
		}
		
		return DETOUR_MEMBER_CALL(CSpawnLocation_Parse)(kv);
	}
	
	
	DETOUR_DECL_MEMBER(void, CMissionPopulator_Update)
	{
		DETOUR_MEMBER_CALL(CMissionPopulator_Update)();
		
		CMissionPopulator *realthis = reinterpret_cast<CMissionPopulator *>(this);
		
		int objective = realthis->m_Objective;
		if (objective == MISSION_PUSHSTICKIES) {
			realthis->UpdateMission(objective);
		}
	}
#endif
	
	
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotScenarioMonitor_DesiredScenarioAndClassAction, CTFBot *actor)
	{
	//	DevMsg("CTFBotScenarioMonitor::DesiredScenarioAndClassAction\n");
		
#if 0
		if (actor->GetMission() == MISSION_PUSHSTICKIES) {
			return new CTFBotMissionPushStickies();
		}
#endif
		
		if (actor->ExtAttr()[CTFBot::ExtendedAttr::TARGET_STICKIES]) {
			return new CTFBotChaseStickies();
		}
		
		return DETOUR_MEMBER_CALL(CTFBotScenarioMonitor_DesiredScenarioAndClassAction)(actor);
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		IVision *vision = reinterpret_cast<IVision *>(this);
		CTFBot *actor = rtti_cast<CTFBot *>(vision->GetBot());
		
	//	DevMsg("CTFBotVision::IsIgnored INextBot %08x CTFBot %08x\n", (uintptr_t)vision->GetBot(), (uintptr_t)actor);
	//	DevMsg("CTFBotVision::IsIgnored actor %08x ent %08x\n", (uintptr_t)actor, (uintptr_t)ent);
		
		if (ent != nullptr && actor != nullptr) {
	//		DevMsg("CTFBotVision::IsIgnored classname \"%s\" mission %d\n", ent->GetClassname(), actor->GetMission());
			bool is_sticky = (strcmp(ent->GetClassname(), "tf_projectile_pipe_remote") == 0);
			
			if (actor->ExtAttr()[CTFBot::ExtendedAttr::TARGET_STICKIES]) {
				/* ignore everything except stickies */
				return !is_sticky;
			} else {
				/* ignore stickies, passthru for all else */
				if (is_sticky) {
					return true;
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(CTFBotVision_IsIgnored)(ent);
	}
	
//	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsVisibleEntityNoticed, CBaseEntity *ent)
//	{
//		
//	}
	
	
	RefCount rc_CTFBotVision_CollectPotentiallyVisibleEntities;
	DETOUR_DECL_MEMBER(void, CTFBotVision_CollectPotentiallyVisibleEntities, CUtlVector<CBaseEntity *> *ents)
	{
		SCOPED_INCREMENT(rc_CTFBotVision_CollectPotentiallyVisibleEntities);
		DETOUR_MEMBER_CALL(CTFBotVision_CollectPotentiallyVisibleEntities)(ents);
		
		for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
			auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
			assert(proj != nullptr);
			
			if (strcmp(proj->GetClassname(), "tf_projectile_pipe_remote") == 0) {
				ents->AddToTail(proj);
			}
		}
	}
	
	
	// TODO: make CTFBotChaseStickies be reference-counted,
	// and prevent mod unloading if it has >=1 active reference
	// (to prevent crashes)
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Pop:ExtAttr:TargetStickies")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotScenarioMonitor_DesiredScenarioAndClassAction, "CTFBotScenarioMonitor::DesiredScenarioAndClassAction");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsIgnored,              "CTFBotVision::IsIgnored");
//			MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsVisibleEntityNoticed, "CTFBotVision::IsVisibleEntityNoticed");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_CollectPotentiallyVisibleEntities, "CTFBotVision::CollectPotentiallyVisibleEntities");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_pop_extattr_targetstickies", "0", FCVAR_NOTIFY,
		"Extended bot attr: TargetStickies",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif
