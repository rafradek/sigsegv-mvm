/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * shared/tf/tf_gamerules.h
 */


class CTFGameRules : public CTeamplayRoundBasedRules, public CGameEventListener
{
public:
	// TODO
	bool GameModeUsesUpgrades() { return this->m_bPlayingMannVsMachine; }
	bool IsPvEModeActive()      { return this->m_bPlayingMannVsMachine; }
	// TODO
	bool IsPvEModeControlled(CBaseEntity *ent)
	{
		return (ent != nullptr && this->m_bPlayingMannVsMachine &&
			ent->GetTeamNumber() == TF_TEAM_BLUE);
	}
	
	// TODO: IsQuickBuildTime
	// TODO: CanPlayerUseRespec
	// TODO: GetUpgradeTier
	// TODO: CanUpgradeWithAttrib
	// TODO: SetNextMvMPopfile
	// TODO: GetNextMvMPopfile
	// TODO: IsUpgradeTierEnabled
	// TODO: GetCostForUpgrade
	
	// TODO
	
private:
	// TODO
	bool m_bPlayingMannVsMachine; // +0x96a
	// TODO
};


// void CTFGameRules::DistributeCurrencyAmount(int, CTFPlayer *, bool, bool, bool)
// - int amount
// - CTFPlayer *player (whose credit balance should be increased)
// - bool give_to_all
// - bool b2: whether to call CMannVsMachineStats::RoundEvent_CreditsDropped
// - bool b3: whether the credits are bonus credits (I think)
// callers:
// - CCaptureFlag::Capture:
//   (amount, player, true, false, false)
// - CTFGameRules::FireGameEvent("teamplay_point_capture", heavyweapons only):
//   (100, player, false, false, false)
// - CTFGameRules::FireGameEvent("player_escort_score"):
//   (amount, player, false, false, false)
// - CCurrencyPack::ComeToReset(trigger_hurt):
//   (amount, nullptr, true, false, false)
// - CCurrencyPack::MyTouch:
//   (amount, player, true, false, false)
// - CCurrencyPack::DistributedBy:
//   (amount, nullptr, true, false, false)
// - CWaveSpawnPopulator::OnNonSupportWavesDone:
//   (amount, nullptr, true, true, false)
// - CWave::WaveIntermissionUpdate:
//   (amount, nullptr, true, false, true)
// - CTFPlayer::AddExperiencePoints:
//   (amount, player, false, false, false)
// - CTFPlayer::Event_Killed:
//   (amount, nullptr, true, true, false)


// int CTFGameRules::CalculateCurrencyAmountByType(CurrencyRewards_t)
// int CTFGameRules::CalculateCurrencyAmount_CustomPack(int)
enum CurrencyRewards_t : int
{
	// TODO
	
	// 0x0 40   
	// 0x1 40   
	// 0x2 20   
	// 0x3 1    
	// 0x4 100  used for "teamplay_point_capture"
	// 0x5 10   
	// 0x6 5    CCurrencyPackSmall::GetPackSize()
	// 0x7 10   CCurrencyPackMedium::GetPackSize()
	// 0x8 25   CCurrencyPack::GetPackSize()
	// 0x9 0    CCurrencyPackCustom::GetPackSize()
	// 0xA 5    
	// 0xB 100  used for wave credit bonus
};


// TODO: CTFGameRulesProxy


// TODO: GlobalAttrModifier_IsMvMModeActive
// TODO: BIgnoreConvarChangeInPVEMode
// TODO: SendProxy_TFGameRules
// TODO: IsValveMap
// TODO: TF_IsHolidayActive
// TODO: GlobalAttrModifier_IsHalloweenEventActive
// TODO: BIsCvarIndicatingHolidayIsActive
// 
