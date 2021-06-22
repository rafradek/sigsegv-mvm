/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/behavior/engineer/mvm_engineer/tf_bot_mvm_engineer_build_sentry.cpp
 * used in MvM: TODO
 */


CTFBotMvMEngineerBuildSentryGun::CTFBotMvMEngineerBuildSentryGun(CTFBotHintSentrygun *hint)
{
	this->m_hintEntity = hint;
}

CTFBotMvMEngineerBuildSentryGun::~CTFBotMvMEngineerBuildSentryGun()
{
}


const char *CTFBotMvMEngineerBuildSentryGun::GetName() const
{
	return "MvMEngineerBuildSentryGun";
}


ActionResult<CTFBot> CTFBotMvMEngineerBuildSentryGun::OnStart(CTFBot *actor, Action<CTFBot> *action)
{
	actor->StartBuildingObjectOfType(OBJ_SENTRYGUN, 0);
	return ActionResult<CTFBot>::Continue();
}

ActionResult<CTFBot> CTFBotMvMEngineerBuildSentryGun::Update(CTFBot *actor, float dt)
{
	if (this->m_hintEntity == nullptr) {
		return ActionResult<CTFBot>::Done("No hint entity");
	}
	
	float range_to_hint = actor->GetRangeTo(this->m_hintEntity->GetAbsOrigin());
	
	if (range_to_hint < 200.0f) {
		actor->PressCrouchButton();
		actor->GetBodyInterface()->AimHeadTowards(this->m_hintEntity->GetAbsOrigin(),
			IBody::LookAtPriorityType::OVERRIDE_ALL, 0.1f, nullptr, "Placing sentry");
	}
	
	if (range_to_hint > 25.0f) {
		if (this->m_ctRecomputePath.IsElapsed()) {
			this->m_ctRecomputePath.Start(RandomFloat(1.0f, 2.0f));
			
			CTFBotPathCost cost_func(actor, SAFEST_ROUTE);
			this->m_PathFollower.Compute<CTFBotPathCost>(actor,
				this->m_hintEntity->GetAbsOrigin(), cost_func, 0.0f, true);
		}
		
		this->m_PathFollower.Update(actor);
		if (!this->m_PathFollower.IsValid()) {
			/* BUG: one path failure ends the entire behavior...
			 * could this be why engiebots sometimes zone out? */
			return ActionResult<CTFBot>::Done("Path failed");
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (!this->m_ctPushAway.HasStarted()) {
		this->m_ctPushAway.Start(0.1f);
		
		if (this->m_hintEntity != nullptr) {
			TFGameRules()->PushAllPlayersAway(this->m_hintEntity->GetAbsOrigin(),
				400.0f, 500.0f, TF_TEAM_RED, nullptr);
		}
		
		return ActionResult<CTFBot>::Continue();
	}
	
	if (!this->m_ctPushAway.IsElapsed()) {
		return ActionResult<CTFBot>::Continue();
	}
	
	actor->DetonateObjectOfType(OBJ_SENTRYGUN, 0, true);
	
	CBaseEntity *ent = CreateEntityByName("obj_sentrygun");
	if (ent == nullptr) {
		/* BUG: no we didn't */
		return ActionResult<CTFBot>::Done("Built a sentry");
	}
	
	this->m_hSentry = static_cast<CObjectSentrygun *>(ent);
	this->m_hSentry->SetName(this->m_hintEntity->GetEntityName());
	
	// TODO:
	// ++this->m_hintEntity->dword_0x370
	
	this->m_hSentry->m_nDefaultUpgradeLevel = 2;
	
	this->m_hSentry->SetAbsOrigin(this->m_hintEntity->GetAbsOrigin());
	this->m_hSentry->SetAbsAngles(this->m_hintEntity->GetAbsAngles());
	this->m_hSentry->Spawn();
	
	this->m_hSentry->StartPlacement(actor);
	this->m_hSentry->StartBuilding(actor);
	
	this->m_hintEntity->SetOwnerEntity(this->m_hSentry);
	
	this->m_hSentry = nullptr;
	return ActionResult<CTFBot>::Done("Built a sentry");
}

void CTFBotMvMEngineerBuildSentryGun::OnEnd(CTFBot *actor, Action<CTFBot> *action)
{
	if (this->m_hSentry != nullptr) {
		this->m_hSentry->DropCarriedObject(actor);
		UTIL_Remove(this->m_hSentry);
		this->m_hSentry = nullptr;
	}
}
