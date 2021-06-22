/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/tf_mann_vs_machine_stats.h
 */


enum eMannVsMachineEvent : int
{
	// 0 ?
	// 1 ?
	// 2 ?
	// 3 PlayerEvent_BoughtInstantRespawn
	// 4 PlayerEvent_Upgraded (bottle)
	// 5 PlayerEvent_Upgraded (non-bottle)
	// 6 NotifyPlayerActiveUpgradeCosts
};


/* used in array in CMannVsMachineStats */
struct ???
{
	// 00 deaths
	// 04 damage to bots
	// 08 damage to giants
	// 0c damage to tanks
};


// sizeof: 0x1c
class CMannVsMachineWaveStats
{
public:
	UNKNOWN ClearStats();
	
	// 0x04 int m_nCreditsDropped
	// 0x08 int m_nCreditsAcquired
	// 0x0c int m_nCreditsBonus
	// 0x10 int m_nPlayerDeaths
	// 0x14 int m_nBuyBacks
	// 0x18 dword ?
};


// sizeof: 0x0c
struct CPlayerWaveSpendingStats
{
	int buyback_spending; // +0x00
	int bottle_spending;  // +0x04
	int upgrade_spending; // +0x08
};


class CMannVsMachineStats : public CBaseEntity
{
public:
	CMannVsMachineStats();
	virtual ~CMannVsMachineStats();
	
	UNKNOWN ObjectCaps();
	UNKNOWN UpdateTransmitState();
	
	void SetCurrentWave(unsigned int wave);
	void ResetStats();
	void OnStatsChanged();
	void RoundOver(bool b1);
	void SW_ReportWaveSummary(int i1, bool b1);
	
	void ResetWaveStats();
	UNKNOWN GetWaveStats(int wave); // returns a struct by value
	int GetDroppedCredits(int wave);
	int GetAcquiredCredits(int wave, bool b1);
	int GetBonusCredits(int wave);
	int GetMissedCredits(int wave);
	
	CPlayerWaveSpendingStats *GetSpending(int wave, uint64_t player_id);
	int GetBuyBackSpending(CTFPlayer *player);
	int GetBottleSpending(CTFPlayer *player);
	int GetUpgradeSpending(CTFPlayer *player);
	void ClearCurrentPlayerWaveSpendingStats(int wave);
	
	bool IsRespecAvailableForPlayer(CTFPlayer *player);
	UNKNOWN AddRespecToPlayer(CTFPlayer *player);
	UNKNOWN RemoveRespecFromPlayer(CTFPlayer *player);
	
	void ResetPlayerEvents(CTFPlayer *player);
	void ResetUpgradeSpending(CTFPlayer *player);
	
	void SendUpgradesToPlayer(CTFPlayer *player, CUtlVector<CUpgradeInfo> *upgrades);
	void NotifyPlayerActiveUpgradeCosts(CTFPlayer *player, int i1);
	void NotifyPlayerEvent(CTFPlayer *player, unsigned int wave, eMannVsMachineEvent event, int i2, int i3);
	void NotifyTargetPlayerEvent(CTFPlayer *player, unsigned int wave, eMannVsMachineEvent event, int i2);
	
	void PlayerEvent_BoughtInstantRespawn(CTFPlayer *player, int cost);
	void PlayerEvent_DealtDamageToBots(CTFPlayer *player, int damage);
	void PlayerEvent_DealtDamageToGiants(CTFPlayer *player, int damage);
	void PlayerEvent_DealtDamageToTanks(CTFPlayer *player, int damage);
	void PlayerEvent_Died(CTFPlayer *player);
	void PlayerEvent_PickedUpCredits(CTFPlayer *player, unsigned int i1, int i2);
	void PlayerEvent_PointsChanged(CTFPlayer *player, int i1);
	void PlayerEvent_Upgraded(CTFPlayer *player, unsigned short i1, unsigned short i2, unsigned char i3, short i4, bool is_canteen);
	
	void RoundEvent_AcquiredCredits(unsigned int wave, int i2, bool b1);
	void RoundEvent_CreditsDropped(unsigned int wave, int i2);
	void RoundEvent_WaveEnd(bool b1);
	void RoundEvent_WaveStart();
	
	// TODO
	
private:
	// 360 array of structs of size 16: [0~33]
	// 580 const char * pop file name
	// 584 
	CNetworkVar(CMannVsMachineStats, m_runningTotalWaveStats);       // +0x588
	CNetworkVar(CMannVsMachineStats, m_previousWaveStats);           // +0x5a4
	CNetworkVar(CMannVsMachineStats, m_currentWaveStats);            // +0x5c0
	CNetworkVar(int,                 m_iCurrentWaveIdx);             // +0x5dc
	CNetworkVar(int,                 m_iServerWaveID);               // +0x5e0
	// 5e4 CUtlMap<uint64_t, CPlayerWaveSpendingStats> (current wave)
	// 600 CUtlMap<uint64_t, CPlayerWaveSpendingStats> (prev wave)
	// 61c CUtlMap<uint64_t, CPlayerWaveSpendingStats> (all waves possibly?)
	CNetworkVar(int,                 m_iCurrencyCollectedForRespec); // +0x638
	CNetworkVar(short,               m_nRespecsAwardedInWave);       // +0x63c
	// TODO
};


int MannVsMachineStats_GetAcquiredCredits(int wave);
int MannVsMachineStats_GetCurrentWave();
int MannVsMachineStats_GetDroppedCredits(int wave);
CMannVsMachineStats *MannVsMachineStats_GetInstance();
int MannVsMachineStats_GetMissedCredits(int wave);
void MannVsMachineStats_Init();
void MannVsMachineStats_PlayerEvent_BoughtInstantRespawn(CTFPlayer *player, int cost);
void MannVsMachineStats_PlayerEvent_Died(CTFPlayer *player);
void MannVsMachineStats_PlayerEvent_PickedUpCredits(CTFPlayer *player, unsigned int i1, int i2);
void MannVsMachineStats_PlayerEvent_PointsChanged(CTFPlayer *player, int i1);
void MannVsMachineStats_PlayerEvent_Upgraded(CTFPlayer *player, unsigned short i1, unsigned short i2, unsigned short i3, short i4, bool is_canteen);
UNKNOWN MannVsMachineStats_RemoveRespecFromPlayer(CTFPlayer *player);
void MannVsMachineStats_ResetPlayerEvents(CTFPlayer *player);
void MannVsMachineStats_RoundEvent_CreditsDropped(unsigned int wave, int i2);
void MannVsMachineStats_SetPopulationFile(const char *s1);
