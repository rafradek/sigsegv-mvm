#include "mod.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/tfbot.h"
#include "stub/objects.h"
#include "stub/tfentities.h"
#include "util/rtti.h"
#include "util/scope.h"
#include "stub/gamerules.h"
#include "stub/econ.h"

#include "stub/tfbot_behavior.h"


//#define TRACE_ENABLE 1
#define TRACE_TERSE  0
#include "util/trace.h"


namespace Mod::AI::EngieBot_Dispensers
{
	bool suppress_speak = false;
	
	
	// CTFPlayer::PlayTauntSceneFromItem(CEconItemView *)
	
	
	bool DoRelaxoTaunt(CTFPlayer *player)
	{
		constexpr int DEFIDX_TAUNT_RELAXO = 1115;
		
		// allocate >= 0xac bytes
		// call CEconItemView default ctor
		
		// call CEconItemView::Init(int, int, int, uint)
		// - int #1 -> offset 0x04 m_iItemDefinitionIndex
		// - int #2 -> offset 0x08 m_iEntityQuality
		// - int #3 -> offset 0x0c m_iEntityLevel
		// - int #4 -> offset 0x20 m_iAccountID
		
		
		
		// ? CItemGeneration::GenerateItemFromDefIndex
		
		
		
		return false;
	}
	
	
	class CTFBotMvMEngineerBuildSGDispenser : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMvMEngineerBuildSGDispenser(CTFBotHintSentrygun *hint) :
			m_hHint(hint)
		{
			TRACE("[this: %08x] [hint: #%d %08x]", (uintptr_t)this, ENTINDEX(hint), (uintptr_t)hint);
		}
		
		virtual const char *GetName() const override { return "MvMEngineerBuildSGDispenser"; }
		
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			TRACE("[this: %08x] [actor: #%d]", (uintptr_t)this, ENTINDEX(actor));
			
			actor->StartBuildingObjectOfType(OBJ_DISPENSER, 0);
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			TRACE("[this: %08x] [actor: #%d]", (uintptr_t)this, ENTINDEX(actor));
			
			if (this->m_hHint == nullptr) {
				return ActionResult<CTFBot>::Done("No hint entity");
			}
			
			INextBot *nextbot = actor->MyNextBotPointer();
			
			float range_to_hint = nextbot->GetRangeTo(this->m_hHint->GetAbsOrigin());
			
			if (range_to_hint < 200.0f) {
				TRACE_MSG("range_to_hint < 200: crouching/aiming\n");
				actor->PressCrouchButton();
				actor->GetBodyInterface()->AimHeadTowards(this->m_hHint->GetAbsOrigin(),
					IBody::LookAtPriorityType::OVERRIDE_ALL, 0.1f, nullptr, "Placing dispenser");
				
				if (!this->m_bNearHint) {
					this->m_bNearHint = true;
					//TFGameRules()->VoiceCommand(actor, 1, 4);
					//actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_DISPENSERHERE);
				}
			} else {
				this->m_bNearHint = false;
			}
			
			if (range_to_hint > 25.0f) {
				TRACE_MSG("range_to_hint > 25: pathing\n");
				
				if (this->m_ctRecomputePath.IsElapsed()) {
					TRACE_MSG("recomputing path\n");
					
					this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
					
					CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
					this->m_PathFollower.Compute(nextbot, this->m_hHint->GetAbsOrigin(), cost_func, 0.0f, true);
				}
				
				this->m_PathFollower.Update(nextbot);
				if (!this->m_PathFollower.IsValid()) {
					return ActionResult<CTFBot>::Done("Path failed");
				}
				
				return ActionResult<CTFBot>::Continue();
			}
			
			TRACE_MSG("at hint: creating dispenser entity\n");
			
			CBaseEntity *ent = CreateEntityByName("obj_dispenser");
			if (ent == nullptr) {
				return ActionResult<CTFBot>::Done("Couldn't create entity");
			}
			
			// TODO: increment hint dword 0x370 (not important for mvm)
			
			auto dispenser = rtti_cast<CObjectDispenser *>(ent);
			dispenser->SetName(this->m_hHint->GetEntityName());
			dispenser->m_nDefaultUpgradeLevel = 2;
			dispenser->SetAbsOrigin(this->m_hHint->GetAbsOrigin());
			dispenser->SetAbsAngles(this->m_hHint->GetAbsAngles());
			DispatchSpawn(dispenser);
			
			dispenser->StartPlacement(actor);
			
			suppress_speak = true;
			dispenser->StartBuilding(actor);
			suppress_speak = false;
			
			this->m_hHint->SetOwnerEntity(dispenser);
			
			actor->SpeakConceptIfAllowed(MP_CONCEPT_BUILDING_OBJECT, "objtype:dispenser");
			
			return ActionResult<CTFBot>::Done("Built a dispenser");
		}
		
		
	private:
		CHandle<CTFBotHintSentrygun> m_hHint;
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
		bool m_bNearHint = false;
	};
	
	class CTFBotMvMEngineerBuildTEDispenser : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMvMEngineerBuildTEDispenser(CTFBotHintTeleporterExit *hint) :
			m_hHint(hint)
		{
			TRACE("[this: %08x] [hint: #%d %08x]", (uintptr_t)this, ENTINDEX(hint), (uintptr_t)hint);
		}
		
		virtual const char *GetName() const override { return "MvMEngineerBuildTEDispenser"; }
		
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			TRACE("[this: %08x] [actor: #%d]", (uintptr_t)this, ENTINDEX(actor));
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			TRACE("[this: %08x] [actor: #%d]", (uintptr_t)this, ENTINDEX(actor));
			
			if (this->m_hHint == nullptr) {
				return ActionResult<CTFBot>::Done("No hint entity");
			}
			
			INextBot *nextbot = actor->MyNextBotPointer();
			
			if (nextbot->IsRangeGreaterThan(this->m_hHint->GetAbsOrigin(), 25.0f)) {
				TRACE_MSG("range_to_hint > 25: pathing\n");
				
				if (this->m_ctRecomputePath.IsElapsed()) {
					TRACE_MSG("recomputing path\n");
					
					this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
					
					CTFBotPathCost cost_func(actor, FASTEST_ROUTE);
					this->m_PathFollower.Compute(nextbot, this->m_hHint->GetAbsOrigin(), cost_func, 0.0f, true);
				}
				
				this->m_PathFollower.Update(nextbot);
				if (!this->m_PathFollower.IsValid()) {
					return ActionResult<CTFBot>::Done("Path failed");
				}
				
				return ActionResult<CTFBot>::Continue();
			}
			
			TRACE_MSG("at hint: creating dispenser entity\n");
			
			CBaseEntity *ent = CreateEntityByName("obj_dispenser");
			if (ent == nullptr) {
				return ActionResult<CTFBot>::Done("Couldn't create entity");
			}
			
			auto dispenser = rtti_cast<CObjectDispenser *>(ent);
			dispenser->SetName(this->m_hHint->GetEntityName());
			dispenser->m_nDefaultUpgradeLevel = 2;
			dispenser->SetAbsOrigin(this->m_hHint->GetAbsOrigin());
			dispenser->SetAbsAngles(this->m_hHint->GetAbsAngles());
			dispenser->Spawn();
			
			dispenser->StartPlacement(actor);
			
			suppress_speak = true;
			dispenser->StartBuilding(actor);
			suppress_speak = false;
			
			this->m_hHint->SetOwnerEntity(dispenser);
			
			actor->SpeakConceptIfAllowed(MP_CONCEPT_BUILDING_OBJECT, "objtype:dispenser");
			
			return ActionResult<CTFBot>::Done("Built a dispenser");
		}
		
		
	private:
		CHandle<CTFBotHintTeleporterExit> m_hHint;
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
	};
	
	
	CTFBotHintSentrygun *hint_sg = nullptr;
	DETOUR_DECL_MEMBER(void, CTFBotMvMEngineerBuildSentryGun_ctor, CTFBotHintSentrygun *hint)
	{
		TRACE();
		
		hint_sg = hint;
		DETOUR_MEMBER_CALL(hint);
	}
	
	CTFBotHintTeleporterExit *hint_te = nullptr;
	DETOUR_DECL_MEMBER(void, CTFBotMvMEngineerBuildTeleportExit_ctor, CTFBotHintTeleporterExit *hint)
	{
		TRACE();
		
		hint_te = hint;
		DETOUR_MEMBER_CALL(hint);
	}
	
	std::unordered_map<Action<CTFBot> *, IHotplugAction<CTFBot> *> build_actions;

	DETOUR_DECL_MEMBER(void, CTFBotMvMEngineerIdle_dtor0)
	{
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		if (build_actions.find(action) != build_actions.end()) {
			delete build_actions[action];
			build_actions.erase(action);
		}
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CTFBotMvMEngineerIdle_dtor1)
	{
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		if (build_actions.find(action) != build_actions.end()) {
			delete build_actions[action];
			build_actions.erase(action);
		}
		DETOUR_MEMBER_CALL();
	}

	DETOUR_DECL_MEMBER(void, CTFBotMvMEngineerIdle_dtor2)
	{
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		if (build_actions.find(action) != build_actions.end()) {
			delete build_actions[action];
			build_actions.erase(action);
		}
		DETOUR_MEMBER_CALL();
	}

	RefCount rc_CTFBotMvMEngineerIdle_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMvMEngineerIdle_Update, CTFBot *actor, float dt)
	{
		SCOPED_INCREMENT(rc_CTFBotMvMEngineerIdle_Update);
		
		TRACE();
		
		// static IntervalTimer last_ask;
		// constexpr float ask_interval = 20.0f;
		// constexpr float ask_minwait  =  3.0f;
		// if (RandomFloat(0.0f, 1.0f) < (dt / ask_interval)) {
		// 	if (last_ask.GetElapsedTime() > ask_minwait) {
		// 		last_ask.Start();
				
		// 		switch (RandomInt(0, 2)) {
		// 		case 0:
		// 		case 1:
		// 			actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_TELEPORTERHERE);
		// 			break;
		// 		case 2:
		// 			actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_SENTRYHERE);
		// 			break;
		// 		}
		// 	}
		// }
		
		auto action = reinterpret_cast<Action<CTFBot> *>(this);

		if (build_actions.find(action) != build_actions.end()) {
			ActionResult<CTFBot> result = build_actions[action]->Update(actor, dt);
			if (result.transition != ActionTransition::DONE)
				return ActionResult<CTFBot>::Continue();
			else {
				delete build_actions[action];
				build_actions.erase(action);
			}
		}

		auto result = DETOUR_MEMBER_CALL(actor, dt);
		
		if (result.transition == ActionTransition::SUSPEND_FOR && result.action != nullptr) {
			if (strcmp(result.action->GetName(), "MvMEngineerBuildSentryGun") == 0 && hint_sg != nullptr) {

				actor->ExtAttr().Dump();
				if (actor != nullptr && !(actor->ExtAttr()[CTFBot::ExtendedAttr::BUILD_DISPENSER_TP]) && actor->ExtAttr()[CTFBot::ExtendedAttr::BUILD_DISPENSER_SG]) {
					DevMsg("  actor has ExtAttr::BUILD_DISPENSER\n");
					delete result.action;
					result.action = nullptr;
					result.transition = ActionTransition::CONTINUE;
					build_actions[action] = new CTFBotMvMEngineerBuildSGDispenser(hint_sg);
					hint_sg = nullptr;
				}

				
				
			} else if (strcmp(result.action->GetName(), "MvMEngineerBuildTeleportExit") == 0 && hint_te != nullptr) {

				actor->ExtAttr().Dump();
				if (actor != nullptr && actor->ExtAttr()[CTFBot::ExtendedAttr::BUILD_DISPENSER_TP]) {
					DevMsg("  actor has ExtAttr::BUILD_DISPENSER\n");
					actor->PressAltFireButton();
					delete result.action;
					result.action = nullptr;
					result.transition = ActionTransition::CONTINUE;
					build_actions[action] = new CTFBotMvMEngineerBuildTEDispenser(hint_te);
					hint_te = nullptr;
				}
			}
		}
		
		return result;
	}
	

	/* CTFBotMvMEngineerIdle::Update static_cast's the owner of the sentry hint
	 * to a CObjectSentrygun and calls GetTurretAngles, which doesn't exist for
	 * CObjectDispenser; this tweak should avoid the problem */
	VHOOK_DECL(const QAngle&, CObjectDispenser_GetAvailableMetal)
	{
		TRACE();
		
		if (rc_CTFBotMvMEngineerIdle_Update > 0) {
			TRACE_MSG("in CTFBotMvMEngineerIdle::Update");
			auto obj = reinterpret_cast<CBaseEntity *>(this);
			return obj->EyeAngles();
		}
		
		return VHOOK_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(bool, CTFPlayer_SpeakConceptIfAllowed, int iConcept, const char *modifiers, char *pszOutResponseChosen, size_t bufsize, IRecipientFilter *filter)
	{
		if (suppress_speak) {
			return false;
		}
		
		return DETOUR_MEMBER_CALL(iConcept, modifiers, pszOutResponseChosen, bufsize, filter);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFBotHintEngineerNest_HintTeleporterThink)
	{
		DETOUR_MEMBER_CALL();
		
		auto nest = reinterpret_cast<CTFBotHintEngineerNest *>(this);
		nest->m_bHasActiveTeleporter = true;
	}
	
	
	// MP_CONCEPT_PLAYER_SENTRYAHEAD
	// MP_CONCEPT_PLAYER_TELEPORTERHERE
	// MP_CONCEPT_PLAYER_DISPENSERHERE
	// MP_CONCEPT_PLAYER_SENTRYHERE
	// MP_CONCEPT_PLAYER_MOVEUP
	// MP_CONCEPT_DETONATED_OBJECT
	// MP_CONCEPT_KILLED_OBJECT
	// MP_CONCEPT_PLAYER_HELP
	// MP_CONCEPT_BUILDING_OBJECT
	// MP_CONCEPT_LOST_OBJECT
	// MP_CONCEPT_SPY_SAPPER
	// MP_CONCEPT_PICKUP_BUILDING
	// MP_CONCEPT_REDEPLOY_BUILDING
	// MP_CONCEPT_CARRYING_BUILDING
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:EngieBot_Dispensers")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerBuildSentryGun_ctor,    "CTFBotMvMEngineerBuildSentryGun::CTFBotMvMEngineerBuildSentryGun [C1]");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerBuildTeleportExit_ctor, "CTFBotMvMEngineerBuildTeleportExit::CTFBotMvMEngineerBuildTeleportExit [C1]");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerIdle_Update, "CTFBotMvMEngineerIdle::Update");
			
			MOD_ADD_VHOOK(CObjectDispenser_GetAvailableMetal, TypeName<CObjectDispenser>(), "CObjectDispenser::GetAvailableMetal");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_SpeakConceptIfAllowed, "CTFPlayer::SpeakConceptIfAllowed");

			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerIdle_dtor0, "~CTFBotMvMEngineerIdle [D0]");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerIdle_dtor1, "~CTFBotMvMEngineerIdle [D1]");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerIdle_dtor2, "~CTFBotMvMEngineerIdle [D2]");
			
			//MOD_ADD_DETOUR_MEMBER(CTFBotHintEngineerNest_HintTeleporterThink, "CTFBotHintEngineerNest::HintTeleporterThink");
		}
		
		virtual void OnUnload() override
		{
			for (int i = 0; i < ITFBotHintEntityAutoList::AutoList().Count(); ++i) {
				auto hint = rtti_scast<CBaseTFBotHintEntity *>(ITFBotHintEntityAutoList::AutoList()[i]);
				if (hint != nullptr) {
					CBaseEntity *owner = hint->GetOwnerEntity();
					if (owner != nullptr && rtti_cast<CObjectDispenser *>(owner) != nullptr) {
						owner->Remove();
						hint->SetOwnerEntity(nullptr);
					}
				}
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_engiebot_dispensers", "0", FCVAR_NOTIFY,
		"Mod: make engiebots build dispensers instead of sentries/teles",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
