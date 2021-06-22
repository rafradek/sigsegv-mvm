/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotEventResponderInterface.cpp
 */


// TODO: these should all actually be inline definitions in NextBotEventResponderInterface.h
// and this file shouldn't exist


INextBotEventResponder::~INextBotEventResponder()
{
}


INextBotEventResponder *INextBotEventResponder::FirstContainedResponder() const
{
	return nullptr;
}

INextBotEventResponder *INextBotEventResponder::NextContainedResponder(INextBotEventResponder *prev) const
{
	return nullptr;
}


void INextBotEventResponder::OnLeaveGround(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnLeaveGround(ent);
	}
}

void INextBotEventResponder::OnLandOnGround(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnLandOnGround(ent);
	}
}


void INextBotEventResponder::OnContact(CBaseEntity *ent, CGameTrace *trace)
{
	FOR_EACH_RESPONDER {
		responder->OnContact(ent, trace);
	}
}


void INextBotEventResponder::OnMoveToSuccess(const Path *path)
{
	FOR_EACH_RESPONDER {
		responder->OnMoveToSuccess(path);
	}
}

void INextBotEventResponder::OnMoveToFailure(const Path *path, MoveToFailureType fail)
{
	FOR_EACH_RESPONDER {
		responder->OnMoveToFailure(path, fail);
	}
}


void INextBotEventResponder::OnStuck()
{
	FOR_EACH_RESPONDER {
		responder->OnStuck();
	}
}

void INextBotEventResponder::OnUnStuck()
{
	FOR_EACH_RESPONDER {
		responder->OnUnStuck();
	}
}


void INextBotEventResponder::OnPostureChanged()
{
	FOR_EACH_RESPONDER {
		responder->OnPostureChanged();
	}
}

void INextBotEventResponder::OnAnimationActivityComplete(int i1)
{
	FOR_EACH_RESPONDER {
		responder->OnAnimationActivityComplete(i1);
	}
}

void INextBotEventResponder::OnAnimationActivityInterrupted(int i1)
{
	FOR_EACH_RESPONDER {
		responder->OnAnimationActivityInterrupted(i1);
	}
}

void INextBotEventResponder::OnAnimationEvent(animevent_t *a1)
{
	FOR_EACH_RESPONDER {
		responder->OnAnimationEvent(a1);
	}
}


void INextBotEventResponder::OnIgnite()
{
	FOR_EACH_RESPONDER {
		responder->OnIgnite();
	}
}

void INextBotEventResponder::OnInjured(const CTakeDamageInfo& info)
{
	FOR_EACH_RESPONDER {
		responder->OnInjured(info);
	}
}

void INextBotEventResponder::OnKilled(const CTakeDamageInfo& info)
{
	FOR_EACH_RESPONDER {
		responder->OnKilled(info);
	}
}

void INextBotEventResponder::OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info)
{
	FOR_EACH_RESPONDER {
		responder->OnOtherKilled(who, info);
	}
}


void INextBotEventResponder::OnSight(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnSight(ent);
	}
}

void INextBotEventResponder::OnLostSight(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnLostSight(ent);
	}
}


void INextBotEventResponder::OnSound(CBaseEntity *ent, const Vector& v1, KeyValues *kv)
{
	FOR_EACH_RESPONDER {
		responder->OnSound(ent, v1, kv);
	}
}

void INextBotEventResponder::OnSpokeConcept(CBaseCombatCharacter *who, const char *s1, AI_Response *response)
{
	FOR_EACH_RESPONDER {
		responder->OnSpokeConcept(who, s1, response);
	}
}

void INextBotEventResponder::OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon)
{
	FOR_EACH_RESPONDER {
		responder->OnWeaponFired(who, weapon);
	}
}


void INextBotEventResponder::OnNavAreaChanged(CNavArea *area1, CNavArea *area2)
{
	FOR_EACH_RESPONDER {
		responder->OnNavAreaChanged(area1, area2);
	}
}

void INextBotEventResponder::OnModelChanged()
{
	FOR_EACH_RESPONDER {
		responder->OnModelChanged();
	}
}

void INextBotEventResponder::OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who)
{
	FOR_EACH_RESPONDER {
		responder->OnPickUp(ent, who);
	}
}

void INextBotEventResponder::OnDrop(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnDrop(ent);
	}
}

void INextBotEventResponder::OnActorEmoted(CBaseCombatCharacter *who, int concept)
{
	FOR_EACH_RESPONDER {
		responder->OnActorEmoted(who, concept);
	}
}


void INextBotEventResponder::OnCommandAttack(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnCommandAttack(ent);
	}
}

void INextBotEventResponder::OnCommandApproach(const Vector& v1, float f1)
{
	FOR_EACH_RESPONDER {
		responder->OnCommandApproach(v1, f1);
	}
}

void INextBotEventResponder::OnCommandApproach(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnCommandApproach(ent);
	}
}

void INextBotEventResponder::OnCommandRetreat(CBaseEntity *ent, float f1)
{
	FOR_EACH_RESPONDER {
		responder->OnCommandRetreat(ent, f1);
	}
}

void INextBotEventResponder::OnCommandPause(float f1)
{
	FOR_EACH_RESPONDER {
		responder->OnCommandPause(f1);
	}
}

void INextBotEventResponder::OnCommandResume()
{
	FOR_EACH_RESPONDER {
		responder->OnCommandResume();
	}
}

void INextBotEventResponder::OnCommandString(const char *cmd)
{
	FOR_EACH_RESPONDER {
		responder->OnCommandString(cmd);
	}
}


void INextBotEventResponder::OnShoved(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnShoved(ent);
	}
}

void INextBotEventResponder::OnBlinded(CBaseEntity *ent)
{
	FOR_EACH_RESPONDER {
		responder->OnBlinded(ent);
	}
}


void INextBotEventResponder::OnTerritoryContested(int i1)
{
	FOR_EACH_RESPONDER {
		responder->OnTerritoryContested(i1);
	}
}

void INextBotEventResponder::OnTerritoryCaptured(int i1)
{
	FOR_EACH_RESPONDER {
		responder->OnTerritoryCaptured(i1);
	}
}

void INextBotEventResponder::OnTerritoryLost(int i1)
{
	FOR_EACH_RESPONDER {
		responder->OnTerritoryLost(i1);
	}
}


void INextBotEventResponder::OnWin()
{
	FOR_EACH_RESPONDER {
		responder->OnWin();
	}
}

void INextBotEventResponder::OnLose()
{
	FOR_EACH_RESPONDER {
		responder->OnLose();
	}
}
