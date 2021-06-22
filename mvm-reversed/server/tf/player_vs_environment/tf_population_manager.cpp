/* reverse engineering by sigsegv
 * based on TF2 version 20160428a
 * server/tf/player_vs_environment/tf_population_manager.cpp
 */


// global static s_iLastKnownMission
// global static s_iLastKnownMissionCategory


// TODO: re-check all convars and concommands from this file, check for new ones too


ConVar tf_mvm_missioncyclefile("tf_mvm_missioncyclefile", "tf_mvm_missioncycle.res", FCVAR_NONE,
	"Name of the .res file used to cycle mvm misisons", &MvMMissionCycleFileChangedCallback);
ConVar tf_populator_debug("tf_populator_debug", "0", FCVAR_CHEAT);
ConVar tf_populator_active_buffer_range("tf_populator_active_buffer_range", "3000", FCVAR_CHEAT,
	"Populate the world this far ahead of lead raider, and this far behind last raider");
ConVar tf_mvm_default_sentry_buster_damage_dealt_threshold("tf_mvm_default_sentry_buster_damage_dealt_threshold", "3000", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);
ConVar tf_mvm_default_sentry_buster_kill_threshold("tf_mvm_default_sentry_buster_kill_threshold", "15", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY);
ConVar tf_mvm_miniboss_scale("tf_mvm_miniboss_scale", "1.75", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Full body scale for minibosses.", &MinibossScaleChangedCallBack);
ConVar tf_mvm_disconnect_on_victory("tf_mvm_disconnect_on_victory", "0", FCVAR_REPLICATED,
	"Enable to Disconnect Players after completing MvM");
ConVar tf_mvm_victory_reset_time("tf_mvm_victory_reset_time", "60.0", FCVAR_REPLICATED,
	"Seconds to wait after MvM victory before cycling to the next mission.  (Only used if tf_mvm_disconnect_on_victory is false.)");
ConVar tf_mvm_victory_disconnect_time("tf_mvm_victory_disconnect_time", "180.0", FCVAR_REPLICATED,
	"Seconds to wait after MvM victory before kicking players.  (Only used if tf_mvm_disconnect_on_victory is true.)");
ConVar tf_mvm_endless_force_on("tf_mvm_endless_force_on", "0", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY,
	"Force MvM Endless mode on");
ConVar tf_mvm_endless_wait_time("tf_mvm_endless_wait_time", "5.0f", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY);
ConVar tf_mvm_endless_bomb_reset("tf_mvm_endless_bomb_reset", "5", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY,
	"Number of Waves to Complete before bomb reset");
ConVar tf_mvm_endless_bot_cash("tf_mvm_endless_bot_cash", "120", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY,
	"In Endless, number of credits bots get per wave");
ConVar tf_mvm_endless_tank_boost("tf_mvm_endless_tank_boost", "0.2", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_DEVELOPMENTONLY,
	"In Endless, amount of extra health for the tank per wave");
ConVar tf_populator_health_multiplier("tf_populator_health_multiplier", "1.0", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar tf_populator_damage_multiplier("tf_populator_damage_multiplier", "1.0", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED);
ConVar tf_mvm_skill("tf_mvm_skill", "1", FCVAR_DONTRECORD | FCVAR_CHEAT | FCVAR_REPLICATED,
	"Sets the challenge level of the invading bot army. 1 = easiest, 3 = normal, 5 = hardest", true, 1.0f, true, 5.0f, &MVMSkillChangedCallback);

ConCommand tf_mvm_nextmission_command("tf_mvm_nextmission", &tf_mvm_nextmission,
	"Load the next mission", FCVAR_CHEAT);
ConCommand tf_mvm_force_victory_command("tf_mvm_force_victory", &tf_mvm_force_victory,
	"Force immediate victory.", FCVAR_CHEAT);
ConCommand tf_mvm_checkpoint_command("tf_mvm_checkpoint", &tf_mvm_checkpoint,
	"Save a checkpoint snapshot", FCVAR_CHEAT);
ConCommand tf_mvm_checkpoint_clear_command("tf_mvm_checkpoint_clear", &tf_mvm_checkpoint_clear,
	"Clear the saved checkpoint", FCVAR_CHEAT);
ConCommand tf_mvm_jump_to_wave_command("tf_mvm_jump_to_wave", &tf_mvm_jump_to_wave,
	"Jumps directly to the given Mann Vs Machine wave number", FCVAR_CHEAT);
ConCommand tf_mvm_debugstats_command("tf_mvm_debugstats", &tf_mvm_debugstats,
	"Dumpout MvM Data", FCVAR_CHEAT);


// TODO: server class init for CPopulationManager ("info_populator")


void CPopulationManager::UpdateObjectiveResource()
{
	if (this->m_Waves.IsEmpty() || g_pObjectiveResource == nullptr) {
		return;
	}
	
	g_pObjectiveResource->m_nMvMEventPopfileType = this->m_bEventPopfile;
	
	if (this->IsInEndlessWaves()) {
		g_pObjectiveResource->m_nMannVsMachineMaxWaveCount = 0;
	} else {
		g_pObjectiveResource->m_nMannVsMachineMaxWaveCount = this->m_Waves.Count();
	}
	
	g_pObjectiveResource->m_nMannVsMachineWaveCount = this->m_iCurrentWave + 1;
	
	CWave *wave = this->GetCurrentWave();
	if (wave == nullptr) {
		return;
	}
	
	g_pObjectiveResource->MannVsMachineWaveEnemyCount = wave->m_iTotalCountNonSupport;
	
	g_pObjectiveResource->ClearMannVsMachineWaveClassFlags();
	
	int i;
	bool has_engineer = false;
	for (i = 0; i < wave->m_ClassCounts.Count() && i < 24; ++i) {
		if (!has_engineer && V_stricmp(STRING(wave->m_ClassCounts[i].icon), "engineer") == 0) {
			has_engineer = true;
		}
		
		g_pObjectiveResource->SetMannVsMachineWaveClassName (i, wave->m_ClassCounts[i].icon);
		g_pObjectiveResource->SetMannVsMachineWaveClassCount(i, wave->m_ClassCounts[i].count);
		g_pObjectiveResource->SetMannVsMachineWaveClassFlags(i, wave->m_ClassCounts[i].flags);
	}
	
	if (has_engineer && i < 24) {
		g_pObjectiveResource->SetMannVsMachineWaveClassName (i, g_pObjectiveResource->m_iszTeleporterName);
		g_pObjectiveResource->SetMannVsMachineWaveClassCount(i, 0);
		g_pObjectiveResource->SetMannVsMachineWaveClassFlags(i, CLASSFLAG_MISSION);
		++i;
	}
	
	for ( ; i < 24; ++i) {
		g_pObjectiveResource->SetMannVsMachineWaveClassName (i, NULL_STRING);
		g_pObjectiveResource->SetMannVsMachineWaveClassCount(i, 0);
		g_pObjectiveResource->SetMannVsMachineWaveClassFlags(i, 0);
	}
}
