/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_populators.cpp
 */


ConVar tf_mvm_engineer_teleporter_uber_duration("tf_mvm_engineer_teleporter_uber_duration", "5.f", FCVAR_CHEAT);


CSpawnLocation::CSpawnLocation()
{
	this->m_iWhere = Where::TEAMSPAWN;
}


bool CSpawnLocation::Parse(KeyValues *kv)
{
	// TODO
}

SpawnResult CSpawnLocation::FindSpawnLocation(Vector& vec) const
{
	// TODO
}

CTFNavArea *CSpawnLocation::SelectSpawnArea() const
{
	// TODO
}


IPopulator::IPopulator(CPopulationManager *popmgr)
	: m_PopMgr(popmgr)
{
}

CRandomPlacementPopulator::CRandomPlacementPopulator(CPopulationManager *popmgr)
	: IPopulator(popmgr)
{
	this->m_iCount = 0;
	this->m_flMinimumSeparation = 0.0f;
	// TODO
}

CPeriodicSpawnPopulator::CPeriodicSpawnPopulator(CPopulationManager *popmgr)
	: IPopulator(popmgr)
{
	this->m_flMinInterval = 30.0f;
	this->m_flMaxInterval = 30.0f;
}

CMissionPopulator::CMissionPopulator(CPopulationManager *popmgr)
	: IPopulator(popmgr)
{
	// TODO
	this->m_flInitialCooldown  = 0.0f;
	this->m_flCooldownTime     = 0.0f;
	this->m_iDesiredCount      = 0;
	this->m_iBeginAtWave       = 0;
	this->m_iEndBeforeThisWave = 9999;
	// TODO
}

CWaveSpawnPopulator::CWaveSpawnPopulator(CPopulationManager *popmgr)
	: IPopulator(popmgr)
{
	// TODO
}

CWave::CWave(CPopulationManager *popmgr)
	: IPopulator(popmgr)
{
	// TODO
}


IPopulator::~IPopulator()
{
	if (this->m_Spawner != nullptr) {
		delete this->m_Spawner;
	}
}

CRandomPlacementPopulator::~CRandomPlacementPopulator()
{
}

CPeriodicSpawnPopulator::~CPeriodicSpawnPopulator()
{
}

CMissionPopulator::~CMissionPopulator()
{
}

CWaveSpawnPopulator::~CWaveSpawnPopulator()
{
	// TODO
}

CWave::~CWave()
{
	// TODO
}


bool CRandomPlacementPopulator::Parse(KeyValues *kv)
{
	// TODO
}

bool CPeriodicSpawnPopulator::Parse(KeyValues *kv)
{
	// TODO
}

bool CMissionPopulator::Parse(KeyValues *kv)
{
	int runforthismanywaves = 99999;
	
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (this->m_Where->Parse(subkey)) {
			continue;
		}
		
		if (V_stricmp(name, "Objective") == 0) {
			if (V_stricmp(subkey->GetString(), "DestroySentries") == 0 ||
				V_stricmp(subkey->GetString(), "SeekAndDestroy") == 0) {
				this->m_Objective = CTFBot::MissionType::DESTROY_SENTRIES;
			} else if (V_stricmp(subkey->GetString(), "Sniper") == 0) {
				this->m_Objective = CTFBot::MissionType::SNIPER;
			} else if (V_stricmp(subkey->GetString(), "Spy") == 0) {
				this->m_Objective = CTFBot::MissionType::SPY;
			} else if (V_stricmp(subkey->GetString(), "Engineer") == 0) {
				this->m_Objective = CTFBot::MissionType::ENGINEER;
			} else {
				Warning("Invalid mission '%s'\n", subkey->GetString());
				return false;
			}
		} else if (V_stricmp(name, "InitialCooldown") == 0) {
			this->m_flInitialCooldown = subkey->GetFloat();
		} else if (V_stricmp(name, "CooldownTime") == 0) {
			this->m_flCooldownTime = subkey->GetFloat();
		} else if (V_stricmp(name, "BeginAtWave") == 0) {
			this->m_iBeginAtWave = subkey->GetInt();
		} else if (V_stricmp(name, "RunForThisManyWaves") == 0) {
			runforthismanywaves = subkey->GetInt();
		} else if (V_stricmp(name, "DesiredCount") == 0) {
			this->m_iDesiredCount = subkey->GetInt();
		} else {
			/* BUG: duplicate spawner definitions will leak memory */
			this->m_Spawner =
				IPopulationSpawner::ParseSpawner(this, subkey);
			if (this->m_Spawner == nullptr) {
				Warning("Unknown attribute '%s' in Mission definition.\n",
					name);
			}
		}
	}
	
	this->m_iEndBeforeThisWave = this->m_iBeginAtWave + runforthismanywaves;
	
	return true;
}

bool CWaveSpawnPopulator::Parse(KeyValues *kv)
{
	/* handle the template reference (if any) before doing anything else */
	KeyValues *kv_tref = kv->FindKey("Template");
	if (kv_tref != nullptr) {
		const char *tname = kv_tref->GetString();
		
		KeyValues *kv_timpl =
			this->m_Populator->m_PopMgr->m_kvTemplates->FindKey(tname);
		if (kv_timpl != nullptr) {
			if (!this->Parse(kv_timpl)) {
				return false;
			}
		} else {
			Warning("Unknown Template '%s' in WaveSpawn definition\n",
				tname);
		}
	}
	
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (this->m_Where->Parse(subkey)) {
			continue;
		}
		
		if (V_stricmp(name, "Template") == 0) {
			continue;
		}
		
		if (V_stricmp(name, "TotalCount") == 0) {
			this->m_iTotalCount = subkey->GetInt();
		} else if (V_stricmp(name, "MaxActive") == 0) {
			this->m_iMaxActive = subkey->GetInt();
		} else if (V_stricmp(name, "SpawnCount") == 0) {
			this->m_iSpawnCount = subkey->GetInt();
		} else if (V_stricmp(name, "WaitBeforeStarting") == 0) {
			this->m_flWaitBeforeStarting = subkey->GetFloat();
		} else if (V_stricmp(name, "WaitBetweenSpawns") == 0) {
			if (this->m_flWaitBetweenSpawns == 0.0f ||
				!this->m_bWaitBetweenSpawnsAfterDeath) {
				this->m_flWaitBetweenSpawns = subkey->GetFloat();
			} else {
				Warning("Already specified WaitBetweenSpawnsAfterDeath time, "
					"WaitBetweenSpawns won't be used\n");
				continue;
			}
		} else if (V_stricmp(name, "WaitBetweenSpawnsAfterDeath") == 0) {
			if (this->m_flWaitBetweenSpawns == 0.0f) {
				this->m_bWaitBetweenSpawnsAfterDeath = true;
				this->m_flWaitBetweenSpawns = subkey->GetFloat();
			} else {
				Warning("Already specified WaitBetweenSpawns time, "
					"WaitBetweenSpawnsAfterDeath won't be used\n");
				continue;
			}
		} else if (V_stricmp(name, "StartWaveWarningSound") == 0) {
			this->m_strStartWaveWarningSound.sprintf("%s",
				subkey->GetString());
		} else if (V_stricmp(name, "StartWaveOutput") == 0) {
			this->m_StartWaveOutput = ParseEvent(subkey);
		} else if (V_stricmp(name, "FirstSpawnWarningSound") == 0) {
			this->m_strFirstSpawnWarningSound.sprintf("%s",
				subkey->GetString());
		} else if (V_stricmp(name, "FirstSpawnOutput") == 0) {
			this->m_FirstSpawnOutput = ParseEvent(subkey);
		} else if (V_stricmp(name, "LastSpawnWarningSound") == 0) {
			this->m_strLastSpawnWarningSound.sprintf("%s",
				subkey->GetString());
		} else if (V_stricmp(name, "LastSpawnOutput") == 0) {
			this->m_LastSpawnOutput = ParseEvent(subkey);
		} else if (V_stricmp(name, "DoneWarningSound") == 0) {
			this->m_strDoneWarningSound.sprintf("%s",
				subkey->GetString());
		} else if (V_stricmp(name, "DoneOutput") == 0) {
			this->m_DoneOutput = ParseEvent(subkey);
		} else if (V_stricmp(name, "TotalCurrency") == 0) {
			this->m_iTotalCurrency = subkey->GetInt();
		} else if (V_stricmp(name, "Name") == 0) {
			this->m_strName = subkey->GetString();
		} else if (V_stricmp(name, "WaitForAllSpawned") == 0) {
			this->m_strWaitForAllSpawned = subkey->GetString();
		} else if (V_stricmp(name, "WaitForAllDead") == 0) {
			this->m_strWaitForAllDead = subkey->GetString();
		} else if (V_stricmp(name, "Support") == 0) {
			this->m_bSupport = true;
			this->m_bSupportLimited =
				(V_stricmp(subkey->GetString(), "Limited") == 0);
		} else if (V_stricmp(name, "RandomSpawn") == 0) {
			this->m_bRandomSpawn = subkey->GetBool();
		} else {
			/* BUG: duplicate spawner definitions will leak memory */
			this->m_Spawner = IPopulationSpawner::ParseSpawner(this, subkey);
			if (this->m_Spawner == nullptr) {
				Warning("Unknown attribute '%s' in WaveSpawn definition.\n",
					name);
			}
		}
		
		this->m_iCountNotYetSpawned = this->m_iTotalCount;
		this->m_iCurrencyLeft = this->m_iTotalCurrency;
	}
	
	return true;
}

bool CWave::Parse(KeyValues *kv)
{
	FOR_EACH_SUBKEY(kv, subkey) {
		/* this is crustier then the other parsers:
		 * keeps calling subkey->GetName() instead of storing it in a local;
		 * also doesn't do a strlen check on the name */
		
		if (V_stricmp(subkey->GetName(), "WaveSpawn") == 0) {
			CWaveSpawnPopulator *wavespawn =
				new CWaveSpawnPopulator(this->m_PopMgr);
			if (!wavespawn->Parse(subkey)) {
				Warning("Error reading WaveSpawn definition\n");
				return false;
			}
			this->m_WaveSpawns.AddToTail(wavespawn);
			
			if (!wavespawn->m_bSupport) {
				this->m_iTotalCountNonSupport += wavespawn->m_iTotalCount;
			}
			this->m_iTotalCurrency += wavespawn->m_iTotalCurrency;
			
			wavespawn->m_Wave = this;
			
			IPopulationSpawner *spawner = wavespawn->m_Spawner;
			if (spawner != nullptr) {
				if (spawner->IsVarious()) {
					for (int i = 0; i < wavespawn->m_iTotalCount; ++i) {
						unsigned int flags = (wavespawn->m_bSupport ?
							CLASSFLAG_SUPPORT : CLASSFLAG_NORMAL);
						if (wavespawn->m_Spawner->IsMiniBoss(i)) {
							flags |= CLASSFLAG_MINIBOSS;
						}
						if (wavespawn->m_Spawner->HasAttribute(CTFBot::AttributeType::ALWAYSCRIT, i)) {
							flags |= CLASSFLAG_CRITICAL;
						}
						if (wavespawn->m_bSupportLimited) {
							flags |= CLASSFLAG_SUPPORT_LIMITED;
						}
						
						this->AddClassType(wavespawn->m_Spawner->GetClassIcon(i),
							1, flags);
					}
				} else {
					unsigned int flags = (wavespawn->m_bSupport ?
						CLASSFLAG_SUPPORT : CLASSFLAG_NORMAL);
					if (wavespawn->m_Spawner->IsMiniBoss(-1)) {
						flags |= CLASSFLAG_MINIBOSS;
					}
					if (wavespawn->m_Spawner->HasAttribute(CTFBot::AttributeType::ALWAYSCRIT, -1)) {
						flags |= CLASSFLAG_CRITICAL;
					}
					if (wavespawn->m_bSupportLimited) {
						flags |= CLASSFLAG_SUPPORT_LIMITED;
					}
					
					this->AddClassType(wavespawn->m_Spawner->GetClassIcon(-1),
						wavespawn->m_iTotalCount, flags);
				}
			}
		} else if (V_stricmp(subkey->GetName(), "Sound") == 0) {
			this->m_strSound.sprintf("%s", subkey->GetString());
		} else if (V_stricmp(subkey->GetName(), "Description") == 0) {
			this->m_strDescription.sprintf("%s", subkey->GetString());
		} else if (V_stricmp(subkey->GetName(), "WaitWhenDone") == 0) {
			this->m_flWaitWhenDone = subkey->GetFloat();
		} else if (V_stricmp(subkey->GetName(), "Checkpoint") == 0) {
			/* doesn't do anything! */
		} else if (V_stricmp(subkey->GetName(), "StartWaveOutput") == 0) {
			this->m_StartWaveOutput = ParseEvent(subkey);
		} else if (V_stricmp(subkey->GetName(), "DoneOutput") == 0) {
			this->m_DoneOutput = ParseEvent(subkey);
		} else if (V_stricmp(subkey->GetName(), "InitWaveOutput") == 0) {
			this->m_InitWaveOutput = ParseEvent(subkey);
		} else {
			Warning("Unknown attribute '%s' in Wave definition.\n",
				subkey->GetName());
		}
	}
	
	return true;
}


void IPopulator::PostInitialize()
{
}

void CRandomPlacementPopulator::PostInitialize()
{
	// TODO
}

void CPeriodicSpawnPopulator::PostInitialize()
{
	// TODO
}


void IPopulator::Update()
{
}

void CPeriodicSpawnPopulator::Update()
{
	// TODO
}

void CMissionPopulator::Update()
{
	VPROF_BUDGET("CMissionPopulator::Update", "NextBot");
	
	if (!TFGameRules()->IsMannVsMachineMode()) {
		this->m_iState = State::NOT_STARTED;
		return;
	}
	
	int wavenum = this->m_PopMgr->m_iCurrentWave;
	if (wavenum < this->m_iBeginAtWave ||
		wavenum >= this->m_iEndBeforeThisWave) {
		this->m_iState = State::NOT_STARTED;
		return;
	}
	
	if (TFObjectiveResource()->GetMannVsMachineIsBetweenWaves()) {
		this->m_iState = State::NOT_STARTED;
		return;
	}
	
	if (this->m_iState == State::NOT_STARTED) {
		if (this->m_flInitialCooldown != 0.0f) {
			this->m_iState = State::INITIAL;
			this->m_ctCooldown.Start(this->m_flInitialCooldown);
			return;
		}
	} else if (this->m_iState == State::INITIAL) {
		if (!this->m_ctCooldown.IsElapsed()) {
			return;
		}
		
		this->m_iState = State::RUNNING;
		this->m_ctCooldown.Invalidate();
	}
	
	CTFBot::MissionType objective = this->m_Objective;
	if (objective == CTFBot::MissionType::DESTROY_SENTRIES) {
		this->UpdateMissionDestroySentries();
	} else {
		if (objective < CTFBot::MissionType::DESTROY_SENTRIES) {
			return;
		} else if (objective > CTFBot::MissionType::ENGINEER) {
			return;
		}
		
		this->UpdateMission(objective);
	}
}

void CWaveSpawnPopulator::Update()
{
	VPROF_BUDGET("CWaveSpawnPopulator::Update", "NextBot");
	
	switch (this->m_iState) {
		
	case InternalStateType::INITIAL:
		this->m_ctSpawnDelay.Start(this->m_flWaitBeforeStarting);
		this->m_reservedPlayerSlotCount = 0;
		this->SetState(InternalStateType::PRE_SPAWN_DELAY);
		return;
		
	case InternalStateType::PRE_SPAWN_DELAY:
		if (this->m_ctSpawnDelay.IsElapsed()) {
			this->m_iCountSpawned = 0;
			this->m_iCountToSpawn = 0;
			this->SetState(InternalStateType::SPAWNING);
		}
		return;
		
	case InternalStateType::SPAWNING:
		if (!this->m_ctSpawnDelay.IsElapsed() ||
			g_pPopulationManager->m_bIsPaused) {
			return;
		}
		
		if (this->m_Spawner == nullptr) {
			Warning("Invalid spawner\n");
			this->SetState(InternalStateType::DONE);
			return;
		}
		
		int num_active = 0;
		FOR_EACH_VEC(this->m_ActiveBots, i) {
			CBaseEntity *ent = this->m_ActiveBots[i]();
			if (ent != nullptr && ent->IsAlive()) {
				++num_active;
			}
		}
		
		if (this->m_bWaitBetweenSpawnsAfterDeath) {
			if (num_active != 0) {
				return;
			}
			
			if (this->m_iSpawnResult != SPAWN_FAIL) {
				this->m_iSpawnResult = SPAWN_FAIL;
				
				float wait_between_spawns = this->m_flWaitBetweenSpawns;
				if (wait_between_spawns != 0.0f) {
					this->m_ctSpawnDelay.Set(wait_between_spawns);
				}
				
				return;
			}
		}
		
		int max_active = this->m_iMaxActive;
		if (num_active >= max_active) {
			return;
		}
		
		if (this->m_iCountToSpawn <= 0) {
			if (num_active + this->m_iSpawnCount > max_active) {
				return;
			}
			
			int spawn_count = this->m_iSpawnCount;
			if (CWaveSpawnPopulator::m_reservedPlayerSlotCount + spawn_count +
				GetGlobalTeam(TF_TEAM_BLUE)->GetNumPlayers() > 22) {
				return;
			}
			
			this->m_iCountToSpawn = spawn_count;
			CWaveSpawnPopulator::m_reservedPlayerSlotCount += spawn_count;
		}
		
		Vector vec_spawn = vec3_origin;
		if (this->m_Spawner->IsWhereRequired()) {
			if (this->m_iSpawnResult != SPAWN_NORMAL) {
				this->m_iSpawnResult = this->m_Where->FindSpawnLocation(&this->m_vecSpawn);
				if (this->m_iSpawnResult == SPAWN_FAIL) {
					return;
				}
			}
			
			vec_spawn = this->m_vecSpawn;
			if (this->m_bRandomSpawn) {
				this->m_iSpawnResult = SPAWN_FAIL;
			}
		}
		
		CUtlVector<CHandle<CBaseEntity>> spawned;
		if (this->m_Spawner->Spawn(&vec_spawn, &spawned) == 0) {
			this->m_ctSpawnDelay.Set(1.0f);
			return;
		}
		
		FOR_EACH_VEC(spawned, i) {
			CBaseEntity *ent = spawned[i]();
			
			CTFBot *bot = ToTFBot(ent);
			if (bot == nullptr) {
				CTFTankBoss *tank = dynamic_cast<CTFTankBoss *>(ent);
				if (tank != nullptr) {
					// TODO: set tank+0x9c4 dword to 0 (presumably this is m_nCurrency?)
					// TODO: set tank+0xe90 CWaveSpawnPopulator* to this
					// TODO: increment this->m_Wave->field_28
				}
				
				continue;
			}
			
			bot->m_nCurrency = 0;
			// TODO: set CTFPlayer offset 0x284c of bot (CWaveSpawnPopulator*) to this
			
			TFObjectiveResource()->SetMannVsMachineWaveClassActive(
				bot->GetPlayerClass()->m_iszClassIcon, true);
			
			if (this->m_bSupportLimited) {
				// TODO: set CTFPlayer bool offset 0x2836 (support limited) to true
			}
			
			if (this->m_iSpawnResult == SPAWN_TELEPORT) {
				OnBotTeleported(bot);
			}
		}
		
		int num_spawned = spawned.Count();
		this->m_iCountSpawned += num_spawned;
		
		int count_to_spawn = this->m_iCountToSpawn;
		if (num_spawned > count_to_spawn) {
			num_spawned = count_to_spawn;
		}
		
		CWaveSpawnPopulator::m_reservedPlayerSlotCount -= num_spawned;
		this->m_iCountToSpawn -= num_spawned;
		
		FOR_EACH_VEC(spawned, i) {
			CBaseEntity *ent1 = spawned[i]();
			
			FOR_EACH_VEC(this->m_ActiveBots, j) {
				CBaseEntity *ent2 = this->m_ActiveBots[j]();
				if (ent2 == nullptr) {
					continue;
				}
				
				if (ENTINDEX(ent1) == ENTINDEX(ent2)) {
					Warning("WaveSpawn duplicate entry in active vector\n");
				}
			}
			
			this->m_ActiveBots.AddToTail(ent1);
		}
		
		if (this->IsFinishedSpawning()) {
			this->SetState(InternalStateType::WAIT_FOR_ALL_DEAD);
		} else if (this->m_iCountToSpawn <= 0 &&
			!this->m_bWaitBetweenSpawnsAfterDeath) {
			this->m_iSpawnResult = SPAWN_FAIL;
			
			float wait_between_spawns = this->m_flWaitBetweenSpawns;
			if (wait_between_spawns != 0.0f) {
				this->m_ctSpawnDelay.Set(wait_between_spawns);
			}
		}
		
		return;
		
	case InternalStateType::WAIT_FOR_ALL_DEAD:
		FOR_EACH_VEC(this->m_ActiveBots, i) {
			CBaseEntity *ent = this->m_ActiveBots[i]();
			if (ent != nullptr && ent->IsAlive()) {
				return;
			}
		}
		
		this->SetState(InternalStateType::DONE);
		return;
	}
}

void CWave::Update()
{
	VPROF_BUDGET("CWave::Update", "NextBot");
	
	gamerules_roundstate_t roundstate = TFGameRules()->State_Get();
	
	if (roundstate == GR_STATE_RND_RUNNING) {
		this->ActiveWaveUpdate();
	} else if (roundstate == GR_STATE_TEAM_WIN ||
		roundstate == GR_STATE_BETWEEN_RNDS) {
		this->WaveIntermissionUpdate();
	}
	
	// TODO: stuff involving CPopulationManager
	// and WaveCompleteUpdate
}


void IPopulator::UnpauseSpawning()
{
}

void CPeriodicSpawnPopulator::UnpauseSpawning()
{
	// TODO
}

void CMissionPopulator::UnpauseSpawning()
{
	this->m_ctCooldown.Start(this->m_flCooldownTime);
	this->m_ctSBCooldown.Start(RandomFloat(5.0f, 10.0f));
}


void IPopulator::OnPlayerKilled(CTFPlayer *player)
{
}

void CWaveSpawnPopulator::OnPlayerKilled(CTFPlayer *player)
{
	this->m_ActiveBots.FindAndFastRemove(player);
}

void CWave::OnPlayerKilled(CTFPlayer *player)
{
	FOR_EACH_VEC(this->m_WaveSpawns, i) {
		this->m_WaveSpawns[i]->OnPlayerKilled(player);
	}
}


bool IPopulator::HasEventChangeAttributes(const char *name) const
{
	if (this->m_Spawner == nullptr) {
		return false;
	}
	
	return this->m_Spawner->HasEventChangeAttributes(name);
}

bool CWave::HasEventChangeAttributes(const char *name) const
{
	FOR_EACH_VEC(this->m_WaveSpawns, i) {
		if (this->m_WaveSpawns[i]->HasEventChangeAttributes(name)) {
			return true;
		}
	}
	
	return false;
}


bool CMissionPopulator::UpdateMission(CTFBot::MissionType mtype)
{
	VPROF_BUDGET("CMissionPopulator::UpdateMission", "NextBot");
	
	Vector<CTFPlayer *> blu_players;
	CollectPlayers<CTFPlayer>(&players, TF_TEAM_BLUE, true);
	
	int num_existing_bots_with_same_mission = 0;
	FOR_EACH_VEC(blu_players, i) {
		CTFBot *bot = dynamic_cast<CTFBot *>(blu_players[i]);
		if (bot != nullptr) {
			if (bot->m_nMission == mtype) {
				++num_existing_bots_with_same_mission;
			}
		}
	}
	
	if (g_pPopulationManager->m_bIsPaused) {
		return false;
	}
	
	if (num_existing_bots_with_same_mission != 0) {
		this->m_ctCooldown.Start(this->m_flCooldownTime);
		return false;
	}
	
	if (!this->m_ctCooldown.IsElapsed()) {
		return false;
	}
	
	if (GetGlobalTeam(TF_TEAM_BLUE)->GetNumPlayers() > 22) {
		if (tf_populator_debug.GetBool()) {
			DevMsg("MANN VS MACHINE: %3.2f: Waiting for slots to spawn mission.\n",
				gpGlobals->curtime);
		}
		
		return false;
	}
	
	if (tf_populator_debug.GetBool()) {
		DevMsg("MANN VS MACHINE: %3.2f: <<<< Spawning Mission >>>>\n",
			gpGlobals->curtime);
	}
	
	int num_sniper_bots = 0;
	FOR_EACH_VEC(blu_players, i) {
		CTFBot *bot = dynamic_cast<CTFBot *>(blu_players[i]);
		if (bot != nullptr) {
			if (bot->IsPlayerClass(TF_CLASS_SNIPER)) {
				++num_sniper_bots;
			}
		}
	}
	
	for (int i = 0; i < this->m_iDesiredCount; ++i) {
		Vector where;
		SpawnResult where_result =
			this->m_Where->FindSpawnLocation(&where);
		
		if (where_result != SPAWN_FAIL) {
			CUtlVector<CHandle<CBaseEntity>> spawned;
			if (this->m_Spawner->Spawn(&where, &spawned) != 0) {
				FOR_EACH_VEC(spawned, j) {
					CBaseEntity *ent = spawned[j]();
					
					CTFBot *bot = ToTFBot(ent);
					if (bot == nullptr) {
						continue;
					}
					
					bot->SetFlagTarget(nullptr);
					bot->SetMission(mtype, true);
					bot->m_bMissionBot = true;
					
					if (TFObjectiveResource() != nullptr) {
						// TODO: define bits for wave status flags
						
						// TODO
					}
					
					if (TFGameRules()->IsMannVsMachineMode()) {
						if (bot->m_nMission == CTFBot::MissionType::SNIPER) {
							++num_sniper_bots;
							if (num_sniper_bots == 1) {
								TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed(TLK_MVM_SNIPER_CALLOUT,
									TF_TEAM_RED);
							}
						}
					}
					
					if (where_result == SPAWN_TELEPORT) {
						OnBotTeleported(bot);
					}
				}
			}
		} else {
			if (tf_populator_debug.GetBool()) {
				DevMsg("MissionPopulator: %3.2f: Skipped a member - can't find a place to spawn\n",
					gpGlobals->curtime);
			}
		}
	}
	
	this->m_ctCooldown.Start(this->m_flCooldownTime);
	return true;
}

bool CMissionPopulator::UpdateMissionDestroySentries()
{
	VPROF_BUDGET("CMissionPopulator::UpdateMissionDestroySentries", "NextBot");
	
	if (!this->m_ctCooldown.IsElapsed()) {
		return false;
	}
	
	if (!this->m_ctSBCooldown.IsElapsed() || g_pPopulationManager->m_bIsPaused) {
		return false;
	}
	this->m_ctSBCooldown.Start(RandomFloat(5.0f, 10.0f));
	
	CUtlVector<CObjectSentrygun *> sentries;
	
	int thresh_dmg;
	int thresh_kills;
	this->m_pPopMgr->GetSentryBusterDamageAndKillThreshold(thresh_dmg, thresh_kills);
	
	for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
		CBaseObject *obj = static_cast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
		
		if (obj->GetType() == OBJ_SENTRYGUN && !obj->m_bDisposableBuilding &&
			obj->GetTeamNumber() == TF_TEAM_RED) {
			CTFPlayer *owner = obj->GetOwner();
			if (owner != nullptr) {
				if ((int)owner->m_flSentryDamage >= thresh_dmg ||
					owner->m_nSentryKills >= thresh_kills) {
					sentries.AddToTail(static_cast<CObjectSentrygun *>(obj));
				}
			}
		}
	}
	
	Vector<CTFPlayer *> blu_players;
	CollectPlayers<CTFPlayer>(&blu_players, TF_TEAM_BLUE, true, false);
	
	bool spawned_buster = false;
	FOR_EACH_VEC(sentries, i) {
		CObjectSentrygun *sentry = sentries[i];
		
		bool already_has_buster = false;
		FOR_EACH_VEC(blu_players, j) {
			CTFBot *bot = dynamic_cast<CTFBot *>(blu_players[j]);
			if (bot != nullptr) {
				if (bot->m_nMission == CTFBot::MissionType::DESTROY_SENTRIES) {
					if (sentry == bot->m_hSBTarget()) {
						already_has_buster = true;
						break;
					}
				}
			}
		}
		
		if (already_has_buster) {
			continue;
		}
		
		Vector where;
		SpawnResult where_result =
			this->m_Where->FindSpawnLocation(&where);
		
		if (where_result != SPAWN_FAIL) {
			CUtlVector<CHandle<CBaseEntity>> spawned;
			if (this->m_Spawner->Spawn(&where, &spawned) != 0) {
				if (tf_populator_debug.GetBool()) {
					DevMsg("MANN VS MACHINE: %3.2f: "
						"<<<< Spawning Sentry Busting Mission >>>>\n",
						gpGlobals->curtime);
				}
				
				FOR_EACH_VEC(spawned, j) {
					CBaseEntity *ent = spawned[j]();
					
					CTFBot *bot = ToTFBot(ent);
					if (bot == nullptr) {
						continue;
					}
					
					bot->SetFlagTarget(nullptr);
					bot->SetMission(CTFBot::MissionType::DESTROY_SENTRIES, true);
					bot->m_hSBTarget = sentry;
					bot->Update();
					bot->m_bMissionBot = true;
					
					bot->PlayerClass.SetCustomModel(
						"models/bots/demo/bot_sentry_buster.mdl", true);
					bot->UpdateModel();
					bot->SetBloodColor(DONT_BLEED);
					
					if (TFObjectiveResource() != nullptr) {
						unsigned int flags = CLASSFLAG_MISSION;
						if (bot->IsMiniBoss()) {
							flags |= CLASSFLAG_MINIBOSS;
						}
						if ((bot->m_nBotAttrs & CTFBot::AttributeType::ALWAYSCRIT) != 0) {
							flags |= CLASSFLAG_CRITICAL;
						}
						
						TFObjectiveResource()->IncrementMannVsMachineWaveClassCount(
							this->m_Spawner->GetClassIcon(j), flags);
					}
					
					if (TFGameRules()!= nullptr) {
						TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed(
							TLK_MVM_SENTRY_BUSTER, TF_TEAM_RED);
					}
					
					spawned_buster = true;
					
					if (where_result == SPAWN_TELEPORT) {
						OnBotTeleported(bot);
					}
				}
			}
		} else {
			if (tf_populator_debug.GetBool()) {
				DevMsg("MissionPopulator: %3.2f: "
					"Can't find a place to spawn a sentry destroying squad\n",
					gpGlobals->curtime);
			}
		}
	}
	
	if (spawned_buster) {
		float next_cooldown = this->m_flCooldownTime;
		
		CWave *wave = this->m_PopMgr->GetCurrentWave();
		if (wave != nullptr) {
			if (++wave->m_iSentryBustersSpawned <= 1) {
				if (TFGameRules() != nullptr) {
					TFGameRules()->BroadcastSound(255,
						"Announcer.MVM_Sentry_Buster_Alert");
				}
			} else {
				if (TFGameRules() != nullptr) {
					TFGameRules()->BroadcastSound(255,
						"Announcer.MVM_Sentry_Buster_Alert_Another");
				}
			}
			
			next_cooldown *= ((float)wave->m_iSentryBustersKilled + 1.0f);
			wave->m_iSentryBustersKilled = 0;
		}
		
		this->m_ctCooldown.Start(next_cooldown);
	}
	
	return spawned_buster;
}


/* this is inlined in CWaveSpawnPopulator::Update */
bool CWaveSpawnPopulator::IsFinishedSpawning()
{
	if (this->m_bSupport && !this->m_bSupportLimited) {
		return false;
	}
	
	return (this->m_iCountSpawned >= this->m_iTotalCount);
}

bool CWave::IsDoneWithNonSupportWaves()
{
	FOR_EACH_VEC(this->m_WaveSpawns, i) {
		CWaveSpawnPopulator *wavespawn = this->m_WaveSpawns[i];
		
		if (wavespawn != nullptr && !wavespawn->m_bSupport &&
			wavespawn->m_iState != CWaveSpawnPopulator::InternalStateType::DONE) {
			return false;
		}
	}
	
	return true;
}


void CWaveSpawnPopulator::OnNonSupportWavesDone()
{
	if (!this->m_bSupport) {
		return;
	}
	
	int state = this->m_iState;
	if (state == InternalStateType::INITIAL ||
		state == InternalStateType::PRE_SPAWN_DELAY) {
		this->SetState(InternalStateType::DONE);
	} else if (state == InternalStateType::SPAWNING ||
		state == InternalStateType::WAIT_FOR_ALL_DEAD) {
		if (TFGameRules() != nullptr && this->m_iCurrencyLeft > 0) {
			TFGameRules()->DistributeCurrencyAmount(this->m_iCurrencyLeft,
				nullptr, true, true, false);
			this->m_iCurrencyLeft = 0;
		}
		
		this->SetState(InternalStateType::WAIT_FOR_ALL_DEAD);
	}
}


void CWaveSpawnPopulator::ForceFinish()
{
	int state = this->m_iState;
	if (state == InternalStateType::INITIAL ||
		state == InternalStateType::PRE_SPAWN_DELAY ||
		state == InternalStateType::SPAWNING) {
		this->SetState(InternalStateType::WAIT_FOR_ALL_DEAD);
	} else if (state != InternalStateType::WAIT_FOR_ALL_DEAD) {
		this->SetState(InternalStateType::DONE);
	}
	
	FOR_EACH_VEC(this->m_ActiveBots, i) {
		CBaseEntity *ent = this->m_ActiveBots[i]();
		
		CTFBot *bot = ToTFBot(ent);
		if (bot != nullptr) {
			bot->ChangeTeam(TEAM_SPECTATOR, false, true);
		} else {
			ent->Remove();
		}
	}
	
	this->m_ActiveBots.RemoveAll();
}

void CWave::ForceFinish()
{
	FOR_EACH_VEC(this->m_WaveSpawns, i) {
		this->m_WaveSpawns[i]->ForceFinish();
	}
}


void CWave::ForceReset()
{
	// TODO: this->byte_0x020 = false
	// TODO: this->byte_0x021 = false
	// TODO: this->dword_0x288 = 0
	// TODO: this->byte_0x28c = false
	// TODO: this->dword_0x2a0 = 0
	
	// TODO: call Invalidate() on CountdownTimer @ 0x290
	
	FOR_EACH_VEC(this->m_WaveSpawns, i) {
		CWaveSpawnPopulator *wavespawn = this->m_WaveSpawns[i];
		
		// TODO: probably put these into an assumed inline function
		// (at least the last two, which pop up in CWaveSpawnPopulator::Parse)
		wavespawn->m_iState = CWaveSpawnPopulator::InternalStateType::INITIAL;
		wavespawn->m_iCurrencyLeft = wavespawn->m_iTotalCurrency;
		wavespawn->m_iCountNotYetSpawned = wavespawn->m_iTotalCount;
	}
}


int CWaveSpawnPopulator::GetCurrencyAmountPerDeath()
{
	if (this->m_bSupport &&
		this->m_iState == InternalStateType::WAIT_FOR_ALL_DEAD) {
		this->m_iCountNotYetSpawned = this->m_ActiveBots.Count();
	}
	
	int currency_left = this->m_iCurrencyLeft;
	if (currency_left <= 0) {
		return 0;
	}
	
	int bots_left = this->m_iCountNotYetSpawned;
	if (bots_left <= 0) {
		bots_left = 1;
	}
	
	int amount = (currency_left / bots_left);
	--this->m_iCountNotYetSpawned;
	this->m_iCurrencyLeft -= amount;
	
	return amount;
}


/* parts of this function are inlined into CWaveSpawnPopulator::Update */
void CWaveSpawnPopulator::SetState(CWaveSpawnPopulator::InternalStateType newstate)
{
	this->m_iState = newstate;
	
	/* specifying teamnum 255 in event "teamplay_broadcast_audio" is a special
	 * case that causes the sound to play for players on all teams; see
	 * ClientModeShared::FireGameEvent in clientmode_shared.cpp */
	
	if (newstate == InternalStateType::PRE_SPAWN_DELAY) {
		if (this->m_strStartWaveWarningSound.Length() > 0) {
			TFGameRules()->BroadcastSound(255,
				this->m_strStartWaveWarningSound());
		}
		
		FireEvent(this->m_StartWaveOutput, "StartWaveOutput");
		
		if (tf_populator_debug.GetBool()) {
			DevMsg("%3.2f: WaveSpawn(%s) started PRE_SPAWN_DELAY\n",
				gpGlobals->curtime, this->m_strName());
		}
	} else if (newstate == InternalStateType::SPAWNING) {
		if (this->m_strFirstSpawnWarningSound.Length() > 0) {
			TFGameRules()->BroadcastSound(255,
				this->m_strFirstSpawnWarningSound());
		}
		
		FireEvent(this->m_FirstSpawnOutput, "FirstSpawnOutput");
		
		if (tf_populator_debug.GetBool()) {
			DevMsg("%3.2f: WaveSpawn(%s) started SPAWNING\n",
				gpGlobals->curtime, this->m_strName());
		}
	} else if (newstate == InternalStateType::WAIT_FOR_ALL_DEAD) {
		if (this->m_strLastSpawnWarningSound.Length() > 0) {
			TFGameRules()->BroadcastSound(255,
				this->m_strLastSpawnWarningSound());
		}
		
		FireEvent(this->m_LastSpawnOutput, "LastSpawnOutput");
		
		if (tf_populator_debug.GetBool()) {
			DevMsg("%3.2f: WaveSpawn(%s) started WAIT_FOR_ALL_DEAD\n",
				gpGlobals->curtime, this->m_strName());
		}
	} else if (newstate == InternalStateType::DONE) {
		if (this->m_strDoneWarningSound.Length() > 0) {
			TFGameRules()->BroadcastSound(255,
				this->m_strDoneWarningSound());
		}
		
		FireEvent(this->m_DoneOutput, "DoneOutput");
		
		if (tf_populator_debug.GetBool()) {
			DevMsg("%3.2f: WaveSpawn(%s) DONE\n",
				gpGlobals->curtime, this->m_strName());
		}
	}
}


CWaveSpawnPopulator *CWave::FindWaveSpawnPopulator(const char *name)
{
	FOR_EACH_VEC(this->m_WaveSpawns, i) {
		CWaveSpawnPopulator *wavespawn = this->m_WaveSpawns[i];
		if (V_stricmp(wavespawn->m_strName(), name) == 0) {
			return wavespawn;
		}
	}
	
	return nullptr;
}

void CWave::AddClassType(string_t icon, int count, unsigned int flags)
{
	WaveClassCount_t *wcc = nullptr;
	
	FOR_EACH_VEC(this->m_ClassCounts, i) {
		WaveClassCount_t& wcc_i = this->m_ClassCounts[i];
		
		/* this is a string pointer comparison, not a content comparison */
		if (wcc_i.icon == icon && (wcc_i.flags & flags) != 0) {
			wcc = &wcc_i;
			break;
		}
	}
	
	if (wcc == nullptr) {
		WaveClassCount_t wcc_new = {
			.count = 0,
			.icon  = icon,
			.flags = 0,
		};
		
		wcc = &(this->m_ClassCounts.AddToTail(wcc_new));
	}
	
	wcc->count += count;
	wcc->flags |= flags;
}


void CWave::ActiveWaveUpdate()
{
	VPROF_BUDGET("CWave::ActiveWaveUpdate", "NextBot");
	
	// TODO
}

void CWave::WaveCompleteUpdate()
{
	// TODO
}

void CWave::WaveIntermissionUpdate()
{
	// TODO
}


// fastcall
// eax: kv
EventInfo *ParseEvent(KeyValues *kv)
{
	EventInfo *info = new EventInfo();
	
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (V_stricmp(name, "Target") == 0) {
			info->target.sprintf(subkey->GetString());
		} else if (V_stricmp(name, "Action") == 0) {
			info->action.sprintf(subkey->GetString());
		} else {
			Warning("Unknown field '%s' in WaveSpawn event definition.\n",
				subkey->GetString());
			delete info;
			return nullptr;
		}
	}
	
	return info;
}

// fastcall
// eax: info
// edx: name
void FireEvent(EventInfo *info, const char *name)
{
	if (info == nullptr) {
		return;
	}
	
	const char *target = info->target.Get();
	const char *action = info->action.Get();
	
	CBaseEntity *ent = gEntList.FindEntityByName(nullptr, target);
	if (ent != nullptr) {
		g_EventQueue.AddEvent(ent, action, 0.0f, nullptr, nullptr);
	} else {
		Warning("WaveSpawnPopulator: Can't find target entity '%s' for %s\n",
			target, name);
	}
}


static CHandle<CBaseEntity> s_lastTeleporter;
static float s_flLastTeleportTime;


SpawnResult DoTeleporterOverride(CBaseEntity *teamspawn, Vector& vec)
{
	CUtlVector<CBaseEntity *> teleporters;
	
	for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
		CBaseObject *obj = static_cast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
		
		// TODO: not sure if this is really the right enum name for object type
		if (obj->GetType() == OBJ_TELEPORTER &&
			obj->GetTeamNumber() == TF_TEAM_BLUE &&
			!obj->IsBuilding() && !obj->HasSapper()) {
			CObjectTeleporter *tele = static_cast<CObjectTeleporter *>(obj);
			const char *spawn_name = STRING(teamspawn->GetEntityName());
			
			/* CObjectTeleporter+0xaec: CUtlStringList m_TeleportWhere */
			FOR_EACH_VEC(tele->m_TeleportWhere, j) {
				if (V_stricmp(tele->m_TeleportWhere[j], spawn_name) == 0) {
					teleporters.AddToTail(tele);
					break;
				}
			}
		}
	}
	
	if (!teleporters.IsEmpty()) {
		CBaseEntity *tele = teleporters.Random();
		
		vec = tele->WorldSpaceCenter();
		s_lastTeleporter = tele;
		
		return SPAWN_TELEPORT;
	}
	
	CNavArea *area = TheNavMesh->GetNearestNavArea(teamspawn->WorldSpaceCenter());
	if (area == nullptr) {
		return SPAWN_FAIL;
	}
	
	vec = area->GetCenter();
	return SPAWN_NORMAL;
}

void OnBotTeleported(CTFBot *bot)
{
	CBaseEntity *tele = s_lastTeleporter();
	
	// TODO
	
	
	
	
	// TODO
}
