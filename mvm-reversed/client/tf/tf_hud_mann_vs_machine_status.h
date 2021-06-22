/* reverse engineering by sigsegv
 * based on TF2 version 20160202a
 * client/tf/tf_hud_mann_vs_machine_status.h
 */


class CEnemyCountPanel : public vgui::EditablePanel
{
public:
	// TODO
	
private:
	// TODO
	
	CTFImagePanel *m_EnemyCountImage;       // +0x16c
	vgui::Panel   *m_EnemyCountImageBG;     // +0x170
	CTFImagePanel *m_EnemyCountCritImageBG; // +0x174
};

// CMvMBossProgressBar
// CMvMBossStatusPanel

// sizeof: 0x204
class CWaveStatusPanel : public vgui::EditablePanel
{
public:
	// TODO
	
	void UpdateEnemyCounts();
	
private:
	// TODO
	
	// ?
	// 164 dword
	// 168 dword 0 bounds: x
	// 16c dword 0 bounds: y
	// 170 dword 0
	// 174 dword 0
	// 178 dword 0
	// 17c dword 0
	// 180 dword 0
	// 184 dword 0
	// 188 dword -1
	// 18c dword -1
	// 190 byte 0
	// 194 
	// 198 
	// 19c 
	// 1a0 
	// 1a4  
	// 1a8 CUtlVector<EnemyCountPanel *> (x24)
	// 1bc 
	// 1c0 
	// 1c4 
	// 1c8 
	// 1cc 
	// 1d0 
	// 1d4 
	// 1d8 
	// 1dc 
	// 1e0 
	// 1e4 
	// 1e8 
	// 1ec 
	// 1f0 
	// 1f4 
	// 1f8 
	// 1fc 
	// 200 
};

// CCurrencyStatusPanel
// CInWorldCurrencyStatus
// CWarningSwoop
// CWaveCompleteSummaryPanel
// CVictorySplash
// CMvMBombCarrierProgress
// CTFHudMannVsMachineStatus
