/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/tf_mann_vs_machine_stats.cpp
 */


// g_pMVMStats


void CMannVsMachineStats::SetCurrentWave(unsigned int wave)
{
	// TODO
}

void CMannVsMachineStats::ResetStats()
{
	// TODO
}

void CMannVsMachineStats::OnStatsChanged()
{
	// TODO
}

void CMannVsMachineStats::RoundOver(bool b1)
{
	/* wtf is this for? */
	++this->m_iServerWaveID;
	--this->m_iServerWaveID;
}

void CMannVsMachineStats::SW_ReportWaveSummary(int i1, bool b1)
{
}


void CMannVsMachineStats::ResetWaveStats()
{
	// TODO
}

UNKNOWN CMannVsMachineStats::GetWaveStats(int wave) // returns a struct by value
{
	// TODO
}

int CMannVsMachineStats::GetDroppedCredits(int wave)
{
	// TODO
}

int CMannVsMachineStats::GetAcquiredCredits(int wave, bool b1)
{
	// TODO
}

int CMannVsMachineStats::GetBonusCredits(int wave)
{
	// TODO
}

int CMannVsMachineStats::GetMissedCredits(int wave)
{
	// TODO
}


CPlayerWaveSpendingStats *CMannVsMachineStats::GetSpending(int wave, uint64_t player_id)
{
	if (wave == this->m_iCurrentWaveIdx) {
		// TODO
	} else if (wave == -1) {
		// TODO
	} else if (wave == this->m_iCurrentWaveIdx - 1) {
		// TODO
	}
	
	// TODO
}

int CMannVsMachineStats::GetBuyBackSpending(CTFPlayer *player)
{
	// TODO
}

int CMannVsMachineStats::GetBottleSpending(CTFPlayer *player)
{
	// TODO
}

int CMannVsMachineStats::GetUpgradeSpending(CTFPlayer *player)
{
	// TODO
}

void CMannVsMachineStats::ClearCurrentPlayerWaveSpendingStats(int wave)
{
	// TODO
}


bool CMannVsMachineStats::IsRespecAvailableForPlayer(CTFPlayer *player)
{
	// TODO
}

UNKNOWN CMannVsMachineStats::AddRespecToPlayer(CTFPlayer *player)
{
	// TODO
}

UNKNOWN CMannVsMachineStats::RemoveRespecFromPlayer(CTFPlayer *player)
{
	// TODO
}


void CMannVsMachineStats::ResetPlayerEvents(CTFPlayer *player)
{
	// TODO
}

void CMannVsMachineStats::ResetUpgradeSpending(CTFPlayer *player)
{
	// TODO
}


void CMannVsMachineStats::SendUpgradesToPlayer(CTFPlayer *player, CUtlVector<CUpgradeInfo> *upgrades)
{
	CRecipientFilter filter;
	filter.AddRecipient(player);
	
	UserMessageBegin(filter, "MVMLocalPlayerUpgradesClear");
		WRITE_BYTE(0);
	MessageEnd();
	
	if (upgrades != nullptr) {
		FOR_EACH_VEC((*upgrades), i) {
			UserMessageBegin(filter, "MVMLocalPlayerUpgradesValue");
				WRITE_BYTE((*upgrades)[i].class_index);
				WRITE_WORD((*upgrades)[i].item_index);
				WRITE_BYTE((*upgrades)[i].upgrade_tier);
				WRITE_WORD((*upgrades)[i].upgrade_cost);
			MessageEnd();
		}
	}
}

void CMannVsMachineStats::NotifyPlayerActiveUpgradeCosts(CTFPlayer *player, int i1)
{
	// TODO: enum value for (eMannVsMachineEvent)6
	this->NotifyTargetPlayerEvent(player, 0, 6, i1);
}

void CMannVsMachineStats::NotifyPlayerEvent(CTFPlayer *player, unsigned int wave, eMannVsMachineEvent event, int i2, int i3)
{
}

void CMannVsMachineStats::NotifyTargetPlayerEvent(CTFPlayer *player, unsigned int wave, eMannVsMachineEvent event, int i2)
{
	CReliableBroadcastRecipientFilter filter;
	
	UserMessageBegin(filter, "MVMLocalPlayerWaveSpendingValue");
		WRITE_BITS(player->GetSteamIDAsUInt64(), 64);
		WRITE_BYTE(wave);
		WRITE_BYTE(event);
		WRITE_WORD(i2);
	MessageEnd();
	
	CPlayerWaveSpendingStats *spending = g_pMVMStats->GetSpending(wave, player->GetSteamIDAsUInt64());
	if (spending != nullptr) {
		// TODO: enum values for eMannVsMachineEvent
		switch (event) {
		case 3:
			spending->bottle_spending += i2;
			break;
		case 4:
			spending->upgrade_spending += i2;
			break;
		case 5:
			spending->buyback_spending += i2;
			break;
		}
	}
}


void CMannVsMachineStats::PlayerEvent_BoughtInstantRespawn(CTFPlayer *player, int cost)
{
	if (!player->IsBot() && TFGameRules() != nullptr && TFGameRules()->IsPVEModeActive()) {
		++this->m_currentWaveStats.m_nBuyBacks;
		
		// TODO: enum value for (eMannVsMachineEvent)3
		this->NotifyTargetPlayerEvent(player, this->m_iCurrentWaveIdx, 3, cost);
	}
}

void CMannVsMachineStats::PlayerEvent_DealtDamageToBots(CTFPlayer *player, int damage)
{
	if (!player->IsBot() && TFGameRules() != nullptr && TFGameRules()->IsPVEModeActive()) {
		// idx 00 => 0x364
		// idx 01 => 0x374
		// idx 30 => 0x544
		// idx 31 => 0x554
		// idx 32 => 0x564
		
		*(this + 0x360 + (0x10*idx) + 0x4) += damage;
		
		// TODO
	}
}

void CMannVsMachineStats::PlayerEvent_DealtDamageToGiants(CTFPlayer *player, int damage)
{
	if (!player->IsBot() && TFGameRules() != nullptr && TFGameRules()->IsPVEModeActive()) {
		// TODO
	}
}

void CMannVsMachineStats::PlayerEvent_DealtDamageToTanks(CTFPlayer *player, int damage)
{
	if (!player->IsBot() && TFGameRules() != nullptr && TFGameRules()->IsPVEModeActive()) {
		// TODO
	}
}

void CMannVsMachineStats::PlayerEvent_Died(CTFPlayer *player)
{
	if (!player->IsBot() && TFGameRules() != nullptr && TFGameRules()->IsPVEModeActive()) {
		// TODO
	}
}

void CMannVsMachineStats::PlayerEvent_PickedUpCredits(CTFPlayer *player, unsigned int i1, int i2)
{
	if (!player->IsBot() && TFGameRules() != nullptr && TFGameRules()->IsPVEModeActive()) {
		// TODO
	}
}

void CMannVsMachineStats::PlayerEvent_PointsChanged(CTFPlayer *player, int i1)
{
}

void CMannVsMachineStats::PlayerEvent_Upgraded(CTFPlayer *player, unsigned short i1, unsigned short i2, unsigned char i3, short i4, bool is_canteen)
{
	// TODO
}


void CMannVsMachineStats::RoundEvent_AcquiredCredits(unsigned int wave, int i2, bool b1)
{
	// TODO
}

void CMannVsMachineStats::RoundEvent_CreditsDropped(unsigned int wave, int i2)
{
	// TODO
}

void CMannVsMachineStats::RoundEvent_WaveEnd(bool b1)
{
	// TODO
}

void CMannVsMachineStats::RoundEvent_WaveStart()
{
	this->ResetWaveStats();
}


int MannVsMachineStats_GetAcquiredCredits(int wave)
{
	if (g_pMVMStats != nullptr) {
		return g_pMVMStats->MannVsMachineStats_GetAcquiredCredits(wave, true);
	}
	
	return 0;
}

int MannVsMachineStats_GetCurrentWave()
{
	if (g_pMVMStats != nullptr) {
		return g_pMVMStats->m_iCurrentWaveIdx;
	}
	
	return 0;
}

int MannVsMachineStats_GetDroppedCredits(int wave)
{
	if (g_pMVMStats != nullptr) {
		return g_pMVMStats->MannVsMachineStats_GetDroppedCredits(wave);
	}
	
	return 0;
}

CMannVsMachineStats *MannVsMachineStats_GetInstance()
{
	return g_pMVMStats;
}

int MannVsMachineStats_GetMissedCredits(int wave)
{
	if (g_pMVMStats != nullptr) {
		return g_pMVMStats->MannVsMachineStats_GetMissedCredits(wave);
	}
	
	return 0;
}

void MannVsMachineStats_Init()
{
	CBaseEntity::Create("tf_mann_vs_machine_stats", vec3_origin, vec3_angle);
}

void MannVsMachineStats_PlayerEvent_BoughtInstantRespawn(CTFPlayer *player, int cost)
{
	if (g_pMVMStats != nullptr) {
		g_pMVMStats->PlayerEvent_BoughtInstantRespawn(player, cost);
	}
}

void MannVsMachineStats_PlayerEvent_Died(CTFPlayer *player)
{
	if (g_pMVMStats != nullptr) {
		g_pMVMStats->PlayerEvent_Died(player);
	}
}

void MannVsMachineStats_PlayerEvent_PickedUpCredits(CTFPlayer *player, unsigned int i1, int i2)
{
	if (g_pMVMStats != nullptr) {
		g_pMVMStats->PlayerEvent_PickedUpCredits(player, i1, i2);
	}
}

void MannVsMachineStats_PlayerEvent_PointsChanged(CTFPlayer *player, int i1)
{
}

void MannVsMachineStats_PlayerEvent_Upgraded(CTFPlayer *player, unsigned short i1, unsigned short i2, unsigned short i3, short i4, bool is_canteen)
{
	if (g_pMVMStats != nullptr) {
		g_pMVMStats->PlayerEvent_Upgraded(player, i1, i2, i3, i4, is_canteen);
	}
}

UNKNOWN MannVsMachineStats_RemoveRespecFromPlayer(CTFPlayer *player)
{
	// TODO
}

void MannVsMachineStats_ResetPlayerEvents(CTFPlayer *player)
{
	if (g_pMVMStats != nullptr) {
		g_pMVMStats->ResetPlayerEvents(player);
	}
}

void MannVsMachineStats_RoundEvent_CreditsDropped(unsigned int wave, int i2)
{
	if (g_pMVMStats != nullptr) {
		g_pMVMStats->RoundEvent_CreditsDropped(wave, i2);
	}
}

void MannVsMachineStats_SetPopulationFile(const char *s1)
{
	if (g_pMVMStats != nullptr) {
		/* 0x580 */ = s1;
	}
}
