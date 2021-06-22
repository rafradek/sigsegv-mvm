/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_vision.h
 */


class CTFBotVision : public IVision
{
public:
	CTFBotVision(INextBot *nextbot);
	virtual ~CTFBotVision();
	
	virtual void Update() override;
	
	virtual void CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents) override;
	virtual float GetMaxVisionRange() const override;
	virtual float GetMinRecognizeTime() const override;
	virtual bool IsIgnored(CBaseEntity *ent) const override;
	virtual bool IsVisibleEntityNoticed(CBaseEntity *ent) const override;
	
private:
	void UpdatePotentiallyVisibleNPCVector();
	
	/* offsets 0x64-0xc3 seem to be completely unused */
	CUtlVector<CHandle<CBaseCombatCharacter>> m_PVNPCs; // +0xc4
	CountdownTimer m_ctUpdatePVNPCs;                    // +0xd8
	CountdownTimer m_ctUpdate;                          // +0xe4
};
