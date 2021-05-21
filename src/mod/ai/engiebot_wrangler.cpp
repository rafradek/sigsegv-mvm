#include "mod.h"
#include "re/nextbot.h"
#include "re/path.h"
#include "stub/objects.h"
#include "stub/tfbot.h"


class CTFBotMvMEngineerIdle : public Action<CTFBot> {};


namespace Mod::AI::EngieBot_Wrangler
{
	class CTFBotMvMEngineerDisableAutopilot : public IHotplugAction<CTFBot>
	{
	public:
		CTFBotMvMEngineerDisableAutopilot() {}
		
		virtual const char *GetName() const override { return "MvMEngineerDisableAutopilot"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			if (!IsPossible(actor)) {
				return ActionResult<CTFBot>::Done("This thing can't go off autopilot anymore, son!");
			}
			
			enginesound->PrecacheSound("vo/mvm/norm/engineer_mvm_wranglekills01.mp3");
			enginesound->PrecacheSound("vo/mvm/norm/engineer_mvm_wranglekills02.mp3");
			enginesound->PrecacheSound("vo/mvm/norm/engineer_mvm_wranglekills03.mp3");
			enginesound->PrecacheSound("vo/mvm/norm/engineer_mvm_wranglekills04.mp3");
			
			CTFWeaponBase *wrangler = actor->Weapon_OwnsThisID(TF_WEAPON_LASER_POINTER);
			actor->PushRequiredWeapon(wrangler);
			
			/* wait for the weapon switch to happen, so we don't accidentally pick up our sentry */
			this->m_ctInitialHoldoff.Start(0.6f);
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			if (!IsPossible(actor)) {
				return ActionResult<CTFBot>::Done("This thing can't go off autopilot anymore, son!");
			}
			
			if (!this->m_ctInitialHoldoff.IsElapsed()) {
				return ActionResult<CTFBot>::Continue();
			}
			
			DevMsg("[%8.3f] Update\n", gpGlobals->curtime);
			
		//	const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(true);
			const CKnownEntity *threat = actor->GetVisionInterface()->GetPrimaryKnownThreat(false);
			if (threat != nullptr && threat->GetEntity() != nullptr) {
				actor->GetBodyInterface()->AimHeadTowards(threat->GetEntity()->WorldSpaceCenter(),
					IBody::LookAtPriorityType::OVERRIDE_ALL, 0.1f, nullptr, "Aiming!");
				
				if (actor->GetBodyInterface()->IsHeadAimingOnTarget() &&
					threat->IsVisibleInFOVNow() && threat->GetEntity()->IsAlive()) {
					actor->PressFireButton(0.1f);
					actor->PressAltFireButton(0.1f);
				}

				INextBot *nextbot = actor->MyNextBotPointer();
				if ( m_repathTimer.IsElapsed() )
				{
					CObjectSentrygun *sentry = rtti_scast<CObjectSentrygun *>(actor->GetObjectOfType(OBJ_SENTRYGUN, 0));
					m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

					Vector mySentryForward;
					AngleVectors( sentry->GetTurretAngles(), &mySentryForward );

					Vector behindSentrySpot = sentry->GetAbsOrigin() - 50.0f * mySentryForward;

					CTFBotPathCost cost( actor, SAFEST_ROUTE );
					m_PathFollower.Compute( nextbot, behindSentrySpot, cost, 0.0f, true);
				}

				m_PathFollower.Update(nextbot);
			}
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override
		{
			actor->PopRequiredWeapon();
		}
		
		
		virtual EventDesiredResult<CTFBot> OnOtherKilled(CTFBot *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)
		{
			if (who->GetTeamNumber() != actor->GetTeamNumber()) {
				actor->EmitSound("Engineer.MvM_Wrangler01");
			}
			
			return EventDesiredResult<CTFBot>::Done("Gotcha!");
		}
		
		
		virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd)
		{
			if (V_stricmp(cmd, "say1") == 0) {
				actor->EmitSound("Engineer.MvM_Wrangler01");
			}
			if (V_stricmp(cmd, "say2") == 0) {
				actor->EmitSound("Engineer.MvM_Wrangler02");
			}
			if (V_stricmp(cmd, "say3") == 0) {
				actor->EmitSound("Engineer.MvM_Wrangler03");
			}
			if (V_stricmp(cmd, "say4") == 0) {
				actor->EmitSound("Engineer.MvM_Wrangler04");
			}
			
			if (V_stricmp(cmd, "unwrangle") == 0) {
				return EventDesiredResult<CTFBot>::Done("This thing's back on autopilot, son!");
			}
			
			return EventDesiredResult<CTFBot>::Continue();
		}
		
		
		static bool IsPossible(CTFBot *actor)
		{
			if (!actor->IsPlayerClass(TF_CLASS_ENGINEER))                     return false;
			if (actor->Weapon_OwnsThisID(TF_WEAPON_LASER_POINTER) == nullptr) return false;
			if (actor->GetObjectOfType(OBJ_SENTRYGUN, 0) == nullptr || actor->GetObjectOfType(OBJ_SENTRYGUN, 0)->m_bBuilding || actor->GetObjectOfType(OBJ_SENTRYGUN, 0)->m_bDisabled)          return false;
			
			return true;
		}
		
	private:
		CountdownTimer m_repathTimer;

		CountdownTimer m_ctInitialHoldoff;
		
		PathFollower m_PathFollower;
		CountdownTimer m_ctRecomputePath;
		
		CHandle<CTFPlayer> m_hTarget;
	};
	
	
	// custom action:
	// switch to wrangler
	// aim at primary known threat
	// hold down primary and secondary fire if primary known threat is visible (recently?)
	// move to the opposite side of the sentry as much as possible (similar to what CTFBotMvMEngineerIdle already does)
	
	// maybe iterate over all known threats
	// then narrow them down by which ones he has a clear line of sight to
	// and prioritize by distance, more dangerous threat functor, etc
	
	// probably manually trigger some of the wrangler voice lines when spotting a target
	// "engineer_wranglekills01" | "engineer_wranglekills01.mp3" "engineer_mvm_wranglekills01.mp3"
	// "engineer_wranglekills02" | "engineer_wranglekills02.mp3" "engineer_mvm_wranglekills02.mp3"
	// "engineer_wranglekills03" | "engineer_wranglekills03.mp3" "engineer_mvm_wranglekills03.mp3"
	// "engineer_wranglekills04" | "engineer_wranglekills04.mp3" "engineer_mvm_wranglekills04.mp3"
	// (no sound script names for the mvm versions sadly)
	
	// ideally: disable the sniper blue laser eyes if possible
	// client C_SniperDot::ClientThink is creating particle "laser_sight_beam" for MvM mode blu team players
	// so maybe we just kill-all-particles from the server right after each time CTFLaserPointer::CreateLaserDot is called?
	// - hopefully we can call StopParticleEffects on the laser dot entity itself and not the player entity!
	
	
	DETOUR_DECL_MEMBER(void, CTFLaserPointer_CreateLaserDot)
	{
		auto weapon = reinterpret_cast<CBaseEntity *>(this);

		if (weapon->GetTeamNumber() != TF_TEAM_BLUE)
			DETOUR_MEMBER_CALL(CTFLaserPointer_CreateLaserDot)();
	}
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, Action_CTFBot_OnCommandString, CTFBot *actor, const char *cmd)
	{
		if (V_stricmp(cmd, "wrangle") == 0) {
			auto action = reinterpret_cast<Action<CTFBot> *>(this);
			if (rtti_cast<CTFBotMvMEngineerIdle *>(action) != nullptr) {
				if (CTFBotMvMEngineerDisableAutopilot::IsPossible(actor)) {
					return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotMvMEngineerDisableAutopilot(), "This thing ain't on autopilot, son!");
				} else {
					Warning("This thing can't disengage autopilot, son!\n");
				}
			}
		}
		
		return DETOUR_MEMBER_CALL(Action_CTFBot_OnCommandString)(actor, cmd);
	}
	DETOUR_DECL_MEMBER(ActionResult< CTFBot >, CTFBotMvMEngineerIdle_Update, CTFBot *actor, float interval)
	{
		
		auto action = reinterpret_cast<Action<CTFBot> *>(this);
		if (CTFBotMvMEngineerDisableAutopilot::IsPossible(actor) && actor->GetVisionInterface()->GetPrimaryKnownThreat(false) != nullptr) {
			return ActionResult< CTFBot >::SuspendFor(new CTFBotMvMEngineerDisableAutopilot(), "This thing ain't on autopilot, son!");
		}
		return DETOUR_MEMBER_CALL(CTFBotMvMEngineerIdle_Update)(actor, interval);
	}

	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:EngieBot_Wrangler")
		{
			MOD_ADD_DETOUR_MEMBER(Action_CTFBot_OnCommandString, "Action<CTFBot>::OnCommandString");
			MOD_ADD_DETOUR_MEMBER(CTFBotMvMEngineerIdle_Update, "CTFBotMvMEngineerIdle::Update");
			//MOD_ADD_DETOUR_MEMBER(CTFLaserPointer_CreateLaserDot, "CTFLaserPointer::CreateLaserDot");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_engiebot_wrangler", "0", FCVAR_NOTIFY,
		"Mod: enable custom AI behavior for wrangling sentry",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
