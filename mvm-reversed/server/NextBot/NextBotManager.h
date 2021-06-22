/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotManager.h
 */


class NextBotManager
{
public:
	struct DebugFilter
	{
		// TODO
		// sizeof: 0x84
		
		// seems to have:
		// 00 int
		// 04 char[0x80]
	};
	
	NextBotManager();
	virtual ~NextBotManager();
	
	virtual void Update();
	virtual void OnMapLoaded();
	virtual void OnRoundRestart();
	virtual void OnBeginChangeLevel();
	
	virtual void OnKilled(CBaseCombatCharacter *who, const CTakeDamageInfo& info);
	virtual void OnSound(CBaseEntity *ent, const Vector& where, KeyValues *kv);
	virtual void OnSpokeConcept(CBaseCombatCharacter *who, const char *s1, AI_Response *a1);
	virtual void OnWeaponFired(CBaseCombatCharacter *who, CBaseCombatWeapon *weapon);
	
	void CollectAllBots(CUtlVector<INextBot *> *nextbots);
	
	INextBot *GetBotUnderCrosshair(CBasePlayer *player);
	
	void DebugFilterAdd(const char *s1);
	void DebugFilterAdd(int i1);
	void DebugFilterClear();
	void DebugFilterRemove(const char *s1);
	void DebugFilterRemove(int i1);
	bool IsDebugFilterMatch(const INextBot *nextbot) const;
	
	bool ShouldUpdate(INextBot *nextbot);
	void NotifyBeginUpdate(INextBot *nextbot);
	void NotifyEndUpdate(INextBot *nextbot);
	
	int Register(INextBot *nextbot);
	void UnRegister(INextBot *nextbot);
	
	static NextBotManager *sInstance;
	
	INextBot *GetSelectedBot() const { return this->m_SelectedBot; }
	
	// TODO: ForEachCombatCharacter
	// TODO: ForEachBot
	// (see: L4D)
	
protected:
	void Reset();
	
	#define PAD(n, x) char n[x]
	
	CUtlLinkedList<INextBot *> m_NextBots; // +0x04
	int m_iTickRate;                       // +0x20
	// 24 double (NotifyEndUpdate)
	// 2c double (ShouldUpdate)
	
	PAD(pad1, 0x10);
	
	unsigned int m_nDebugMask;              // +0x34
	CUtlVector<DebugFilter> m_DebugFilters; // +0x38
	INextBot *m_SelectedBot;                // +0x4c
};


NextBotManager& TheNextBots();


// TODO: TargetScan
