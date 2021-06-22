/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotEventResponderInterface.h
 */


#define FOR_EACH_RESPONDER \
	for (auto responder = this->FirstContainedResponder(); \
		responder != nullptr; \
		responder = this->NextContainedResponder(responder))


class INextBotEventResponder
{
public:
	enum class MoveToFailureType : int
	{
		// not sure if these are 100% valid (they're from CImprovLocomotor)
		FAIL_INVALID_PATH = 0,
		FAIL_STUCK        = 1,
		FAIL_FELL_OFF     = 2,
	};
	
	virtual ~INextBotEventResponder();
	
	virtual INextBotEventResponder *FirstContainedResponder() const;
	virtual INextBotEventResponder *NextContainedResponder(INextBotEventResponder *prev) const;
	
	virtual void OnLeaveGround(CBaseEntity *ent);
	virtual void OnLandOnGround(CBaseEntity *ent);
	
	virtual void OnContact(CBaseEntity *ent, CGameTrace *trace);
	
	virtual void OnMoveToSuccess(const Path *path);
	virtual void OnMoveToFailure(const Path *path, MoveToFailureType fail);
	
	virtual void OnStuck();
	virtual void OnUnStuck();
	
	virtual void OnPostureChanged();
	virtual void OnAnimationActivityComplete(int i1);
	virtual void OnAnimationActivityInterrupted(int i1);
	virtual void OnAnimationEvent(animevent_t *a1);
	
	virtual void OnIgnite();
	virtual void OnInjured(const CTakeDamageInfo& info);
	virtual void OnKilled(const CTakeDamageInfo& info);
	virtual void OnOtherKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info);
	
	virtual void OnSight(CBaseEntity *ent);
	virtual void OnLostSight(CBaseEntity *ent);
	
	virtual void OnSound(CBaseEntity *ent, const Vector& v1, KeyValues *kv);
	virtual void OnSpokeConcept(CBaseCombatCharacter *who, const char *s1, AI_Response *response);
	virtual void OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon);
	
	virtual void OnNavAreaChanged(CNavArea *area1, CNavArea *area2);
	virtual void OnModelChanged();
	virtual void OnPickUp(CBaseEntity *ent, CBaseCombatCharacter *who);
	virtual void OnDrop(CBaseEntity *ent);
	virtual void OnActorEmoted(CBaseCombatCharacter *who, int concept);
	
	virtual void OnCommandAttack(CBaseEntity *ent);
	virtual void OnCommandApproach(const Vector& v1, float f1);
	virtual void OnCommandApproach(CBaseEntity *ent);
	virtual void OnCommandRetreat(CBaseEntity *ent, float f1);
	virtual void OnCommandPause(float f1);
	virtual void OnCommandResume();
	virtual void OnCommandString(const char *cmd);
	
	virtual void OnShoved(CBaseEntity *ent);
	virtual void OnBlinded(CBaseEntity *ent);
	
	virtual void OnTerritoryContested(int i1);
	virtual void OnTerritoryCaptured(int i1);
	virtual void OnTerritoryLost(int i1);
	
	virtual void OnWin();
	virtual void OnLose();
};
