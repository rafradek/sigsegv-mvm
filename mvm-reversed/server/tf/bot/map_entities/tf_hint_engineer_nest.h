/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/map_entities/tf_hint_engineer_nest.h
 */


// sizeof: 0x398
class CTFBotHintEngineerNest : public CBaseTFBotHintEntity
{
public:
	CTFBotHintEngineerNest();
	virtual ~CTFBotHintEngineerNest();
	
	virtual int UpdateTransmitState() override;
	
	virtual void Spawn() override;
	
	virtual HintType GetHintType() const override;
	
	void DetonateStateNest();
	
	CTFBotHintSentrygun *GetSentryHint() const;
	CTFBotHintTeleporterExit *GetTeleporterHint() const;
	
	bool IsStaleNest() const;
	
private:
	void HintThink();
	void HintTeleporterThink();
	
	void DetonateObjectsFromHints(const CUtlVector<CHandle<CBaseTFBotHintEntity>>& hints);
	CBaseTFBotHintEntity *GetHint(const CUtlVector<CHandle<CBaseTFBotHintEntity>>& hints);
	
	CUtlVector<CHandle<CBaseTFBotHintEntity>> m_SentryHints; // +0x36c
	CUtlVector<CHandle<CBaseTFBotHintEntity>> m_TeleHints;   // +0x380
	CNetworkVar(bool, m_bHasActiveTeleporter);               // +0x394
};
