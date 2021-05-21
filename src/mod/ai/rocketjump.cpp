#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"
#include "util/scope.h"
#include "util/misc.h"


namespace Mod::AI::RocketJump
{
	/* base class */
	class CTFBotRocketJump : public IHotplugAction<CTFBot>
	{
	protected:
		CTFBotRocketJump() {}
		
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
			return QueryResponse::NO;
		}
		
		
		virtual EventDesiredResult<CTFBot> OnLeaveGround(CTFBot *actor, CBaseEntity *ent) override
		{
			DevMsg("[%8.3f] %s(#%d): OnLeaveGround(#%d)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), ENTINDEX(ent));
			return EventDesiredResult<CTFBot>::Continue();
		}
		virtual EventDesiredResult<CTFBot> OnLandOnGround(CTFBot *actor, CBaseEntity *ent) override
		{
			DevMsg("[%8.3f] %s(#%d): OnLandOnGround(#%d)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), ENTINDEX(ent));
			return EventDesiredResult<CTFBot>::Continue();
		}
		
//		virtual EventDesiredResult<CTFBot> OnContact(CTFBot *actor, CBaseEntity *ent, CGameTrace *trace) override
//		{
//			DevMsg("[%8.3f] %s(#%d): OnContact(#%d)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), ENTINDEX(ent));
//			return EventDesiredResult<CTFBot>::Continue();
//		}
		
		virtual EventDesiredResult<CTFBot> OnPostureChanged(CTFBot *actor) override
		{
			DevMsg("[%8.3f] %s(#%d): OnPostureChanged\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
			return EventDesiredResult<CTFBot>::Continue();
		}
		virtual EventDesiredResult<CTFBot> OnAnimationActivityComplete(CTFBot *actor, int i1) override
		{
			DevMsg("[%8.3f] %s(#%d): OnAnimationActivityComplete(%d)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), i1);
			return EventDesiredResult<CTFBot>::Continue();
		}
		virtual EventDesiredResult<CTFBot> OnAnimationActivityInterrupted(CTFBot *actor, int i1) override
		{
			DevMsg("[%8.3f] %s(#%d): OnAnimationActivityInterrupted(%d)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), i1);
			return EventDesiredResult<CTFBot>::Continue();
		}
		virtual EventDesiredResult<CTFBot> OnAnimationEvent(CTFBot *actor, animevent_t *a1) override
		{
			DevMsg("[%8.3f] %s(#%d): OnAnimationEvent(...)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
			return EventDesiredResult<CTFBot>::Continue();
		}
		
		virtual EventDesiredResult<CTFBot> OnInjured(CTFBot *actor, const CTakeDamageInfo& info) override
		{
			DevMsg("[%8.3f] %s(#%d): OnInjured(...)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor));
			return EventDesiredResult<CTFBot>::Continue();
		}
		
		virtual EventDesiredResult<CTFBot> OnSpokeConcept(CTFBot *actor, CBaseCombatCharacter *who, const char *s1, AI_Response *response) override
		{
			DevMsg("[%8.3f] %s(#%d): OnSpokeConcept(#%d, \"%s\", ...)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), ENTINDEX(who), s1);
			return EventDesiredResult<CTFBot>::Continue();
		}
		virtual EventDesiredResult<CTFBot> OnWeaponFired(CTFBot *actor, CBaseCombatCharacter *who, CBaseCombatWeapon *weapon) override
		{
			DevMsg("[%8.3f] %s(#%d): OnWeaponFired(#%d, #%d)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), ENTINDEX(who), ENTINDEX(weapon));
			return EventDesiredResult<CTFBot>::Continue();
		}
		
		virtual EventDesiredResult<CTFBot> OnActorEmoted(CTFBot *actor, CBaseCombatCharacter *who, int concept) override
		{
			DevMsg("[%8.3f] %s(#%d): OnActorEmoted(#%d, %d)\n", gpGlobals->curtime, this->GetName(), ENTINDEX(actor), ENTINDEX(who), concept);
			return EventDesiredResult<CTFBot>::Continue();
		}
	};
	
	
	/* wait for commands */
	class CTFBotRocketJump_Wait : public CTFBotRocketJump
	{
	public:
		virtual const char *GetName() const override { return "RocketJump_Wait"; }
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		
		virtual EventDesiredResult<CTFBot> OnActorEmoted(CTFBot *actor, CBaseCombatCharacter *who, int concept) override;
		virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd) override;
		
	private:
		CountdownTimer m_ctRespond;
	};
	
	
	/* do a vertical jump */
	class CTFBotRocketJump_JumpVertical : public CTFBotRocketJump
	{
	public:
		virtual const char *GetName() const override { return "RocketJump_JumpVertical"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override;
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override;
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override;
		
	private:
		IntervalTimer m_itTimeout;
	};
	
	
	inline ActionResult<CTFBot> CTFBotRocketJump_Wait::Update(CTFBot *actor, float dt)
	{
		if (this->m_ctRespond.HasStarted() && this->m_ctRespond.IsElapsed()) {
			this->m_ctRespond.Invalidate();
			actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_YES);
			return ActionResult<CTFBot>::SuspendFor(new CTFBotRocketJump_JumpVertical(), "Doing a vertical jump");
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	inline EventDesiredResult<CTFBot> CTFBotRocketJump_Wait::OnActorEmoted(CTFBot *actor, CBaseCombatCharacter *who, int concept)
	{
		CTFPlayer *player = ToTFPlayer(who);
		if (player != nullptr && !player->IsBot() && concept == MP_CONCEPT_PLAYER_GO) {
			if (!this->m_ctRespond.HasStarted()) {
				this->m_ctRespond.Start(0.6f);
			}
			
		//	actor->SpeakConceptIfAllowed(MP_CONCEPT_PLAYER_YES);
		//	return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotRocketJump_JumpVertical(), "Doing a vertical jump");
		}
		
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	inline EventDesiredResult<CTFBot> CTFBotRocketJump_Wait::OnCommandString(CTFBot *actor, const char *cmd)
	{
		if (V_stricmp(cmd, "JumpVertical") == 0) {
			return EventDesiredResult<CTFBot>::SuspendFor(new CTFBotRocketJump_JumpVertical(), "Doing a vertical jump");
		}
		
		return EventDesiredResult<CTFBot>::Continue();
	}
	
	
	inline ActionResult<CTFBot> CTFBotRocketJump_JumpVertical::OnStart(CTFBot *actor, Action<CTFBot> *action)
	{
		constexpr float duration = 1.0f;
		
		Vector vecAim = actor->EyePosition() + Vector(0.0f, 0.0f, -100.0f);
		actor->GetBodyInterface()->AimHeadTowards(vecAim, IBody::LookAtPriorityType::OVERRIDE_ALL, duration, nullptr, "Aiming downward for a rocket jump");
		
		this->m_itTimeout.Start();
		
		return ActionResult<CTFBot>::Continue();
	}
	
	inline ActionResult<CTFBot> CTFBotRocketJump_JumpVertical::Update(CTFBot *actor, float dt)
	{
		Vector vecEyes;
		actor->EyeVectors(&vecEyes);
		
		Vector vecDown(0.0f, 0.0f, -1.0f);
		
		static float tolerance = cos(5_deg);
		bool is_aiming_down	= (DotProduct(vecEyes, vecDown) >= tolerance);
		
		if (is_aiming_down) {
			const QAngle& eyeang = actor->EyeAngles();
			DevMsg("*** EyeAngles:  [ %f %f %f ]\n", eyeang.x, eyeang.y, eyeang.z);
			DevMsg("*** EyeVectors: [ %f %f %f ]\n", vecEyes.x, vecEyes.y, vecEyes.z);
			
			actor->PressJumpButton(0.1f);
			actor->PressCrouchButton(3.0f);
			actor->PressFireButton();
			return ActionResult<CTFBot>::Done("Jumped successfully");
		}
		
		if (this->m_itTimeout.IsGreaterThen(1.0f)) {
			return ActionResult<CTFBot>::Done("Timed out");
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	inline void CTFBotRocketJump_JumpVertical::OnEnd(CTFBot *actor, Action<CTFBot> *action)
	{
		
	}
	
	
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotScenarioMonitor_DesiredScenarioAndClassAction, CTFBot *actor)
	{
		return new CTFBotRocketJump_Wait();
	}
	
	
	/* disable the faulty underground detection logic, which warps bots back to
	 * their last nav area if airborne for more than 3 seconds */
	RefCount rc_CTFBotMainAction_Update;
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_Update, CTFBot *actor, float dt)
	{
		constexpr int OFF_CTFBotMainAction_m_itUnderground = 0x70;
		auto m_itUnderground = (IntervalTimer *)((uintptr_t)this + OFF_CTFBotMainAction_m_itUnderground);
		
		m_itUnderground->Reset();
		
		return DETOUR_MEMBER_CALL(CTFBotMainAction_Update)(actor, dt);
	}
	
	
	RefCount rc_CTFPlayer_ApplyPushFromDamage;
	const CTakeDamageInfo *dmginfo = nullptr;
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyPushFromDamage, const CTakeDamageInfo& info, Vector vec)
	{
		dmginfo = &info;
		SCOPED_INCREMENT(rc_CTFPlayer_ApplyPushFromDamage);
		DETOUR_MEMBER_CALL(CTFPlayer_ApplyPushFromDamage)(info, vec);
	}
	
	DETOUR_DECL_MEMBER(void, CTFPlayer_ApplyAbsVelocityImpulse, const Vector& impulse)
	{
		if (rc_CTFPlayer_ApplyPushFromDamage > 0) {
			auto player = reinterpret_cast<CTFPlayer *>(this);
			if (player->IsBot()) {
				if (dmginfo->GetAttacker() == player) {
					float magnitude = impulse.Length();
					
					QAngle angles;
					VectorAngles(impulse, angles);
					
					DevMsg("IMPULSE: mag %.1f, dir [ %.1f %.1f %.1f ]\n",
						magnitude, angles.x, angles.y, angles.z);
					
					constexpr float duration = 3.0f;
					char buf[1024];
					
					NDebugOverlay::VertArrow(player->GetAbsOrigin(), player->GetAbsOrigin() + impulse,
						5.0f, 0x00, 0xff, 0x00, 0xff, false, duration);
					
					snprintf(buf, sizeof(buf), "Mag: %.1f", magnitude);
					NDebugOverlay::EntityTextAtPosition(player->GetAbsOrigin(), 0,
						buf, duration, 0xff, 0xff, 0xff, 0xff);
					
					snprintf(buf, sizeof(buf), "Dir: [ %.1f %.1f %.1f ]", angles.x, angles.y, angles.z);
					NDebugOverlay::EntityTextAtPosition(player->GetAbsOrigin(), 1,
						buf, duration, 0xff, 0xff, 0xff, 0xff);
				}
			}
		}
		
		DETOUR_MEMBER_CALL(CTFPlayer_ApplyAbsVelocityImpulse)(impulse);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("AI:RocketJump")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotScenarioMonitor_DesiredScenarioAndClassAction, "CTFBotScenarioMonitor::DesiredScenarioAndClassAction");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_Update, "CTFBotMainAction::Update");
			
			// TODO: draw vector of rocket start-to-end positions
			// TODO: draw vector of explosion position to bot origin
			//MOD_ADD_DETOUR_MEMBER()
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyPushFromDamage,     "CTFPlayer::ApplyPushFromDamage");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ApplyAbsVelocityImpulse, "CTFPlayer::ApplyAbsVelocityImpulse");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_ai_rocketjump", "0", FCVAR_NOTIFY,
		"Mod: bot AI for rocket jumping",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
