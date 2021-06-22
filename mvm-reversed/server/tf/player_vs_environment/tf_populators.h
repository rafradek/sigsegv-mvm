/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_populators.h
 */


struct EventInfo
{
	CFmtStrN<256> target; // +0x000
	CFmtStrN<256> action; // +0x10c
};


enum SpawnResult
{
	SPAWN_FAIL     = 0,
	SPAWN_NORMAL   = 1,
	SPAWN_TELEPORT = 2,
};


class CSpawnLocation
{
public:
	enum class Where : int
	{
		TEAMSPAWN = 0,
		AHEAD     = 1,
		BEHIND    = 2,
		ANYWHERE  = 3,
	};
	
	CSpawnLocation();
	
	bool Parse(KeyValues *kv);
	SpawnResult FindSpawnLocation(Vector& vec) const;
	
private:
	CTFNavArea *SelectSpawnArea() const;
	
	Where m_iWhere;                            // +0x00
	CUtlVector<CHandle<CBaseEntity>> m_Spawns; // +0x04
};


class IPopulator
{
public:
	IPopulator(CPopulationManager *popmgr);
	virtual ~IPopulator();
	
	virtual bool Parse(KeyValues *kv) = 0;
	virtual void PostInitialize();
	virtual void Update();
	virtual void UnpauseSpawning();
	virtual void OnPlayerKilled(CTFPlayer *player);
	virtual bool HasEventChangeAttributes(const char *name) const;
	
protected:
	IPopulationSpawner *m_Spawner; // +0x04
	CPopulationManager *m_PopMgr;  // +0x08
};


class CRandomPlacementPopulator : public IPopulator
{
public:
	CRandomPlacementPopulator(CPopulationManager *popmgr);
	virtual ~CRandomPlacementPopulator();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual void PostInitialize() override;
	
private:
	int m_iCount;                // +0x0c
	float m_flMinimumSeparation; // +0x10
	// +0x14 NavAreaFilter dword -1 TODO: enum for this?
};


class CPeriodicSpawnPopulator : public IPopulator
{
public:
	CPeriodicSpawnPopulator(CPopulationManager *popmgr);
	virtual ~CPeriodicSpawnPopulator();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual void PostInitialize() override;
	virtual void Update() override;
	virtual void UnpauseSpawning() override;
	
private:
	CSpawnLocation m_Where; // +0x0c
	float m_flMinInterval;  // +0x24
	float m_flMaxInterval;  // +0x28
	// +0x2c CountdownTimer
};


class CMissionPopulator : public IPopulator
{
public:
	enum class State : int
	{
		NOT_STARTED = 0, // mission not running yet
		INITIAL     = 1, // waiting for the initial cooldown period
		RUNNING     = 2, // running normally
	};
	
	CMissionPopulator(CPopulationManager *popmgr);
	virtual ~CMissionPopulator();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual void Update() override;
	virtual void UnpauseSpawning() override;
	
private:
	bool UpdateMission(CTFBot::MissionType mtype);
	bool UpdateMissionDestroySentries();
	
	CTFBot::MissionType m_Objective; // +0x0c
	CSpawnLocation m_Where;          // +0x10
	State m_iState;                  // +0x28
	float m_flInitialCooldown;       // +0x2c
	float m_flCooldownTime;          // +0x30
	CountdownTimer m_ctCooldown;     // +0x34 (used for initial cooldown and subsequent cooldowns for non-busters)
	CountdownTimer m_ctSBCooldown;   // +0x40 (used for subsequent cooldowns for busters)
	int m_iDesiredCount;             // +0x4c
	int m_iBeginAtWave;              // +0x50
	int m_iEndBeforeThisWave;        // +0x54
};


class CWaveSpawnPopulator : public IPopulator
{
public:
	CWaveSpawnPopulator(CPopulationManager *popmgr);
	virtual ~CWaveSpawnPopulator();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual void Update() override;
	virtual void OnPlayerKilled(CTFPlayer *player) override;
	
	bool IsFinishedSpawning();
	void OnNonSupportWavesDone();
	void ForceFinish();
	
	int GetCurrencyAmountPerDeath();
	
private:
	enum InternalStateType : int
	{
		INITIAL           = 0,
		PRE_SPAWN_DELAY   = 1,
		SPAWNING          = 2,
		WAIT_FOR_ALL_DEAD = 3,
		DONE              = 4,
	};
	
	void SetState(InternalStateType newstate);
	
	static int m_reservedPlayerSlotCount = 0;
	
	CSpawnLocation m_Where;                    // +0x00c
	int m_iTotalCount;                         // +0x024
	int m_iCountNotYetSpawned;                 // +0x028
	// +0x02c 
	int m_iMaxActive;                          // +0x030
	int m_iSpawnCount;                         // +0x034
	float m_flWaitBeforeStarting;              // +0x038
	float m_flWaitBetweenSpawns;               // +0x03c
	bool m_bWaitBetweenSpawnsAfterDeath;       // +0x040
	CFmtStrN<256> m_strStartWaveWarningSound;  // +0x044
	EventInfo *m_StartWaveOutput;              // +0x150
	CFmtStrN<256> m_strFirstSpawnWarningSound; // +0x154
	EventInfo *m_FirstSpawnOutput;             // +0x260
	CFmtStrN<256> m_strLastSpawnWarningSound;  // +0x264
	EventInfo *m_LastSpawnOutput;              // +0x370
	CFmtStrN<256> m_strDoneWarningSound;       // +0x374
	EventInfo *m_DoneOutput;                   // +0x480
	int m_iTotalCurrency;                      // +0x484
	int m_iCurrencyLeft;                       // +0x488
	CUtlString m_strName;                      // +0x48c
	CUtlString m_strWaitForAllSpawned;         // +0x490
	// +0x494 dword/float 0
	CountdownTimer m_ctSpawnDelay;             // +0x498
	CUtlVector<CHandle<CBaseEntity>> m_ActiveBots; // +0x4a4
	int m_iCountSpawned;                       // +0x4b8
	int m_iCountToSpawn;                       // +0x4bc
	bool m_bSupport;                           // +0x4c0
	bool m_bSupportLimited;                    // +0x4c1
	CWave *m_Wave;                             // +0x4c4
	InternalStateType m_iState;                // +0x4c8
	bool m_bRandomSpawn;                       // +0x4cc
	SpawnResult m_iSpawnResult;                // +0x4d0
	Vector m_vecSpawn;                         // +0x4d4
};


class CWave : public IPopulator
{
public:
	CWave(CPopulationManager *popmgr);
	virtual ~CWave();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual void Update() override;
	virtual void OnPlayerKilled(CTFPlayer *player) override;
	virtual bool HasEventChangeAttributes(const char *name) const override;
	
	void ForceFinish();
	void ForceReset();
	
	CWaveSpawnPopulator *FindWaveSpawnPopulator(char const *name);
	void AddClassType(string_t icon, int count, unsigned int flags);
	
private:
	struct WaveClassCount_t
	{
		int count;          // +0x00
		string_t icon;      // +0x04
		unsigned int flags; // +0x08
	};
	
	void ActiveWaveUpdate();
	void WaveCompleteUpdate();
	void WaveIntermissionUpdate();
	
	bool IsDoneWithNonSupportWaves();
	
	CUtlVectorAutoPurge<CWaveSpawnPopulator *> m_WaveSpawns;
	// +0x020 byte 0
	bool m_bFiredInitWaveOutput;    // +0x021
	int m_iTotalCountNonSupport;    // +0x024
	int m_iTanksSpawned;            // +0x028
	int m_iSentryBustersSpawned;    // +0x02c
	int m_iEngiesTeleportedIn;      // +0x030
	int m_iSentryBustersKilled;     // +0x034
	CUtlVector<WaveClassCount_t> m_ClassCounts; // +0x038
	int m_iTotalCurrency;           // +0x04c
	EventInfo *m_StartWaveOutput;   // +0x050
	EventInfo *m_DoneOutput;        // +0x054
	EventInfo *m_InitWaveOutput;    // +0x058
	CFmtStrN<256> m_strDescription; // +0x05c
	CFmtStrN<256> m_strSound;       // +0x168
	float m_flWaitWhenDone;         // +0x274
	// +0x278 CountdownTimer
	// +0x284 byte 1
	// +0x288 IntervalTimer
	// +0x28c byte 0
	// +0x290 CountdownTimer
	// +0x29c byte 0
	// +0x2a0 float
};


EventInfo *ParseEvent(KeyValues *kv);
void FireEvent(EventInfo *info, const char *name);

SpawnResult DoTeleporterOverride(CBaseEntity *teamspawn, Vector& vec);
void OnBotTeleported(CTFBot *bot);
