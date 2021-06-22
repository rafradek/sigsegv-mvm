/* reverse engineering by sigsegv
 * based on TF2 version 20160428a
 * server/tf/player_vs_environment/tf_population_manager.h
 */


// TODO: class/struct CMvMBotUpgrade (sizeof: 0x54 probably)
// (which file does this belong in? is it shared with the client?)

// TODO: which file does this belong in? is it shared with the client?
struct CUpgradeInfo
{
	int class_index;  // +0x00
	short item_index; // +0x04
	int upgrade_tier; // +0x08
	int upgrade_cost; // +0x0c
};


// sizeof: 0x724
class CPopulationManager : public CPointEntity, public CGameEventListener
{
public:
	struct PlayerUpgradeHistory
	{
		// 00 uint64_t steamid
		// 08 CUtlVector<CUpgradeInfo>
		// 1c int, returned by GetPlayerCurrencySpent
	};
	
	struct CheckpointSnapshotInfo
	{
		// 00 CSteamID
		// 08 int, same as PlayerUpgradeHistory+0x1c
		// 0c CUtlVector<CUpgradeInfo>
	};
	
	CPopulationManager();
	virtual ~CPopulationManager();
	
	/* CBaseEntity overrides */
	virtual void Spawn() override;
	virtual void Precache() override;
	
	/* CGameEventListener overrides */
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual void Reset();
	virtual bool Initialize();
	
	void AddRespecToPlayer(CTFPlayer *player);
	void AddPlayerCurrencySpent(CTFPlayer *player, int amount);
	void AdjustMinPlayerSpawnTime();
	void ClearCheckpoint();
	int CollectMvMBots(CUtlVector<CTFPlayer *> *bots);
	void CycleMission();
	void DebugWaveStats();
	void EndlessFlagHasReset();
	void EndlessSetAttributesForBot(CTFBot *bot);
	bool EndlessShouldResetFlag();
	void ForgetOtherBottleUpgrades(CTFPlayer *player, CEconItemView *bottle, int i1);
	UNKNOWN GetCurrentWave(); // probably returns CWave *
	float GetDamageMultiplier();
	float GetHealthMultiplier(bool b1);
	int GetNumRespecsAvailableForPlayer(CTFPlayer *player);
	int GetPlayerCurrencySpent(CTFPlayer *player);
	UNKNOWN GetPlayerUpgradeHistory(CTFPlayer *player); // probably returns PlayerUpgradeHistory * or CUtlVector<CUpgradeInfo> *
	const char *GetPopulationFilename();
	const char *GetPopulationFilenameShort();
	void GetSentryBusterDamageAndKillThreshold(int& i1, int& i2) const;
	int GetTotalPopFileCurrency();
	bool HasEventChangeAttributes(const char *name) const;
	bool IsInEndlessWaves();
	bool IsValidMvMMap(const char *name);
	void JumpToWave(unsigned int i1, float f1);
	void LoadMissionCycleFile();
	void MvMVictory();
	void OnCurrencyCollected(int i1, bool b1, bool b2);
	void OnCurrencyPackFade();
	void OnPlayerKilled(CTFPlayer *player);
	void PauseSpawning();
	void PlayerDoneViewingLoot(const CTFPlayer *player);
	void RemovePlayerAndItemUpgradesFromHistory(CTFPlayer *player);
	void RemoveRespecFromPlayer(CTFPlayer *player);
	void ResetMap();
	void RestorePlayerCurrency();
	void SendUpgradesToPlayer(CTFPlayer *player);
	void SetCheckpoint(int i1);
	void SetNumRespecsForPlayer(CTFPlayer *player, int num);
	void SetPopulationFilename(const char *name);
	void SetupOnRoundStart();
	void StartCurrentWave();
	void UnpauseSpawning();
	void Update();
	void WaveEnd(bool b1);
	
private:
	void AllocateBots();
	void EndlessParseBotUpgrades();
	void EndlessRollEscalation();
	UNKNOWN FindCheckpointSnapshot(CSteamID steamid) const; // probably returns CheckpointSnapshotInfo *
	UNKNOWN FindCheckpointSnapshot(CTFPlayer *player) const; // probably returns CheckpointSnapshotInfo *
	PlayerUpgradeHistory *FindOrAddPlayerUpgradeHistory(CSteamID steamid);
	PlayerUpgradeHistory *FindOrAddPlayerUpgradeHistory(CTFPlayer *player);
	void LoadLastKnownMission();
	bool LoadMvMMission(KeyValues *kv);
	bool Parse();
	void PostInitialize();
	void ResetRespecPoints();
	void RestoreCheckpoint();
	void RestoreItemToCheckpointState(CTFPlayer *player, CEconItemView *item);
	void ShowNextWaveDescription();
	void UpdateObjectiveResource();
	
	// TODO: re-check functions
	
	// +0x000: vtable CBaseEntity
	// +0x360: vtable CGameEventListener
	
	// TODO: sizeof went from 0x6fc to 0x724
	
	// INSERT 4 bytes (from CBaseEntity)
	
	// 364 byte
	CUtlVectorAutoPurge<IPopulator *> m_Populators; // +0x368 (used for MissionPopulators and RandomPlacementPopulators)
	char m_szPopFileNameFull[MAX_PATH];             // +0x37c
	char m_szPopFileNameShort[MAX_PATH];            // +0x480
	KeyValues *m_kvTemplates; // +0x584
	// 588 byte
	// 589 byte
	// 58a byte
	// 58b?
	// 58c CHandle<?> (at least a CBaseEntity)
	int m_iStartingCurrency;               // +0x590 (from popfile)
	bool m_bEventPopfile;                  // +0x594 (from popfile)
	int m_iRespawnWaveTime;                // +0x598 (from popfile)
	bool m_bFixed;                         // +0x59c (from popfile)
	bool m_bCanBotsAttackWhileInSpawnRoom; // +0x59d (from popfile)
	// 59e?
	// 59f?
	int m_iSentryBusterThresholdDamage;    // +0x5a0
	int m_iSentryBusterThresholdKills;     // +0x5a4
	int m_iCurrentWave;                    // +0x5a8
	CUtlVector<CWave *> m_Waves;           // +0x5ac
	// 5c0 
	CUtlVector<PlayerUpgradeHistory *> m_UpgradeHistory; // +0x5c4
	// 5d8 bool
	bool m_bAdvanced; // +0x5d9 (from popfile)
	// 5da?
	// 5db?
	CMannVsMachineStats *m_pMvMStats; // +0x5dc
	KeyValues *m_kvMissionCycle; // +0x5e0
	// 5e4?
	bool m_bIsPaused;  // +0x5e5
	// 5e6?
	bool m_bIsEndless; // +0x5e7
	// 5e8 CUtlVector<CMvMBotUpgrade> something related to endless bot upgrades
	// 5fc CUtlVector<CMvMBotUpgrade> something related to endless bot upgrades
	// 610 CUniformRandomStream (sizeof: 0x94)
	// 6a4 CUtlVector<int>
	// 6b8 
	// 6bc CUtlVector<const CTFPlayer *> (used in PlayerDoneViewingLoot)
	// 6d0 CUtlString, set to "Default" in reset, almost undoubtably the current eventchangeattributes set name
	// 6d4 CUtlMap<uint64_t, int>, to store number of respec credits for each player
	// 6d8 ptr to something of size 0x294
	// 6dc 
	// 6e0 
	// 6e4 
	// 6e8 
	// 6ec 
	// 6f0 
	// 6f4 dword (number of respecs possibly)
	// 6f8 dword (respec credit progress possibly)
	
	static int m_nNumConsecutiveWipes = 0;
	static int m_checkpointWaveIndex = 0;
	static CUtlVector<CheckpointSnapshotInfo *> m_checkpointSnapshot;
};


// TODO: HaveMap(const char *)
// TODO: MvMSkillChangedCallback
// TODO: MinibossScaleChangedCallback
// TODO: MvMMissionCycleFileChangedCallback
