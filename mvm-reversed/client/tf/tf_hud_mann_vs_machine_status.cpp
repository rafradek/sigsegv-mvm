/* reverse engineering by sigsegv
 * based on TF2 version 20160202a
 * client/tf/tf_hud_mann_vs_machine_status.cpp
 */


// cl_mvm_wave_status_visible_during_wave


void CWaveStatusPanel::UpdateEnemyCounts()
{
	if (TFGameRules() == nullptr)         return;
	if (TFObjectiveResource() == nullptr) return;
	
	bool between_waves = false;
	if (TFGameRules()->InSetup() || TFObjectiveResource()->m_bMannVsMachineBetweenWaves) {
		// this->dword_0x164 = 0
		between_waves = true;
	}
}
