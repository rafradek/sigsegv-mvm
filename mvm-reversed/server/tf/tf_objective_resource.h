/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/tf_objective_resource.h
 */


enum MvMWaveClassFlags : unsigned int
{
	CLASSFLAG_NORMAL          = (1 << 0), // set for non-support bots and tanks
	CLASSFLAG_SUPPORT         = (1 << 1), // set for "support 1" and "support limited" bots
	CLASSFLAG_MISSION         = (1 << 2), // set for mission support bots and teleporters
	CLASSFLAG_MINIBOSS        = (1 << 3), // set for minibosses and tanks (red background)
	CLASSFLAG_CRITICAL        = (1 << 4), // set for crit bots (blue border)
	CLASSFLAG_SUPPORT_LIMITED = (1 << 5), // set for "support limited" bots
};


class CTFObjectiveResource : public CBaseTeamObjectiveResource
{
public:
	CTFObjectiveResource();
	virtual ~CTFObjectiveResource();
	
	virtual void Spawn() override;
	
	void IncrementMannVsMachineWaveClassCount(string_t name, unsigned int flags);
	void DecrementMannVsMachineWaveClassCount(string_t name, unsigned int flags);
	
	void IncrementTelporterCount();
	void DecrementTeleporterCount();
	
	int GetMannVsMachineWaveClassCount(int index);
	void SetMannVsMachineWaveClassCount(int index, int count);
	
	void ClearMannVsMachineWaveClassFlags();
	void AddMannVsMachineWaveClassFlags(int index, unsigned int flags);
	
	void SetMannVsMachineWaveClassName(int index, string_t name);
	void SetMannVsMachineWaveClassActive(string_t name, bool active);
	
	// inline functions known to exist from SDK:
	// SetMannVsMachineBetweenWaves(bool)
	// GetMannVsMachineIsBetweenWaves()
	// GetMvMPopFileName()
	
	// functions only in certain versions:
	// SetBaseMvMBombUpgradeTime(float)
	// SetFlagCarrierUpgradeLevel(int)
	// SetNextMvMBombUpgradeTime(float)
	
	CNetworkVar(int,   m_nMannVsMachineMaxWaveCount);                 // +0x0fa4
	CNetworkVar(int,   m_nMannVsMachineWaveCount);                    // +0x0fa8
	CNetworkVar(int,   m_nMannVsMachineWaveEnemyCount);               // +0x0fac
	CNetworkVar(int,   m_nMvMWorldMoney);                             // +0x0fb0
	CNetworkVar(float, m_flMannVsMachineNextWaveTime);                // +0x0fb4
	CNetworkVar(bool,  m_bMannVsMachineBetweenWaves);                 // +0x0fb8
	
	CNetworkArray(int, m_nMannVsMachineWaveClassCounts,  12);         // +0x0fbc
	CNetworkArray(int, m_nMannVsMachineWaveClassCounts2, 12);         // +0x0fec
	
	CNetworkArray(string_t, m_iszMannVsMachineWaveClassNames,  12);   // +0x101c
	CNetworkArray(string_t, m_iszMannVsMachineWaveClassNames2, 12);   // +0x104c
	
	CNetworkArray(unsigned int, m_nMannVsMachineWaveClassFlags,  12); // +0x107c
	CNetworkArray(unsigned int, m_nMannVsMachineWaveClassFlags2, 12); // +0x10ac
	
	CNetworkArray(bool, m_bMannVsMachineWaveClassActive,  12);        // +0x10dc
	CNetworkArray(bool, m_bMannVsMachineWaveClassActive2, 12);        // +0x10e8
	
	CNetworkVar(int,      m_nFlagCarrierUpgradeLevel);                // +0x10f4
	CNetworkVar(float,    m_flMvMBaseBombUpgradeTime);                // +0x10f8
	CNetworkVar(float,    m_flMvMNextBombUpgradeTime);                // +0x10fc
	CNetworkVar(int,      m_iChallengeIndex);                         // +0x1100
	CNetworkVar(string_t, m_iszMvMPopfileName);                       // +0x1104
	CNetworkVar(int,      m_nMvMEventPopfileType);                    // +0x1108
	
	string_t m_iszTeleporterName;                                     // +0x110c
};
