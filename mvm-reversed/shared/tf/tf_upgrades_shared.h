/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/tf/tf_upgrades_shared.h
 */


// sizeof: 0x15c
struct CMannVsMachineUpgrades
{
	char m_szAttribute[0x40]; // +0x000
	char m_szIcon[MAX_PATH];  // +0x040
	float m_flIncrement;      // +0x144
	float m_flCap;            // +0x148
	int m_nCost;              // +0x14c
	int m_iUIGroup;           // +0x150
	int m_iQuality;           // +0x154
	int m_iTier;              // +0x158
};


// sizeof: 0x3c
class CMannVsMachineUpgradeManager : public CAutoGameSystem
{
public:
	CMannVsMachineUpgradeManager();
	virtual ~CMannVsMachineUpgradeManager();
	
	virtual UNKNOWN LevelInitPostEntity() override;
	virtual UNKNOWN LevelShutdownPostEntity() override;
	
	UNKNOWN LoadUpgradesFile();
	UNKNOWN LoadUpgradesFileFromPath(const char *path);
	
private:
	UNKNOWN GetAttributeIndexByName(const char *name);
	UNKNOWN ParseUpgradeBlockForUIGroup(KeyValues *kv, int ui_group);
	
	CUtlVector<CMannVsMachineUpgrades> m_Upgrades; // +0x0c
	CUtlMap<const char *, int> m_UpgradeMap;       // +0x24
};


UNKNOWN GetUpgradeStepData(CTFPlayer *player, int i1, int i2, int& i3, bool& b1);
