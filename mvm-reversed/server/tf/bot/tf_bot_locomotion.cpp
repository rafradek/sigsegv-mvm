/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_locomotion.cpp
 */


CTFBotLocomotion::CTFBotLocomotion(INextBot *nextbot)
{
}

CTFBotLocomotion::~CTFBotLocomotion()
{
}


void CTFBotLocomotion::Update()
{
	PlayerLocomotion::Update();
	
	CTFBot *actor = ToTFBot(this->GetBot()->GetEntity());
	if (actor == nullptr) {
		return;
	}
	
	if (this->!IsOnGround()) {
		actor->PressCrouchButton(0.3f);
	} else {
		if (!actor->IsPlayerClass(TF_CLASS_SPY)) {
			actor->ReleaseCrouchButton();
		}
	}
}


void CTFBotLocomotion::Approach(const Vector& v1, float f1)
{
	if (TFGameRules()->IsMannVsMachineMode() &&
		!this->IsOnGround() && !this->IsClimbingOrJumping()) {
		return;
	}
	
	PlayerLocomotion::Approach(v1, f1);
}

void CTFBotLocomotion::Jump()
{
	PlayerLocomotion::Jump();
	
	CTFBot *actor = ToTFBot(this->GetBot()->GetEntity());
	if (actor == nullptr) {
		return;
	}
	
	if (TFGameRules()->IsMannVsMachineMode) {
		if (CAttributeManager::AttribHookValue<int>(0,
			"bot_custom_jump_particle", actor, nullptr, true) != 0) {
			DispatchParticleEffect("rocketjump_smoke", PATTACH_POINT_FOLLOW,
				actor, "foot_L");
			DispatchParticleEffect("rocketjump_smoke", PATTACH_POINT_FOLLOW,
				actor, "foot_R");
		}
	}
}

float CTFBotLocomotion::GetMaxJumpHeight() const
{
	return 72.0f;
}

float CTFBotLocomotion::GetDeathDropHeight() const
{
	return 1000.0f;
}

float CTFBotLocomotion::GetRunSpeed() const
{
	/* BUG: this function is the reason why fast bots (e.g. super scouts) will
	 * occasionally decide to randomly slow down to their class's normal maximum
	 * movement speed */
	
	CTFBot *actor = static_cast<CTFBot *>(this->GetBot()->GetEntity());
	
	return GetPlayerClassData(actor->GetPlayerClass()->GetClassIndex())->m_flMaxSpeed;
}

bool CTFBotLocomotion::IsAreaTraversable(const CNavArea *area) const
{
	CBaseCombatCharacter *actor = this->GetBot()->GetEntity();
	
	if (area->IsBlocked(actor->GetTeamNumber())) {
		return false;
	}
	
	// TODO
}

bool CTFBotLocomotion::IsEntityTraversable(CBaseEntity *ent, TraverseWhenType ttype) const
{
	if (ent != nullptr && ent->IsPlayer()) {
		return true;
	}
	
	return ILocomotion::IsEntityTraversable(ent, ttype);
}

void CTFBotLocomotion::AdjustPosture(const Vector& v1)
{
}
