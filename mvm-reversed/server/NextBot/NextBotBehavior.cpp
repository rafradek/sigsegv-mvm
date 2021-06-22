/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotBehavior.cpp
 */


// TODO: these should all actually be inline definitions in NextBotContextualQueryInterface.h
// and this file shouldn't exist


template<class T> Behavior<T>::Behavior(Action<T> *mainaction)
	: m_MainAction(mainaction), m_strName("%s", "")
{
	this->m_Actor = nullptr;
}

template<class T> Behavior<T>::~Behavior()
{
	if (this->m_MainAction != nullptr) {
		if (this->m_Actor != nullptr) {
			this->m_MainAction->InvokeOnEnd(actor, this, nullptr);
			this->m_Actor = nullptr;
		}
		
		Action<T> *action = this->m_MainAction;
		while (action->m_ActionWeSuspended != nullptr) {
			action = action->m_ActionWeSuspended;
		}
		delete action;
	}
}


template<class T> INextBotEventResponder *Behavior<T>::FirstContainedResponder() const
{
	return this->m_MainAction;
}

template<class T> INextBotEventResponder *Behavior<T>::NextContainedResponder(INextBotEventResponder *prev) const
{
	return nullptr;
}


template<class T> QueryResponse Behavior<T>::ShouldPickUp(const INextBot *nextbot, CBaseEntity *it) const
{
	if (this->m_MainAction == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			QueryResponse response = action->ShouldPickUp(nextbot, it);
			if (response != QueryResponse::DONTCARE) {
				return response;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return QueryResponse::DONTCARE;
}

template<class T> QueryResponse Behavior<T>::ShouldHurry(const INextBot *nextbot) const
{
	if (this->m_MainAction == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			QueryResponse response = action->ShouldHurry(nextbot);
			if (response != QueryResponse::DONTCARE) {
				return response;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return QueryResponse::DONTCARE;
}

template<class T> QueryResponse Behavior<T>::ShouldRetreat(const INextBot *nextbot) const
{
	if (this->m_MainAction == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			QueryResponse response = action->ShouldRetreat(nextbot);
			if (response != QueryResponse::DONTCARE) {
				return response;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return QueryResponse::DONTCARE;
}

template<class T> QueryResponse Behavior<T>::ShouldAttack(const INextBot *nextbot, const CKnownEntity *threat) const
{
	if (this->m_MainAction == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			QueryResponse response = action->ShouldAttack(nextbot, threat);
			if (response != QueryResponse::DONTCARE) {
				return response;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return QueryResponse::DONTCARE;
}

template<class T> QueryResponse Behavior<T>::IsHindrance(const INextBot *nextbot, CBaseEntity *it) const
{
	if (this->m_MainAction == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			QueryResponse response = action->IsHindrance(nextbot, it);
			if (response != QueryResponse::DONTCARE) {
				return response;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return QueryResponse::DONTCARE;
}

template<class T> Vector Behavior<T>::SelectTargetPoint(const INextBot *nextbot, const CBaseCombatCharacter *them) const
{
	if (this->m_MainAction == nullptr) {
		return vec3_origin;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			Vector v = action->SelectTargetPoint(nextbot, them);
			if (v != vec3_origin) {
				return v;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return vec3_origin;
}

template<class T> QueryResponse Behavior<T>::IsPositionAllowed(const INextBot *nextbot, const Vector& v1) const
{
	if (this->m_MainAction == nullptr) {
		return QueryResponse::DONTCARE;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			QueryResponse response = action->IsPositionAllowed(nextbot, v1);
			if (response != QueryResponse::DONTCARE) {
				return response;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return QueryResponse::DONTCARE;
}

template<class T> const CKnownEntity *Behavior<T>::SelectMoreDangerousThreat(const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2) const
{
	if (this->m_MainAction == nullptr) {
		return nullptr;
	}
	
	Action<T> *action = this->m_MainAction;
	while (action->m_ActionChild != nullptr) {
		action = action->m_ActionChild;
	}
	
	do {
		Action<T> *parent = action->m_ParentAction;
		
		while (action != nullptr) {
			const CKnownEntity *known = action->SelectMoreDangerousThreat(nextbot, them, threat1, threat2);
			if (known != nullptr) {
				return known;
			}
			
			action = action->m_ActionWeSuspended;
		}
		
		action = parent;
	} while (action != nullptr);
	
	return QueryResponse::DONTCARE;
}


template<class T> void Behavior<T>::Update(T *actor, float dt)
{
	if (actor == nullptr || this->m_MainAction == nullptr) {
		return;
	}
	
	this->m_Actor = actor;
	
	ActionResult<T> result = this->m_MainAction->InvokeUpdate(actor, this, dt);
	this->m_MainAction = this->m_MainAction->ApplyResult(actor, this, result);
	
	if (this->m_MainAction != nullptr && actor->IsDebugging(DEBUG_BEHAVIOR)) {
		CFmtStrN<256> fmtstr;
		actor->DisplayDebugText(fmtstr.sprintf("%s: %s",
			this->m_strName.m_szBuf, this->m_MainAction->DebugString()))
	}
	
	this->m_DestroyedActions.PurgeAndDeleteElements();
}

/* inlined except in ClientOSX >= 20160121a */
template<class T> void Behavior<T>::DestroyAction(Action<T> *action)
{
	this->m_DestroyedActions.AddToTail(action);
}


template<class T> Action<T>::Action()
{
}

template<class T> Action<T>::~Action()
{
	if (this->m_ActionParent != nullptr && this->m_ActionParent->m_ActionChild == this) {
		this->m_ActionParent->m_ActionChild = this->m_ActionWeSuspended;
	}
	
	if (this->m_ActionChild != nullptr) {
		Action<T> *suspended = this->m_ActionChild;
		do {
			Action<T> *next = suspended->m_ActionWeSuspended;
			delete suspended;
			suspended = next;
		} while (suspended != nullptr);
	}
	
	if (this->m_ActionWeSuspended != nullptr) {
		this->m_ActionWeSuspended->m_ActionSuspendedUs = nullptr;
	}
	
	if (this->m_ActionSuspendedUs != nullptr) {
		delete this->m_ActionSuspendedUs;
	}
	
	if (this->m_Result.action != nullptr) {
		delete this->m_Result.action;
	}
}


template<class T> INextBotEventResponder *Action<T>::FirstContainedResponder() const
{
	return this->m_ActionChild;
}

template<class T> INextBotEventResponder *Action<T>::NextContainedResponder(INextBotEventResponder *prev) const
{
	return nullptr;
}


template<class T> void Action<T>::OnLeaveGround(CBaseEntity *ent)
{
	this->HandleEvent("OnLeaveGround", [&](Action<T> *action, T *actor) {
			return action->OnLeaveGround(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnLeaveGround(ent);
	}
}

template<class T> void Action<T>::OnLandOnGround(CBaseEntity *ent)
{
	this->HandleEvent("OnLandOnGround", [&](Action<T> *action, T *actor) {
			return action->OnLandOnGround(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnLandOnGround(ent);
	}
}


template<class T> void Action<T>::OnContact(CBaseEntity *ent, CGameTrace *trace)
{
	this->HandleEvent("OnContact", [&](Action<T> *action, T *actor) {
			return action->OnContact(actor, ent, trace);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnContact(ent, trace);
	}
}


template<class T> void Action<T>::OnMoveToSuccess(const Path *path)
{
	this->HandleEvent("OnMoveToSuccess", [&](Action<T> *action, T *actor) {
			return action->OnMoveToSuccess(actor, path);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnMoveToSuccess(path);
	}
}

template<class T> void Action<T>::OnMoveToFailure(const Path *path, MoveToFailureType fail)
{
	this->HandleEvent("OnMoveToFailure", [&](Action<T> *action, T *actor) {
			return action->OnMoveToFailure(actor, path, fail);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnMoveToFailure(path, fail);
	}
}


template<class T> void Action<T>::OnStuck()
{
	this->HandleEvent("OnStuck", [&](Action<T> *action, T *actor) {
			return action->OnStuck(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnStuck();
	}
}

template<class T> void Action<T>::OnUnStuck()
{
	this->HandleEvent("OnUnStuck", [&](Action<T> *action, T *actor) {
			return action->OnUnStuck(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnUnStuck();
	}
}


template<class T> void Action<T>::OnPostureChanged()
{
	this->HandleEvent("OnPostureChanged", [&](Action<T> *action, T *actor) {
			return action->OnPostureChanged(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnPostureChanged();
	}
}

template<class T> void Action<T>::OnAnimationActivityComplete(int i1)
{
	this->HandleEvent("OnAnimationActivityComplete", [&](Action<T> *action, T *actor) {
			return action->OnAnimationActivityComplete(actor, i1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnAnimationActivityComplete(i1);
	}
}

template<class T> void Action<T>::OnAnimationActivityInterrupted(int i1)
{
	this->HandleEvent("OnAnimationActivityInterrupted", [&](Action<T> *action, T *actor) {
			return action->OnAnimationActivityInterrupted(actor, i1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnAnimationActivityInterrupted(i1);
	}
}

template<class T> void Action<T>::OnAnimationEvent(animevent_t *a1)
{
	this->HandleEvent("OnAnimationEvent", [&](Action<T> *action, T *actor) {
			return action->OnAnimationEvent(actor, a1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnAnimationEvent(a1);
	}
}


template<class T> void Action<T>::OnIgnite()
{
	this->HandleEvent("OnIgnite", [&](Action<T> *action, T *actor) {
			return action->OnIgnite(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnIgnite();
	}
}

template<class T> void Action<T>::OnInjured(const CTakeDamageInfo& info)
{
	this->HandleEvent("OnInjured", [&](Action<T> *action, T *actor) {
			return action->OnInjured(actor, info);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnInjured(info);
	}
}

template<class T> void Action<T>::OnKilled(const CTakeDamageInfo& info)
{
	this->HandleEvent("OnKilled", [&](Action<T> *action, T *actor) {
			return action->OnKilled(actor, info);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnKilled(info);
	}
}

template<class T> void Action<T>::OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info)
{
	this->HandleEvent("OnOtherKilled", [&](Action<T> *action, T *actor) {
			return action->OnOtherKilled(actor, who, info);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnOtherKilled(who, info);
	}
}


template<class T> void Action<T>::OnSight(CBaseEntity *ent)
{
	this->HandleEvent("OnSight", [&](Action<T> *action, T *actor) {
			return action->OnSight(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnSight(ent);
	}
}

template<class T> void Action<T>::OnLostSight(CBaseEntity *ent)
{
	this->HandleEvent("OnLostSight", [&](Action<T> *action, T *actor) {
			return action->OnLostSight(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnLostSight(ent);
	}
}


template<class T> void Action<T>::OnSound(CBaseEntity *ent, const Vector& v1, KeyValues *kv)
{
	this->HandleEvent("OnSound", [&](Action<T> *action, T *actor) {
			return action->OnSound(actor, ent, v1, kv);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnSound(ent, v1, kv);
	}
}

template<class T> void Action<T>::OnSpokeConcept(CBaseCombatCharacter *who, const char *s1, AI_Response *response)
{
	this->HandleEvent("OnSpokeConcept", [&](Action<T> *action, T *actor) {
			return action->OnSpokeConcept(actor, who, s1, response);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnSpokeConcept(who, s1, response);
	}
}

template<class T> void Action<T>::OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)
{
	this->HandleEvent("OnWeaponFired", [&](Action<T> *action, T *actor) {
			return action->OnWeaponFired(actor, who, weapon);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnWeaponFired(who, weapon);
	}
}


template<class T> void Action<T>::OnNavAreaChanged(CNavArea *area1, CNavArea *area2)
{
	this->HandleEvent("OnNavAreaChanged", [&](Action<T> *action, T *actor) {
			return action->OnNavAreaChanged(actor, area1, area2);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnNavAreaChanged(area1, area2);
	}
}

template<class T> void Action<T>::OnModelChanged()
{
	this->HandleEvent("OnModelChanged", [&](Action<T> *action, T *actor) {
			return action->OnModelChanged(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnModelChanged();
	}
}

template<class T> void Action<T>::OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who)
{
	this->HandleEvent("OnPickUp", [&](Action<T> *action, T *actor) {
			return action->OnPickUp(actor, ent, who);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnPickUp(ent, who);
	}
}

template<class T> void Action<T>::OnDrop(CBaseEntity *ent)
{
	this->HandleEvent("OnDrop", [&](Action<T> *action, T *actor) {
			return action->OnDrop(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnDrop(ent);
	}
}

template<class T> void Action<T>::OnActorEmoted(CBaseCombatCharacter *who, int concept)
{
	this->HandleEvent("OnActorEmoted", [&](Action<T> *action, T *actor) {
			return action->OnActorEmoted(actor, who, concept);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnActorEmoted(who, concept);
	}
}


template<class T> void Action<T>::OnCommandAttack(CBaseEntity *ent)
{
	this->HandleEvent("OnCommandAttack", [&](Action<T> *action, T *actor) {
			return action->OnCommandAttack(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnCommandAttack(ent);
	}
}

template<class T> void Action<T>::OnCommandApproach(const Vector& v1, float f1)
{
	this->HandleEvent("OnCommandApproach", [&](Action<T> *action, T *actor) {
			return action->OnCommandApproach(actor, v1, f1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnCommandApproach(v1, f1);
	}
}

template<class T> void Action<T>::OnCommandApproach(CBaseEntity *ent)
{
	this->HandleEvent("OnCommandApproach", [&](Action<T> *action, T *actor) {
			return action->OnCommandApproach(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnCommandApproach(ent);
	}
}

template<class T> void Action<T>::OnCommandRetreat(CBaseEntity *ent, float f1)
{
	this->HandleEvent("OnCommandRetreat", [&](Action<T> *action, T *actor) {
			return action->OnCommandRetreat(actor, ent, f1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnCommandRetreat(ent, f1);
	}
}

template<class T> void Action<T>::OnCommandPause(float f1)
{
	this->HandleEvent("OnCommandPause", [&](Action<T> *action, T *actor) {
			return action->OnCommandPause(actor, f1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnCommandPause(f1);
	}
}

template<class T> void Action<T>::OnCommandResume()
{
	this->HandleEvent("OnCommandResume", [&](Action<T> *action, T *actor) {
			return action->OnCommandResume(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnCommandResume();
	}
}

template<class T> void Action<T>::OnCommandString(const char *cmd)
{
	this->HandleEvent("OnCommandString", [&](Action<T> *action, T *actor) {
			return action->OnCommandString(actor, cmd);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnCommandString(cmd);
	}
}


template<class T> void Action<T>::OnShoved(CBaseEntity *ent)
{
	this->HandleEvent("OnShoved", [&](Action<T> *action, T *actor) {
			return action->OnShoved(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnShoved(ent);
	}
}

template<class T> void Action<T>::OnBlinded(CBaseEntity *ent)
{
	this->HandleEvent("OnBlinded", [&](Action<T> *action, T *actor) {
			return action->OnBlinded(actor, ent);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnBlinded(ent);
	}
}


template<class T> void Action<T>::OnTerritoryContested(int i1)
{
	this->HandleEvent("OnTerritoryContested", [&](Action<T> *action, T *actor) {
			return action->OnTerritoryContested(actor, i1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnTerritoryContested(i1);
	}
}

template<class T> void Action<T>::OnTerritoryCaptured(int i1)
{
	this->HandleEvent("OnTerritoryCaptured", [&](Action<T> *action, T *actor) {
			return action->OnTerritoryCaptured(actor, i1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnTerritoryCaptured(i1);
	}
}

template<class T> void Action<T>::OnTerritoryLost(int i1)
{
	this->HandleEvent("OnTerritoryLost", [&](Action<T> *action, T *actor) {
			return action->OnTerritoryLost(actor, i1);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnTerritoryLost(i1);
	}
}


template<class T> void Action<T>::OnWin()
{
	this->HandleEvent("OnWin", [&](Action<T> *action, T *actor) {
			return action->OnWin(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnWin();
	}
}

template<class T> void Action<T>::OnLose()
{
	this->HandleEvent("OnLose", [&](Action<T> *action, T *actor) {
			return action->OnLose(actor);
		});
	
	FOR_EACH_RESPONDER {
		responder->OnLose();
	}
}


template<class T> bool Action<T>::IsNamed(const char *name) const
{
	const char *my_name = this->GetName();
	return ((name == my_name) || (V_stricmp(name, my_name) == 0));
}

template<class T> char *Action<T>::GetFullName() const
{
	static char strconst[256];
	const char *names[66];
	
	strconst[0] = '\0';
	
	if (this != nullptr) {
		Action<T> *action = this;
		int count = 0;
		
		do {
			names[count++] = action->GetName();
			action = action->m_ActionParent;
		} while (count <= 63 && action != nullptr);
		
		for (int i = count - 1; i != 0; --i) {
			V_strncat(strconst, names[i], sizeof(strconst));
			V_strncat(strconst, "/", sizeof(strconst));
		}
	}
	
	V_strncat(strconst, names[0], sizeof(strconst));
	return strconst;
}


template<class T> ActionResult<T> Action<T>::OnStart(T *actor, Action<T> *action)
{
	return ActionResult<T>::Continue();
}

template<class T> ActionResult<T> Action<T>::Update(T *actor, float dt)
{
	return ActionResult<T>::Continue();
}

template<class T> void Action<T>::OnEnd(T *actor, Action<T> *action)
{
}


template<class T> ActionResult<T> Action<T>::OnSuspend(T *actor, Action<T> *action)
{
	return ActionResult<T>::Continue();
}

template<class T> ActionResult<T> Action<T>::OnResume(T *actor, Action<T> *action)
{
	return ActionResult<T>::Continue();
}


template<class T> Action<T> *Action<T>::InitialContainedAction(T *actor)
{
	return nullptr;
}


template<class T> EventDesiredResult<T> Action<T>::OnLeaveGround(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnLandOnGround(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnContact(T *actor, CBaseEntity *ent, CGameTrace *trace)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnMoveToSuccess(T *actor, const Path *path)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnMoveToFailure(T *actor, const Path *path, MoveToFailureType fail)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnStuck(T *actor)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnUnStuck(T *actor)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnPostureChanged(T *actor)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnAnimationActivityComplete(T *actor, int i1)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnAnimationActivityInterrupted(T *actor, int i1)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnAnimationEvent(T *actor, animevent_t *a1)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnIgnite(T *actor)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnInjured(T *actor, const CTakeDamageInfo& info)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnKilled(T *actor, const CTakeDamageInfo& info)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnOtherKilled(T *actor, CBaseCombatCharacter *who, const CTakeDamageInfo& info)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnSight(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnLostSight(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnSound(T *actor, CBaseEntity *ent, const Vector& v1, KeyValues *kv)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnSpokeConcept(T *actor, CBaseCombatCharacter *who, const char *s1, AI_Response *response)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnWeaponFired(T *actor, CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnNavAreaChanged(T *actor, CNavArea *area1, CNavArea *area2)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnModelChanged(T *actor)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnPickUp(T *actor, CBaseEntity *ent, CBaseCombatCharacter *who)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnDrop(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnActorEmoted(T *actor, CBaseCombatCharacter *who, int concept)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnCommandAttack(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnCommandApproach(T *actor, const Vector& v1, float f1)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnCommandApproach(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnCommandRetreat(T *actor, CBaseEntity *ent, float f1)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnCommandPause(T *actor, float f1)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnCommandResume(T *actor)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnCommandString(T *actor, const char *cmd)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnShoved(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnBlinded(T *actor, CBaseEntity *ent)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnTerritoryContested(T *actor, int i1)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnTerritoryCaptured(T *actor, int i1)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnTerritoryLost(T *actor, int i1)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> EventDesiredResult<T> Action<T>::OnWin(T *actor)
{
	return EventDesiredResult<T>::Continue();
}

template<class T> EventDesiredResult<T> Action<T>::OnLose(T *actor)
{
	return EventDesiredResult<T>::Continue();
}


template<class T> bool Action<T>::IsAbleToBlockMovementOf(const INextBot *nextbot) const
{
	return true;
}


template<class T> T *Action<T>::GetActor() const
{
	return this->m_Actor;
}


template<class T> Action<T> *Action<T>::ApplyResult(T *actor, Behavior<T> *behavior, ActionResult<T> result)
{
	if (result.transition == ActionTransition::CHANGE_TO) {
		if (result.action == nullptr) {
			DevMsg("Error: Attempted CHANGE_TO to a NULL Action\n");
			return this;
		}
		
		if (actor->IsDebugging(DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_LTYELLOW, "%3.2f: %s:%s: ",
				gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->m_strName());
			
			if (result.action == this) {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_RED, "START ");
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_WHITE, this->GetName());
			} else {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_WHITE, this->GetName());
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_RED, " CHANGE_TO ");
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_WHITE, result.action->GetName());
			}
			
			if (result.reason != nullptr) {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_LTGREEN, "  (%s)\n",
					result.reason);
			} else {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_WHITE, "\n");
			}
		}
		
		this->InvokeOnEnd(actor, behavior, result.action);
		ActionResult<T> result2 =
			result.action->InvokeOnStart(actor, behavior, this, this->m_ActionWeSuspended);
		
		if (result.action != this) {
			behavior->DestroyAction(this);
		}
		
		if (actor->IsDebugging(DEBUG_BEHAVIOR)) {
			result.action->PrintStateToConsole();
		}
		
		return result.action->ApplyResult(actor, behavior, result2);
	} else if (result.transition == ActionTransition::SUSPEND_FOR) {
		Action<T> *action = this;
		while (action->m_ActionSuspendedUs != nullptr) {
			action = action->m_ActionSuspendedUs;
		}
		
		if (actor->IsDebugging(DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_LTYELLOW, "%3.2f: %s:%s: ",
				gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->m_strName());
			
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_WHITE, this->GetName());
			
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_MAGENTA, " caused ");
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_WHITE, action->GetName());
			
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_MAGENTA, " to SUSPEND_FOR ");
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_WHITE, result.action->GetName());
			
			if (result.reason != nullptr) {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_LTGREEN, "  (%s)\n",
					result.reason);
			} else {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_WHITE, "\n");
			}
		}
		
		Action<T> *next = action->InvokeOnSuspend(actor, behavior, result.action);
		ActionResult<T> result2 =
			result.action->InvokeOnStart(actor, behavior, next, next);
		
		if (actor->IsDebugging(DEBUG_BEHAVIOR)) {
			result.action->PrintStateToConsole();
		}
		
		return result.action->ApplyResult(actor, behavior, result2);
	} else if (result.transition == ActionTransition::DONE) {
		// TODO: name for m_strName
		
		this->InvokeOnEnd(actor, behavior, this->m_ActionWeSuspended);
		
		if (actor->IsDebugging(DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_LTYELLOW, "%3.2f: %s:%s: ",
				gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->m_strName());
			
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_WHITE, this->GetName());
			
			if (this->m_ActionWeSuspended != nullptr) {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_GREEN, " DONE, RESUME ");
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_WHITE, this->m_ActionWeSuspended->GetName());
			} else {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_GREEN, " DONE.");
			}
			
			if (result.reason != nullptr) {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_LTGREEN, "  (%s)\n",
					result.reason);
			} else {
				actor->DebugConColorMsg(DEBUG_BEHAVIOR,
					COLOR_WHITE, "\n");
			}
		}
		
		if (this->m_ActionWeSuspended != nullptr) {
			ActionResult<T> result2 =
				this->m_ActionWeSuspended->InvokeOnResume(actor, behavior, this);
			
			if (actor->IsDebugging(DEBUG_BEHAVIOR)) {
				this->m_ActionWeSuspended->PrintStateToConsole();
			}
			
			behavior->DestroyAction(this);
			return this->m_ActionWeSuspended->ApplyResult(actor, behavior, result2);
		} else {
			behavior->DestroyAction(this);
			return nullptr;
		}
	} else {
		/* ActionTransition::CONTINUE */
		return this;
	}
}


template<class T> ActionResult<T> Action<T>::InvokeOnStart(T *actor, Behavior<T> *behavior, Action<T> *action1, Action<T> *action2)
{
	// SUSPEND_FOR:
	// action1 = action which was suspended for us
	// action2 = action which was suspended for us
	
	// SUSPEND_FOR + DONE:
	// action1 = the suspended action's previously suspended action, if any
	// action2 = the suspended action's previously suspended action, if any
	
	// CHANGE_TO:
	// action1 = action which ended for us
	// action2 = the ended action's previously suspended action, if any
	
	
	if (actor->IsDebugging(DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_LTYELLOW, "%3.2f: %s:%s: ",
			gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->m_strName());
		
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_GREEN, " STARTING ");
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_WHITE, this->GetName());
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_WHITE, "\n");
	}
	
	this->m_bStarted = true;
	this->m_Actor    = actor;
	this->m_Behavior = behavior;
	
	Action<T> *parent;
	if (action1 != nullptr) {
		parent = action1->m_ActionParent;
		this->m_ActionParent = parent;
	} else {
		parent = this->m_ActionParent;
	}
	
	if (parent != nullptr) {
		parent->m_ActionChild = this;
	}
	
	this->m_ActionWeSuspended = action2;
	if (action2 != nullptr) {
		action2->m_ActionSuspendedUs = this;
	}
	
	this->m_ActionSuspendedUs = nullptr;
	
	Action<T> *child = this->InitialContainedAction(actor);
	this->m_ActionChild = child;
	if (this->m_ActionChild != nullptr) {
		child->m_ActionParent = this;
		this->m_ActionChild = this->m_ActionChild->ApplyResult(actor, behavior,
			ActionResult<T>::ChangeTo(this->m_ActionChild, "Starting child Action"));
	}
	
	return this->OnStart(actor, action1);
}

template<class T> ActionResult<T> Action<T>::InvokeUpdate(T *actor, Behavior<T> *behavior, float dt)
{
	// dt = (INextBotComponent *)behavior->m_flTickInterval
	
	
	Action<T> *suspended = this;
	while ((suspended = suspended->m_ActionWeSuspended) != nullptr) {
		if (suspended->m_Result.transition == ActionTransition::CHANGE_TO ||
			suspended->m_Result.transition == ActionTransition::DONE) {
			return ActionResult<T>::Done("Out of scope");
		}
	}
	
	if (this->m_bStarted) {
		if (this->m_Result.transition > ActionTransition::CONTINUE) {
			suspended = this;
			while (suspended != nullptr) {
				if (suspended->m_Result.transition == ActionTransition::SUSPEND_FOR) {
					ActionResult<T> result = suspended->m_Result;
					this->m_Result = { 0, nullptr, nullptr, 0 };
					return result;
				}
				
				suspended = suspended->m_ActionWeSuspended;
			}
			
			if (this->m_ActionChild != nullptr) {
				ActionResult<T> result = this->m_ActionChild->InvokeUpdate(actor, behavior, dt);
				this->m_ActionChild = this->m_ActionChild->ApplyResult(actor, behavior, result);
			}
			
			VPROF_BUDGET(this->GetName(), "NextBot");
			return this->Update(actor, dt);
		} else {
			ActionResult<T> result = this->m_Result;
			this->m_Result = { 0, nullptr, nullptr, 0 };
			return result;
		}
	} else {
		return ActionResult<T>::ChangeTo(this, "Starting Action");
	}
}

template<class T> void Action<T>::InvokeOnEnd(T *actor, Behavior<T> *behavior, Action<T> *action)
{
	// DONE:
	// action = the action we previously suspended, if any
	
	// CHANGE_TO:
	// action = the action we're changing to
	
	
	if (this->m_bStarted) {
		if (actor->IsDebugging(DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_LTYELLOW, "%3.2f: %s:%s: ",
				gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->m_strName());
			
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_MAGENTA, " ENDING ");
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_WHITE, this->GetName());
			actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_WHITE, "\n");
		}
		
		this->m_bStarted = false;
		
		if (this->m_ActionChild != nullptr) {
			Action<T> *suspended = this->m_ActionChild;
			do {
				suspended->InvokeOnEnd(actor, behavior, action);
				suspended = suspended->m_ActionWeSuspended;
			} while (suspended != nullptr);
		}
		
		this->OnEnd(actor, action);
		
		if (this->m_ActionSuspendedUs != nullptr) {
			this->m_ActionSuspendedUs->InvokeOnEnd(actor, behavior, action);
		}
	}
}


template<class T> ActionResult<T> Action<T>::InvokeOnResume(T *actor, Behavior<T> *behavior, Action<T> *action)
{
	// DONE:
	// action = the action that previously suspended us, who just ended
	
	
	if (actor->IsDebugging(DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_LTYELLOW, "%3.2f: %s:%s: ",
			gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->m_strName());
		
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_MAGENTA, " RESUMING ");
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_WHITE, this->GetName());
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_WHITE, "\n");
	}
	
	if (this->m_bSuspended && this->m_Result.transition > ActionTransition::DONE) {
		this->m_bSuspended = false;
		this->m_ActionSuspendedUs = nullptr;
		
		if (this->m_ActionParent != nullptr) {
			this->m_ActionParent->m_ActionChild = this;
		}
		
		if (this->m_ActionChild != nullptr) {
			ActionResult<T> result = this->m_ActionChild->InvokeOnResume(actor, behavior, action);
			this->m_ActionChild = this->m_ActionChild->ApplyResult(actor, behavior, result);
		}
		
		return this->OnResume(actor, action);
	} else {
		return ActionResult<T>::Continue();
	}
}

template<class T> Action<T> *Action<T>::InvokeOnSuspend(T *actor, Behavior<T> *behavior, Action<T> *action)
{
	// SUSPEND_FOR:
	// action = the action we're suspending for
	
	
	if (actor->IsDebugging(DEBUG_BEHAVIOR) || NextBotDebugHistory.GetBool()) {
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_LTYELLOW, "%3.2f: %s:%s: ",
			gpGlobals->curtime, actor->GetDebugIdentifier(), behavior->m_strName());
		
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_MAGENTA, " SUSPENDING ");
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_WHITE, this->GetName());
		actor->DebugConColorMsg(DEBUG_BEHAVIOR,
			COLOR_WHITE, "\n");
	}
	
	if (this->m_ActionChild != nullptr) {
		this->m_ActionChild = this->m_ActionChild->InvokeOnSuspend(actor, behavior, action);
	}
	
	this->m_bSuspended = true;
	
	ActionResult<T> result = this->OnSuspend(actor, action);
	if (result.transition == ActionTransition::DONE) {
		this->InvokeOnEnd(actor, behavior, nullptr);
		behavior->DestroyAction(this);
		return this->m_ActionWeSuspended;
	} else {
		return this;
	}
}


template<class T> char *Action<T>::BuildDecoratedName(char buf[256], const Action<T> *action) const
{
	V_strncat(buf, action->GetName(), 256);
	
	if (action->m_ActionChild != nullptr) {
		V_strncat(buf, "( ", 256);
		this->BuildDecoratedName(buf, action->m_ActionChild);
		V_strncat(buf, " )", 256);
	}
	
	if (action->m_ActionWeSuspended != nullptr) {
		V_strncat(buf, "<<", 256);
		this->BuildDecoratedName(buf, action->m_ActionWeSuspended);
	}
	
	return buf;
}

template<class T> char *Action<T>::DebugString() const
{
	static char str[256];
	str[0] = '\0';
	
	Action<T> *action = this;
	while (action->m_ActionParent != nullptr) {
		action = action->m_ActionParent;
	}
	
	return this->BuildDecoratedName(str, action);
}

template<class T> void Action<T>::PrintStateToConsole() const
{
	/* this is a huge pointless mess...
	 * it looks like it was intended to colorize the action hierarchy */
	
	const char *str = this->DebugString();
	char buf[256];
	
	int flag = 0;
	char *p_buf = buf;
	
	while (*str != '\0') {
		*p_buf = *str;
		if (*str == '(') {
			*(p_buf + 1) = '\0';
			if (flag >= 1) {
				++flag;
			}
			DevMsg("%s", buf);
			p_buf = buf;
		} else if (*str == ')') {
			*p_buf = '\0';
			if (flag >= 1) {
				--flag;
			}
			DevMsg("%s", buf);
			buf[0] = ')';
			p_buf = buf + 1;
		} else if (*str == '<' && !flag) {
			*(p_buf + 1) = '<';
			flag = 1;
			*(p_buf + 2) = '\0';
			DevMsg("%s", buf);
			p_buf = buf;
			++str;
		} else {
			++p_buf;
		}
		
		++str;
	}
	
	*p_buf = '\0';
	DevMsg("%s", buf);
	DevMsg("\n\n");
}


/* this was invented by me to avoid repeating event handler code a hundred times */
template<class T> void Action<T>::HandleEvent(const char *name, const std::function<EventDesiredResult<T> (Action<T> *, T *)>& handler)
{
	if (!this->m_bStarted) {
		return;
	}
	
	EventDesiredResult<T> result;
	
	Action<T> *action = this;
	do {
		if (action->m_Actor != nullptr) {
			if (action->m_Actor->IsDebugging(DEBUG_EVENTS) || NextBotDebugHistory.GetBool()) {
				action->m_Actor->DebugConColorMsg(DEBUG_EVENTS,
					COLOR_GRAY100, "%3.2f: %s:%s: %s received EVENT %s\n",
					gpGlobals->curtime, action->m_Actor->GetDebugIdentifier(), this->m_Behavior->m_strName(),
					action->GetFullName(), name);
			}
		}
		
		result = handler(action, action->m_Actor);
		
		action = action->m_ActionWeSuspended;
		if (action == nullptr) {
			return;
		}
	} while (result.transition != ActionTransition::CONTINUE);
	
	if (action->m_Actor != nullptr) {
		if (action->m_Actor->IsDebugging(DEBUG_BEHAVIOR) ||
			(NextBotDebugHistory.GetBool() && result.transition <= ActionTransition::DONE)) {
			action->m_Actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_YELLOW, "%3.2f: %s:%s: ",
				gpGlobals->curtime, action->m_Actor->GetDebugIdentifier(), this->m_Behavior->m_strName());
			
			action->m_Actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_WHITE, "%s ", action->GetFullName());
			action->m_Actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_YELLOW, "responded to EVENT %s with ",
				name);
			
			const char *s_action = "";
			if (result.action != nullptr) {
				s_action = result.action->GetName();
			}
			
			const char *s_transition;
			switch (result.transition) {
			case ActionTransition::CHANGE_TO:
				s_transition = "CHANGE_TO";
				break;
			case ActionTransition::SUSPEND_FOR:
				s_transition = "SUSPEND_FOR";
				break;
			case ActionTransition::DONE:
				s_transition = "DONE";
				break;
			case ActionTransition::SUSTAIN:
				s_transition = "SUSTAIN";
				break;
			}
			
			action->m_Actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_RED, "%s %s ",
				s_transition, s_action);
			
			const char *s_reason = "";
			if (result.reason != nullptr) {
				s_reason = result.reason;
			}
			
			action->m_Actor->DebugConColorMsg(DEBUG_BEHAVIOR,
				COLOR_GREEN, "%s\n", s_reason);
		}
	}
	
	action->StorePendingEventResult(result, name);
}

/* this function is currently inlined, but it can be found in the L4D binary
 * as well as the 20130214 OSX TF2 server binary */
template<class T> void Action<T>::StorePendingEventResult(const EventDesiredResult<T>& result, const char *event)
{
	if (result.severity < action->m_Result.severity) {
		if (result.action != nullptr) {
			delete result.action;
		}
	} else {
		if (action->m_Result.severity == ResultSeverity::CRITICAL && developer.GetInt() != 0) {
			DevMsg("%3.2f: WARNING: %s::%s() RESULT_CRITICAL collision\n",
				gpGlobals->curtime, action->GetName(), name);
		}
		
		if (action->m_Result.action != nullptr) {
			delete action->m_Result.action;
		}
		
		action->m_Result = result;
	}
}
