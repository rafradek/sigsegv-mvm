/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_manager.h
 */


// TODO: CStuckBotEvent (sizeof: 0x4)

struct CStuckBot
{
	// TODO
	// sizeof: 0x118
	// CUtlVectorAutoPurge<CStuckBotEvent *> @ 0x104
};


class CTFBotManager : public NextBotManager
{
public:
	// TODO: class/struct/typedef CTFBotManager::ArcherAssignmentInfo
	
	CTFBotManager();
	virtual ~CTFBotManager();
	
	virtual void Update() override;
	virtual void OnMapLoaded() override;
	virtual void OnRoundRestart() override;
	
	void LevelShutdown();
	
	void ClearStuckBotData();
	void DrawStuckBotData(float f1);
	
	bool IsInOfflinePractice() const;
	void SetIsInOfflinePractice(bool b1);
	
	CTFBot *GetAvailableBotFromPool();
	bool RemoveBotFromTeamAndKick(int i1);
	
	void OnForceAddedBots(int i1);
	void OnForceKickedBots(int i1);
	
	CStuckBot *FindOrCreateStuckBot(int i1, const char *s1);
	bool IsAllBotTeam(int i1);
	bool IsMeleeOnly() const;
	
private:
	void MaintainBotQuota();
	void RevertOfflinePracticeConvars();
	
	float m_flQuotaTime; // +0x50
	// 54 
	// 58 CUtlVector<CBaseEntity *>
	// 6c CUtlVector<ArcherAssignmentInfo>
	// 80 CountdownTimer
	// 8c CUtlVector<CStuckBot *>
	// a0 CountdownTimer
};


CTFBotManager *TheTFBots();


// TODO: UTIL_KickBotFromTeam(int)
