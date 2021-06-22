/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_populator_spawners.cpp
 */


IPopulationSpawner::IPopulationSpawner(IPopulator *populator)
	: m_Populator(populator)
{
}

CMobSpawner::CMobSpawner(IPopulator *populator)
	: IPopulationSpawner(populator)
{
	this->m_iCount     = 0;
	this->m_SubSpawner = nullptr;
}

CSentryGunSpawner::CSentryGunSpawner(IPopulator *populator)
	: IPopulationSpawner(populator)
{
	this->m_iLevel = 0;
}

CTankSpawner::CTankSpawner(IPopulator *populator)
	: IPopulationSpawner(populator)
{
	this->m_iHealth             = 50000;
	this->m_flSpeed             = 75.0f;
	this->m_strName             = "Tank";
	this->m_strStartNode        = nullptr;
	this->m_iSkin               = 0;
	this->m_OnKilledOutput      = nullptr;
	this->m_OnBombDroppedOutput = nullptr;
}

CTFBotSpawner::CTFBotSpawner(IPopulator *populator)
	: IPopulationSpawner(populator)
{
	this->m_iClass        = TF_CLASS_UNDEFINED;
	this->m_strClassIcon  = NULL_STRING;
	this->m_iHealth       = -1;
	this->m_flScale       = -1.0f;
	this->m_flAutoJumpMax = 0.0f;
	this->m_flAutoJumpMin = 0.0f;
	
	/* apparently it wasn't enough to reset this once with the default ctor,
	 * so they reset it again here */
	this->m_DefaultAttrs.Reset();
}

CSquadSpawner::CSquadSpawner(IPopulator *populator)
	: IPopulationSpawner(populator)
{
	this->m_flFormationSize      = -1.0f;
	this->m_bShouldPreserveSquad = false;
}

CRandomChoiceSpawner::CRandomChoiceSpawner(IPopulator *populator)
	: IPopulationSpawner(populator)
{
	this->m_iSpawned = 0;
}


IPopulationSpawner::~IPopulationSpawner()
{
}

CMobSpawner::~CMobSpawner()
{
	if (this->m_SubSpawner != nullptr) {
		delete this->m_SubSpawner;
	}
}

CSentryGunSpawner::~CSentryGunSpawner()
{
}

CTankSpawner::~CTankSpawner()
{
	/* likely bug: the EventInfo members were new'd, but are not delete'd */
}

CTFBotSpawner::~CTFBotSpawner()
{
}

CSquadSpawner::~CSquadSpawner()
{
}

CRandomChoiceSpawner::~CRandomChoiceSpawner()
{
}


bool CMobSpawner::Parse(KeyValues *kv)
{
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (V_stricmp(name, "Count") == 0) {
			this->m_iCount = subkey->GetInt();
		} else {
			IPopulationSpawner *spawner =
				IPopulationSpawner::ParseSpawner(this->m_Populator, subkey);
			if (spawner != nullptr) {
				if (this->m_SubSpawner == nullptr) {
					this->m_SubSpawner = spawner;
				} else {
					Warning("CMobSpawner: Duplicate spawner encountered - discarding!\n");
					delete spawner;
				}
			} else {
				Warning("Unknown attribute '%s' in Mob definition.\n", name);
			}
		}
	}
	
	return true;
}

bool CSentryGunSpawner::Parse(KeyValues *kv)
{
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (V_stricmp(name, "Level") == 0) {
			this->m_iLevel = subkey->GetInt();
		} else {
			Warning("Invalid attribute '%s' in SentryGun definition\n", name);
			return false;
		}
	}
	
	return true;
}

bool CTankSpawner::Parse(KeyValues *kv)
{
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (V_stricmp(name, "Health") == 0) {
			this->m_iHealth = subkey->GetInt();
		} else if (V_stricmp(name, "Speed") == 0) {
			this->m_flSpeed = subkey->GetFloat();
		} else if (V_stricmp(name, "Name") == 0) {
			this->m_strName = subkey->GetString();
		} else if (V_stricmp(name, "Skin") == 0) {
			this->m_iSkin = subkey->GetInt();
		} else if (V_stricmp(name, "StartingPathTrackNode") == 0) {
			this->m_strStartNode = subkey->GetString();
		} else if (V_stricmp(name, "OnKilledOutput") == 0) {
			this->m_OnKilledOutput = ParseEvent(subkey);
		} else if (V_stricmp(name, "OnBombDroppedOutput") == 0) {
			this->m_OnBombDroppedOutput = ParseEvent(subkey);
		} else {
			Warning("Invalid attribute '%s' in Tank definition\n", name);
			return false;
		}
	}
	
	return true;
}

bool CTFBotSpawner::Parse(KeyValues *kv)
{
	this->m_iClass        = TF_CLASS_UNDEFINED;
	this->m_strClassIcon  = NULL_STRING;
	this->m_iHealth       = -1;
	this->m_flScale       = -1.0f;
	this->m_flAutoJumpMax = 0.0f;
	this->m_flAutoJumpMin = 0.0f;
	
	this->m_DefaultAttrs.Reset();
	
	this->m_ECAttrs.RemoveAll();
	
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
			Warning("Unknown Template '%s' in TFBotSpawner definition\n",
				tname);
		}
	}
	
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (V_stricmp(name, "Template") == 0) {
			continue;
		}
		
		if (V_stricmp(name, "Class") == 0) {
			const char *str = subkey->GetString();
			if ((this->m_iClass = GetClassIndexFromString(str, 10)) !=
				TF_CLASS_UNDEFINED) {
				if (this->m_strName.IsEmpty()) {
					this->m_strName = str;
				}
			} else {
				Warning("TFBotSpawner: Invalid class '%s'\n", str);
				return false;
			}
		} else if (V_stricmp(name, "ClassIcon") == 0) {
			this->m_strClassIcon =
				AllocPooledString(subkey->GetString());
		} else if (V_stricmp(name, "Health") == 0) {
			this->m_iHealth = subkey->GetInt();
		} else if (V_stricmp(name, "Scale") == 0) {
			this->m_flScale = subkey->GetFloat();
		} else if (V_stricmp(name, "Name") == 0) {
			this->m_strName = subkey->GetString();
		} else if (V_stricmp(name, "TeleportWhere") == 0) {
			this->m_TeleportWhere.CopyAndAddToTail(subkey->GetString());
		} else if (V_stricmp(name, "AutoJumpMin") == 0) {
			this->m_flAutoJumpMin = subkey->GetFloat();
		} else if (V_stricmp(name, "AutoJumpMax") == 0) {
			this->m_flAutoJumpMax = subkey->GetFloat();
		} else {
			if (V_stricmp(name, "EventChangeAttributes") == 0) {
				if (!this->ParseEventChangeAttributes(subkey)) {
					Warning("TFBotSpawner: Failed to parse EventChangeAttributes\n");
					return false;
				}
			} else {
				if (!ParseDynamicAttributes(this->m_DefaultAttrs, subkey)) {
					Warning("TFBotSpawner: Unknown field '%s'\n", name);
					return false;
				}
			}
		}
	}
	
	return true;
}

bool CSquadSpawner::Parse(KeyValues *kv)
{
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		if (V_stricmp(name, "FormationSize") == 0) {
			this->m_flFormationSize = subkey->GetFloat();
		} else if (V_stricmp(name, "ShouldPreserveSquad") == 0) {
			this->m_bShouldPreserveSquad = subkey->GetBool();
			/* BUG: ShouldPreserveSquad is parsed correctly, but it falls thru
			 * into the spawner parsing part of the code, which will give an
			 * "Unknown attribute 'ShouldPreserveSquad' in Squad definition"
			 * error in console */
			goto fall_thru;
		} else {
		fall_thru:
			IPopulationSpawner *spawner =
				IPopulationSpawner::ParseSpawner(this->m_Populator, subkey);
			if (spawner != nullptr) {
				this->m_SubSpawners.AddToTail(spawner);
			} else {
				Warning("Unknown attribute '%s' in Squad definition.\n", name);
			}
		}
	}
	
	return true;
}

bool CRandomChoiceSpawner::Parse(KeyValues *kv)
{
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		if (strlen(name) <= 0) {
			continue;
		}
		
		IPopulationSpawner *spawner =
			IPopulationSpawner::ParseSpawner(this->m_Populator, subkey);
		if (spawner != nullptr) {
			this->m_SubSpawners.AddToTail(spawner);
		} else {
			Warning("Unknown attribute '%s' in RandomChoice definition.\n",
				name);
		}
	}
	
	return true;
}


int CMobSpawner::Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
{
	if (this->m_SubSpawner == nullptr) {
		return 0;
	}
	
	for (int i = 0; i < this->m_iCount; ++i) {
		if (this->m_SubSpawner->Spawn(where, ents) == 0) {
			return 0;
		}
	}
	
	return 1;
}

int CSentryGunSpawner::Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
{
	CBaseEntity *ent = CreateEntityByName("obj_sentrygun");
	if (ent == nullptr) {
		if (tf_populator_debug.GetBool()) {
			DevMsg("CSentryGunSpawner: %3.2f: Failed to create obj_sentrygun\n",
				gpGlobals->curtime);
		}
		
		return 0;
	}
	
	CObjectSentrygun *sentry = static_cast<CObjectSentrygun *>(ent);
	
	sentry->SetAbsOrigin(where);
	sentry->SetAbsAngles(vec3_angle);
	sentry->Spawn();
	sentry->ChangeTeam(TF_TEAM_BLUE);
	
	/* undocumented: CBaseObject+0x9e4 */
	sentry->m_iUpgradeLevelDesired = this->m_iLevel + 1;
	
	sentry->InitializeMapPlacedObject();
	
	if (ents != nullptr) {
		ents->AddToTail(sentry);
	}
	
	return 1;
}

int CTankSpawner::Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
{
	CBaseEntity *ent = CreateEntityByName("tank_boss");
	if (ent == nullptr) {
		if (tf_populator_debug.GetBool()) {
			DevMsg("CTankSpawner: %3.2f: Failed to create base_boss\n",
				gpGlobals->curtime);
		}
		
		return 0;
	}
	
	CTFTankBoss *tank = static_cast<CTFTankBoss *>(ent);
	
	tank->SetAbsOrigin(where);
	tank->SetAbsAngles(vec3_angle);
	tank->m_initialHealth = (int)((float)this->m_iHealth *
		g_pPopulationManager->GetHealthMultiplier(true));
	tank->m_speed = this->m_flSpeed;
	tank->SetName(MAKE_STRING(this->m_strName.Get()));
	tank->SetSkin(this->m_iSkin);
	tank->SetStartingPathTrackNode(this->m_strStartNode.GetForModify());
	tank->Spawn();
	
	if (this->m_OnKilledOutput != nullptr) {
		/* implicit member-by-member copy of the EventInfo struct */
		tank->m_OnKilledOutput = *this->m_OnKilledOutput;
	}
	if (this->m_OnBombDroppedOutput != nullptr) {
		/* implicit member-by-member copy of the EventInfo struct */
		tank->m_OnBombDroppedOutput = *this->m_OnBombDroppedOutput;
	}
	
	if (ents != nullptr) {
		ents->AddToTail(tank);
	}
	
	return 1;
}

int CTFBotSpawner::Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
{
	static CHandle<CBaseEntity> g_internalSpawnPoint;
	
	VPROF_BUDGET("CTFBotSpawner::Spawn", "NextBot");
	
	CNavArea *area = TheNavMesh->GetNavArea(where, 120.0f);
	if ((area->m_nAttributes & NO_SPAWNING) != 0) {
		if (tf_populator_debug.GetBool()) {
			DevMsg("CTFBotSpawner: %3.2f: *** Tried to spawn in a NO_SPAWNING area at (%f, %f, %f)\n",
				gpGlobals->curtime, where.x, where.y, where.z);
		}
		
		return 0;
	}
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() &&
		TFGameRules()->State_Get() != GR_STATE_RND_RUNNING) {
		return 0;
	}
	
	Vector where_modified = where;
	
	bool is_space = false;
	for (float dz = 0.0f; dz < 18.0f; dz += 4.0f) {
		/* bug: should probably be adding dz here, not 18.0f every time */
		where_modified.z = where.z + 18.0f;
		
		if (IsSpaceToSpawnHere(where_modified)) {
			is_space = true;
			break;
		}
	}
	
	if (!is_space) {
		if (tf_populator_debug.GetBool()) {
			DevMsg("CTFBotSpawner: %3.2f: *** No space to spawn at (%f, %f, %f)\n",
				gpGlobals->curtime, where.x, where.y, where.z);
		}
		
		return 0;
	}
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() &&
		this->m_iClass == TF_CLASS_ENGINEER &&
		(this->m_DefaultAttrs.m_nBotAttrs & CTFBot::AttributeType::TELEPORTTOHINT) != 0 &&
		!CTFBotMvMEngineerHintFinder::FindHint(true, false, nullptr)) {
		if (tf_populator_debug.GetBool()) {
			DevMsg("CTFBotSpawner: %3.2f: *** No teleporter hint for engineer\n",
				gpGlobals->curtime);
		}
		
		return 0;
	}
	
	CTeam *team_spec = GetGlobalTeam(TEAM_SPECTATOR);
	/* bug: should be using the initial number of spectators; by constantly
	 * updating the count, which will be decreasing, as we increment i, we may
	 * end up spawning less bots than we actually should be able to */
	for (int i = 0; i < team_spec->GetNumPlayers(); ++i) {
		CBasePlayer *player = team_spec->GetPlayer(i);
		if (!player->IsBot()) {
			continue;
		}
		
		CTFBot *bot = static_cast<CTFBot *>(team_spec->GetPlayer(i));
		
		// TODO: bot offset 0x2b14 dword = 0
		
		bot->RemovePlayerAttributes(false);
		
		if (*g_internalSpawnPoint == nullptr) {
			CBaseEntity *spawnpoint =
				CreateEntityByName("populator_internal_spawn_point");
			
			g_internalSpawnPoint = spawnpoint;
			g_internalSpawnPoint->Spawn();
		}
		
		// TODO: "TFBot" fallback string logic
		// from MAKE_STRING(this->m_strName.Get())
		// set const char *name accordingly
		
		engine->SetFakeClientConVarValue(bot->edict(), "name", name);
		
		g_internalSpawnPoint->SetAbsOrigin(where_modified);
		g_internalSpawnPoint->SetLocalAngles(vec3_angle);
		bot->SetSpawnPoint(g_internalSpawnPoint);
		
		bot->ChangeTeam((TFGameRules()->IsMannVsMachineMode() ?
			TF_TEAM_BLUE : TF_TEAM_RED), false, true);
		
		// TODO bot offset 0x9c0 byte = 0
		
		/* this is where CTFPlayer::Spawn is ultimately called */
		bot->HandleCommand_JoinClass(GetPlayerClassData(this->m_iClass)->m_szClassName);
		
		bot->GetPlayerClass()->m_iszClassIcon = STRING(this->GetClassIcon(-1));
		
		// TODO: CTFBot:: funcs:
		// AddEventChangeAttributes
		// GetEventChangeAttributes
		// OnEventChangeAttributes
		
		/* possible bug: doesn't purge old array members? */
		bot->m_ECAttrs.SetSize(0);
		FOR_EACH_VEC(this->m_ECAttrs, i) {
			bot->AddEventChangeAttributes(this->m_ECAttrs[i]);
		}
		
		if (g_pPopulationManager->IsInEndlessWaves()) {
			g_pPopulationManager->EndlessSetAttributesForBot(bot);
		}
		
		FOR_EACH_VEC(this->m_TeleportWhere, i) {
			bot->m_TeleportWhere.CopyAndAddToTail(this->m_TeleportWhere[i]);
		}
		
		if (this->HasAttribute(CTFBot::AttributeType::MINIBOSS, -1)) {
			bot->m_bIsMiniBoss = true;
		}
		if (this->HasAttribute(CTFBot::AttributeType::USEBOSSHEALTHBAR, -1)) {
			bot->m_bUseBossHealthBor = true;
		}
		
		if (this->HasAttribute(CTFBot::AttributeType::AUTOJUMP, -1)) {
			bot->m_flAutoJumpMin = this->m_flAutoJumpMin;
			bot->m_flAutoJumpMax = this->m_flAutoJumpMax;
		}
		
		if (this->HasAttribute(CTFBot::AttributeType::BULLETIMMUNE, -1)) {
			bot->m_Shared.AddCond(TF_COND_BULLET_IMMUNE, -1.0f, nullptr);
		}
		if (this->HasAttribute(CTFBot::AttributeType::BLASTIMMUNE, -1)) {
			bot->m_Shared.AddCond(TF_COND_BLAST_IMMUNE, -1.0f, nullptr);
		}
		if (this->HasAttribute(CTFBot::AttributeType::FIREIMMUNE, -1)) {
			bot->m_Shared.AddCond(TF_COND_FIRE_IMMUNE, -1.0f, nullptr);
		}
		
		if (TFGameRules()->IsMannVsMachineMode()) {
			bot->m_nCurrency = 0;
		}
		
		if (this->m_iClass == TF_CLASS_SPY) {
			CUtlVector<CTFPlayer *> blu_players;
			CollectPlayers<CTFPlayer>(&blu_players, TF_TEAM_BLUE, true, false);
			
			int num_spies = 0;
			FOR_EACH_VEC(blu_players, i) {
				if (blu_players[i]->IsPlayerClass(TF_CLASS_SPY)) {
					++num_spies;
				}
			}
			
			IGameEvent *event =
				gameeventmanager->CreateEvent("mvm_mission_update");
			if (event != nullptr) {
				event->SetInt("class", TF_CLASS_SPY);
				event->SetInt("count", num_spies);
				
				gameeventmanager->FireEvent(event);
			}
		}
		
		bot->m_flScale = this->m_flScale;
		
		float scale = this->m_flScale;
		if (scale < 0.0f) {
			scale = 1.0f;
		}
		bot->SetModelScale(scale);
		
		float health = (float)this->m_iHealth;
		if (health < 0.0f) {
			health = (float)bot->GetMaxHealth();
		}
		health *= g_pPopulationManager->GetHealthMultiplier(false);
		bot->ModifyMaxHealth((int)health, true, true);
		
		bot->StartIdleSound();
		
		if (TFGameRules()->IsMannVsMachineMode() &&
			bot->GetTeamNumber() == TF_TEAM_BLUE) {
			// TODO: determine for sure what is at CMissionPopulator+0x30
			// before we finish this block
			// (romevision stuff)
		}
		
		// TODO: what is at CPopulationManager+0x6d0?
		// (ecattr stuff)
		
		CCaptureFlag *flag = bot->GetFlagToFetch();
		if (flag != nullptr) {
			bot->SetFlagTarget(flag);
		}
		
		if ((bot->m_nBotAttrs & CTFBot::AttributeType::SPAWNWITHFULLCHARGE) != 0) {
			// TODO: slot enum
			CBaseCombatWeapon *w_secondary = bot->Weapon_GetSlot(1);
			if (w_secondary != nullptr) {
				CWeaponMedigun *w_medigun =
					dynamic_cast<CWeaponMedigun *>(w_secondary);
				if (w_medigun != nullptr) {
					w_medigun->AddCharge(1.00f);
				}
			}
			
			bot->m_Shared.SetRageMeter(100.0f);
		}
		
		int model_class = bot->GetPlayerClass()->GetClassIndex();
		// TODO: find out what's at CPopulationManager+1650?
		// presumably it's the EventPopfile value
		// TODO: holiday enum
		if (this->m_Populator->m_Popmgr->??? == 1 && TF_IsHolidayActive(10)) {
			bot->m_nSkin = 4;
			
			CFmtStrN<256> zombie_soul_name("Zombie %s",
				g_aRawPlayerClassNamesShort[model_class]);
			bot->AddItem(*zombie_soul_name);
		} else {
			if (model_class <= TF_CLASS_SPY) {
				const char *model      = g_szBotModels[model_class];
				const char *model_boss = g_szBotBossModels[model_class];
				
				if ((this->m_flScale >= tf_mvm_miniboss_scale.GetFloat() ||
					bot->IsMiniBoss()) &&
					g_pFullFileSystem->FileExists(model_boss)) {
					bot->PlayerClass.SetCustomModel(model_boss, true);
					bot->UpdateModel();
					bot->SetBloodColor(DONT_BLEED);
				} else if (g_pFullFileSystem->FileExists(model)) {
					bot->PlayerClass.SetCustomModel(model, true);
					bot->UpdateModel();
					bot->SetBloodColor(DONT_BLEED);
				}
			}
		}
		
		if (ents != nullptr) {
			ents->AddToTail(bot);
		}
		
		if (TFGameRules()->IsMannVsMachineMode() && bot->IsMiniBoss()) {
			TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed(
				TLK_MVM_GIANT_CALLOUT, TF_TEAM_RED);
		}
		
		if (tf_populator_debug.GetBool()) {
			DevMsg("%3.2f: Spawned TFBot '%s'\n",
				gpGlobals->curtime, name);
		}
		
		return 1;
	}
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode()) {
		CUtlVector<CTFPlayer *> mvm_bots;
		CPopulationManager::CollectMvMBots(&mvm_bots);
		
		if (mvm_bots.Count() > 21) {
			if (tf_populator_debug.GetBool()) {
				DevMsg("CTFBotSpawner: %3.2f: *** Can't spawn. Max number invaders already spawned.\n",
					gpGlobals->curtime);
			}
			
			if (mvm_bots.Count() != 22) {
				CUtlVector<CTFPlayer *> 
				// TODO
			}
		}
	}
	
	// TODO
}

int CSquadSpawner::Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
{
	VPROF_BUDGET("CSquadSpawner::Spawn", "NextBot");
	
	if (tf_populator_debug.GetBool()) {
		DevMsg("CSquadSpawner: %3.2f: <<<< Spawning Squad >>>>\n",
			gpGlobals->curtime);
	}
	
	if (GetGlobalTeam(TEAM_SPECTATOR)->GetNumPlayers() <
		this->m_SubSpawners.Count()) {
		return 0;
	}
	
	CUtlVector<CHandle<CBaseEntity>> spawned;
	
	bool failed = false;
	FOR_EACH_VEC(this->m_SubSpawners, i) {
		IPopulationSpawner *spawner = this->m_SubSpawners[i];
		if (spawner->Spawn(where, spawned) == 0) {
			failed = true;
			break;
		}
	}
	
	if (failed) {
		if (tf_populator_debug.GetBool()) {
			DevMsg("CSquadSpawner: %3.2f: Unable to spawn entire squad\n",
				gpGlobals->curtime);
		}
		
		FOR_EACH_VEC(spawned, i) {
			CBaseEntity *ent = spawned[i];
			if (ent != nullptr && ent->IsPlayer()) {
				CBasePlayer *player = static_cast<CBasePlayer *>(ent);
				player->ChangeTeam(TEAM_SPECTATOR, false, true);
			} else {
				UTIL_Remove(ent);
			}
		}
		
		return 0;
	}
	
	CTFBotSquad *squad = new CTFBotSquad();
	if (squad != nullptr) {
		squad->m_flFormationSize      = this->m_flFormationSize;
		squad->m_bShouldPreserveSquad = this->m_bShouldPreserveSquad;
		
		FOR_EACH_VEC(spawned, i) {
			CBaseEntity *ent = spawned[i];
			
			CTFBot *bot = ToTFBot(ent);
			if (bot != nullptr) {
				bot->JoinSquad(squad);
			}
		}
	}
	
	if (ents != nullptr) {
		ents->AddVectorToTail(spawned);
	}
	
	return 1;
}

int CRandomChoiceSpawner::Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
{
	int spawned = 0;
	
	if (!this->m_SubSpawners.IsEmpty()) {
		this->GenerateRandomIndexes(this->m_iSpawned);
		
		IPopulationSpawner *spawner = this->m_SubSpawners[this->m_iSpawned];
		spawned = spawner->Spawn(where, ents);
		
		++this->m_iSpawned;
	}
	
	return spawned;
}


bool IPopulationSpawner::IsWhereRequired()
{
	return true;
}

bool CTankSpawner::IsWhereRequired()
{
	return false;
}


bool IPopulationSpawner::IsVarious()
{
	return false;
}

bool CSquadSpawner::IsVarious()
{
	return true;
}

bool CRandomChoiceSpawner::IsVarious()
{
	return true;
}


int IPopulationSpawner::GetClass(int index)
{
	return TF_CLASS_UNDEFINED;
}

int CTFBotSpawner::GetClass(int index)
{
	return this->m_iClass;
}

int CSquadSpawner::GetClass(int index)
{
	if (index < 0 || this->m_SubSpawners.IsEmpty()) {
		return IPopulationSpawner::GetClass(index);
	}
	
	index %= this->m_SubSpawners.Count();
	IPopulationSpawner *spawner = this->m_SubSpawners[index];
	
	if (!spawner->IsVarious()) {
		return spawner->GetClass(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return TF_CLASS_UNDEFINED;
	}
}

int CRandomChoiceSpawner::GetClass(int index)
{
	if (index < 0) {
		return IPopulationSpawner::GetClass(index);
	}
	
	this->GenerateRandomIndexes(index);
	
	IPopulationSpawner *spawner = this->m_SubSpawners[this->m_Indexes[index]];
	if (!spawner->IsVarious()) {
		return spawner->GetClass(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return TF_CLASS_UNDEFINED;
	}
}


string_t IPopulationSpawner::GetClassIcon(int index)
{
	return NULL_STRING;
}

string_t CTankSpawner::GetClassIcon(int index)
{
	return MAKE_STRING("tank");
}

string_t CTFBotSpawner::GetClassIcon(int index)
{
	if (!this->m_strClassIcon) {
		return AllocPooledString(g_aRawPlayerClassNamesShort[this->m_iClass]);
	} else {
		return this->m_strClassIcon;
	}
}

string_t CSquadSpawner::GetClassIcon(int index)
{
	if (index < 0 || this->m_SubSpawners.IsEmpty()) {
		return IPopulationSpawner::GetClassIcon(index);
	}
	
	index %= this->m_SubSpawners.Count();
	IPopulationSpawner *spawner = this->m_SubSpawners[index];
	
	if (!spawner->IsVarious()) {
		return spawner->GetClassIcon(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return NULL_STRING;
	}
}

string_t CRandomChoiceSpawner::GetClassIcon(int index)
{
	if (index < 0) {
		return IPopulationSpawner::GetClassIcon(index);
	}
	
	this->GenerateRandomIndexes(index);
	
	IPopulationSpawner *spawner = this->m_SubSpawners[this->m_Indexes[index]];
	if (!spawner->IsVarious()) {
		return spawner->GetClassIcon(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return NULL_STRING;
	}
}


int IPopulationSpawner::GetHealth(int index)
{
	return 0;
}

int CTankSpawner::GetHealth(int index)
{
	return m_iHealth;
}

int CTFBotSpawner::GetHealth(int index)
{
	return m_iHealth;
}

int CSquadSpawner::GetHealth(int index)
{
	if (index < 0 || this->m_SubSpawners.IsEmpty()) {
		return IPopulationSpawner::GetHealth(index);
	}
	
	index %= this->m_SubSpawners.Count();
	IPopulationSpawner *spawner = this->m_SubSpawners[index];
	
	if (!spawner->IsVarious()) {
		return spawner->GetHealth(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return 0;
	}
}

int CRandomChoiceSpawner::GetHealth(int index)
{
	if (index < 0) {
		return IPopulationSpawner::GetHealth(index);
	}
	
	this->GenerateRandomIndexes(index);
	
	IPopulationSpawner *spawner = this->m_SubSpawners[this->m_Indexes[index]];
	if (!spawner->IsVarious()) {
		return spawner->GetHealth(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return 0;
	}
}


bool IPopulationSpawner::IsMiniBoss(int index)
{
	return false;
}

bool CTankSpawner::IsMiniBoss(int index)
{
	return true;
}

bool CTFBotSpawner::IsMiniBoss(int index)
{
	return ((this->m_DefaultAttrs.m_nBotAttrs & CTFBot::AttributeType::MINIBOSS) != 0);
}

bool CSquadSpawner::IsMiniBoss(int index)
{
	if (index < 0 || this->m_SubSpawners.IsEmpty()) {
		return IPopulationSpawner::IsMiniBoss(index);
	}
	
	index %= this->m_SubSpawners.Count();
	IPopulationSpawner *spawner = this->m_SubSpawners[index];
	
	if (!spawner->IsVarious()) {
		return spawner->IsMiniBoss(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return false;
	}
}

bool CRandomChoiceSpawner::IsMiniBoss(int index)
{
	if (index < 0) {
		return IPopulationSpawner::IsMiniBoss(index);
	}
	
	this->GenerateRandomIndexes(index);
	
	IPopulationSpawner *spawner = this->m_SubSpawners[this->m_Indexes[index]];
	if (!spawner->IsVarious()) {
		return spawner->IsMiniBoss(-1);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return false;
	}
}


bool IPopulationSpawner::HasAttribute(CTFBot::AttributeType attr, int index)
{
	return false;
}

bool CTFBotSpawner::HasAttribute(CTFBot::AttributeType attr, int index)
{
	return ((this->m_DefaultAttrs.m_nBotAttrs & attr) != 0);
}

bool CSquadSpawner::HasAttribute(CTFBot::AttributeType attr, int index)
{
	if (index < 0 || this->m_SubSpawners.IsEmpty()) {
		return IPopulationSpawner::HasAttribute(attr, index);
	}
	
	index %= this->m_SubSpawners.Count();
	IPopulationSpawner *spawner = this->m_SubSpawners[index];
	
	if (!spawner->IsVarious()) {
		/* likely bug: passing index thru to the sub-spawner (rather than -1) */
		return spawner->HasAttribute(attr, index);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return false;
	}
}

bool CRandomChoiceSpawner::HasAttribute(CTFBot::AttributeType attr, int index)
{
	if (index < 0) {
		return IPopulationSpawner::HasAttribute(attr, index);
	}
	
	this->GenerateRandomIndexes(index);
	
	IPopulationSpawner *spawner = this->m_SubSpawners[this->m_Indexes[index]];
	if (!spawner->IsVarious()) {
		/* likely bug: passing index thru to the sub-spawner (rather than -1) */
		return spawner->HasAttribute(attr, index);
	} else {
		DevWarning("Nested complex spawner types... need a method for counting these.");
		return false;
	}
}


bool CMobSpawner::HasEventChangeAttributes(const char *name) const
{
	if (this->m_SubSpawner != nullptr) {
		return this->m_SubSpawner->HasEventChangeAttributes(name);
	} else {
		return false;
	}
}

bool CSentryGunSpawner::HasEventChangeAttributes(const char *name) const
{
	return false;
}

bool CTankSpawner::HasEventChangeAttributes(const char *name) const
{
	return false;
}

bool CTFBotSpawner::HasEventChangeAttributes(const char *name) const
{
	FOR_EACH_VEC(this->m_ECAttrs, i) {
		const CTFBot::EventChangeAttributes_t& ecattr = this->m_ECAttrs[i];
		
		const char *ecname = ecattr.m_strName.Get();
		if (name == ecname || V_stricmp(name, ecname) == 0) {
			return true;
		}
	}
	
	return false;
}

bool CSquadSpawner::HasEventChangeAttributes(const char *name) const
{
	FOR_EACH_VEC(this->m_SubSpawners, i) {
		if (this->m_SubSpawners[i]->HasEventChangeAttributes(name)) {
			return true;
		}
	}
	
	return false;
}

bool CRandomChoiceSpawner::HasEventChangeAttributes(const char *name) const
{
	/* weird: this function still recurses over the sub-spawners, yet it always
	 * returns false regardless */
	
	FOR_EACH_VEC(this->m_SubSpawners, i) {
		(void)this->m_SubSpawners[i]->HasEventChangeAttributes(name);
	}
	
	return false;
}


IPopulationSpawner *IPopulationSpawner::ParseSpawner(IPopulator *populator, KeyValues *kv)
{
	const char *name = kv->GetName();
	if (strlen(name) <= 0) {
		return nullptr;
	}
	
	IPopulationSpawner* spawner;
	if (V_stricmp(name, "TFBot") == 0) {
		spawner = new CTFBotSpawner(populator);
		if (spawner->Parse(kv)) {
			return spawner;
		} else {
			Warning("Warning reading TFBot spawner definition\n");
		}
	} else if (V_stricmp(name, "Tank") == 0) {
		spawner = new CTankSpawner(populator);
		if (spawner->Parse(kv)) {
			return spawner;
		} else {
			Warning("Warning reading Tank spawner definition\n");
		}
	} else if (V_stricmp(name, "SentryGun") == 0) {
		spawner = new CSentryGunSpawner(populator);
		if (spawner->Parse(kv)) {
			return spawner;
		} else {
			Warning("Warning reading SentryGun spawner definition\n");
		}
	} else if (V_stricmp(name, "Squad") == 0) {
		spawner = new CSquadSpawner(populator);
		if (spawner->Parse(kv)) {
			return spawner;
		} else {
			Warning("Warning reading Squad spawner definition\n");
		}
	} else if (V_stricmp(name, "Mob") == 0) {
		spawner = new CMobSpawner(populator);
		if (spawner->Parse(kv)) {
			return spawner;
		} else {
			Warning("Warning reading Mob spawner definition\n");
		}
	} else if (V_stricmp(name, "RandomChoice") == 0) {
		spawner = new CRandomChoiceSpawner(populator);
		if (spawner->Parse(kv)) {
			return spawner;
		} else {
			Warning("Warning reading RandomChoice spawner definition\n");
		}
	}
	
	delete spawner;
	return nullptr;
}


bool CTFBotSpawner::ParseEventChangeAttributes(KeyValues *kv)
{
	if (kv == nullptr) {
		return true;
	}
	
	FOR_EACH_SUBKEY(kv, subkey) {
		const char *name = subkey->GetName();
		
		this->m_ECAttrs.AddToTail();
		CTFBot::EventChangeAttributes_t& ecattr = this->m_ECAttrs.Tail();
		ecattr.m_strName = name;
		
		FOR_EACH_SUBKEY(subkey, subsubkey) {
			if (!ParseDynamicAttributes(ecattr, subsubkey)) {
				Warning("TFBotSpawner EventChangeAttributes: Failed to parse event '%s' with unknown attribute '%s'\n",
					subkey->GetName(), subsubkey->GetName());
				return false;
			}
		}
		
		if (V_stricmp(name, "default") == 0) {
			/* use the default copy constructor */
			this->m_DefaultAttrs = ecattr;
		}
	}
	
	return true;
}


int CRandomChoiceSpawner::GenerateRandomIndexes(int index)
{
	/* ensure that m_Indexes has been random generated at least enough so that
	 * an access to m_Indexes[index] can succeed */
	
	int size_before = this->m_Indexes.Count();
	int size_wanted = (index + 1);
	
	int size_growby = size_wanted - size_before;
	if (size_growby > 0) {
		this->m_Indexes.AddMultipleToTail(size_growby);
		
		for (int i = size_before; i < size_wanted; ++i) {
			this->m_Indexes[i] = RandomInt(0, this->m_SubSpawners.Count() - 1);
		}
	}
}


// fastcall
// eax: ecattr
// edx: kv
bool ParseDynamicAttributes(CTFBot::EventChangeAttributes_t& ecattr, KeyValues *kv)
{
	const char *name = kv->GetName();
	
	if (V_stricmp(name, "Skill") == 0) {
		const char *val = kv->GetString();
		
		if (V_stricmp(val, "Easy") == 0) {
			ecattr.m_iSkill = CTFBot::DifficultyType::EASY;
		} else if (V_stricmp(val, "Normal") == 0) {
			ecattr.m_iSkill = CTFBot::DifficultyType::NORMAL;
		} else if (V_stricmp(val, "Hard") == 0) {
			ecattr.m_iSkill = CTFBot::DifficultyType::HARD;
		} else if (V_stricmp(val, "Expert") == 0) {
			ecattr.m_iSkill = CTFBot::DifficultyType::EXPERT;
		} else {
			Warning("TFBotSpawner: Invalid skill '%s'\n", val);
			return false;
		}
		
		return true;
	}
	
	if (V_stricmp(name, "WeaponRestrictions") == 0) {
		const char *val = kv->GetString();
		
		if (V_stricmp(val, "MeleeOnly") == 0) {
			ecattr.m_nWeaponRestrict = CTFBot::WeaponRestriction::MELEEONLY;
		} else if (V_stricmp(val, "PrimaryOnly") == 0) {
			ecattr.m_nWeaponRestrict = CTFBot::WeaponRestriction::PRIMARYONLY;
		} else if (V_stricmp(val, "SecondaryOnly") == 0) {
			ecattr.m_nWeaponRestrict = CTFBot::WeaponRestriction::SECONDARYONLY;
		} else {
			Warning("TFBotSpawner: Invalid weapon restriction '%s'\n", val);
			return false;
		}
		
		return true;
	}
	
	if (V_stricmp(name, "BehaviorModifiers") == 0) {
		const char *val = kv->GetString();
		
		if (V_stricmp(val, "Mobber") == 0 ||
			V_stricmp(val, "Push") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::AGGRESSIVE;
		} else {
			Warning("TFBotSpawner: invalid behavior modifier '%s'\n", val);
			return false;
		}
		
		return true;
	}
	
	if (V_stricmp(name, "MaxVisionRange") == 0) {
		ecattr.m_flVisionRange = kv->GetFloat();
		return true;
	}
	
	if (V_stricmp(name, "Item") == 0) {
		ecattr.m_ItemNames.CopyAndAddToTail(kv->GetString());
		return true;
	}
	
	if (V_stricmp(name, "Tag") == 0) {
		ecattr.m_Tags.CopyAndAddToTail(kv->GetString());
		return true;
	}
	
	if (V_stricmp(name, "Attributes") == 0) {
		const char *val = kv->GetString();
		
		if (V_stricmp(val, "RemoveOnDeath") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::REMOVEONDEATH;
		} else if (V_stricmp(val, "Aggressive") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::AGGRESSIVE;
		} else if (V_stricmp(val, "SuppressFire") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::SUPPRESSFIRE;
		} else if (V_stricmp(val, "DisableDodge") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::DISABLEDODGE;
		} else if (V_stricmp(val, "BecomeSpectatorOnDeath") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::BECOMESPECTATORONDEATH;
		} else if (V_stricmp(val, "RetainBuildings") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::RETAINBUILDINGS;
		} else if (V_stricmp(val, "SpawnWithFullCharge") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::SPAWNWITHFULLCHARGE;
		} else if (V_stricmp(val, "AlwaysCrit") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::ALWAYSCRIT;
		} else if (V_stricmp(val, "IgnoreEnemies") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::IGNOREENEMIES;
		} else if (V_stricmp(val, "HoldFireUntilFullReload") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::HOLDFIREUNTILFULLRELOAD;
		} else if (V_stricmp(val, "AlwaysFireWeapon") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::ALWAYSFIREWEAPON;
		} else if (V_stricmp(val, "TeleportToHint") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::TELEPORTTOHINT;
		} else if (V_stricmp(val, "MiniBoss") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::MINIBOSS;
		} else if (V_stricmp(val, "UseBossHealthBar") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::USEBOSSHEALTHBAR;
		} else if (V_stricmp(val, "IgnoreFlag") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::IGNOREFLAG;
		} else if (V_stricmp(val, "AutoJump") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::AUTOJUMP;
		} else if (V_stricmp(val, "AirChargeOnly") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::AIRCHARGEONLY;
		} else if (V_stricmp(val, "VaccinatorBullets") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::VACCINATORBULLETS;
		} else if (V_stricmp(val, "VaccinatorBlast") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::VACCINATORBLAST;
		} else if (V_stricmp(val, "VaccinatorFire") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::VACCINATORFIRE;
		} else if (V_stricmp(val, "BulletImmune") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::BULLETIMMUNE;
		} else if (V_stricmp(val, "BlastImmune") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::BLASTIMMUNE;
		} else if (V_stricmp(val, "FireImmune") == 0) {
			ecattr.m_nBotAttrs |= CTFBot::AttributeType::FIREIMMUNE;
		} else {
			Warning("TFBotSpawner: Invalid attribute '%s'\n", val);
			return false;
		}
		
		return true;
	}
	
	if (V_stricmp(name, "CharacterAttributes") == 0) {
		CUtlVector<static_attrib_t> attrs;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			static_attrib_t attr;
			CUtlVector<CUtlString> errors;
			
			if (attr.BInitFromKV_SingleLine("CharacterAttributes",
				subkey, &errors, true)) {
				attrs.AddToHead(attr);
			} else {
				FOR_EACH_VEC(errors, i) {
					Warning("TFBotSpawner: attribute error: '%s'\n",
						errors[i].Get());
				}
			}
		}
		
		FOR_EACH_VEC(attrs, i) {
			static_attrib_t& attr_new = attrs[i];
			
			/* if the same character attribute is specified again, find the
			 * existing entry and overwrite the value */
			bool found_old = false;
			FOR_EACH_VEC(ecattr.m_CharAttrs, j) {
				static_attrib_t& attr_old = ecattr.m_CharAttrs[j];
				
				if (attr_new.m_iAttrIndex == attr_old.m_iAttrIndex) {
					attr_old.m_Value = attr_new.m_Value;
					
					found_old = true;
					break;
				}
			}
			
			if (!found_old) {
				ecattr.m_CharAttrs.AddToHead(attr_new);
			}
		}
		
		return true;
	}
	
	if (V_stricmp(name, "ItemAttributes") == 0) {
		const char *item_name = nullptr;
		CUtlVector<static_attrib_t> attrs;
		
		FOR_EACH_SUBKEY(kv, subkey) {
			if (V_stricmp(subkey->GetName(), "ItemName") == 0) {
				if (item_name == nullptr) {
					item_name = subkey->GetString();
				} else {
					Warning("TFBotSpawner: \"ItemName\" field specified multiple times ('%s' / '%s').\n",
						item_name, subkey->GetString());
				}
			} else {
				static_attrib_t attr;
				CUtlVector<CUtlString> errors;
				
				if (attr.BInitFromKV_SingleLine("ItemAttributes",
					subkey, &errors, true)) {
					attrs.AddToHead(attr);
				} else {
					FOR_EACH_VEC(errors, i) {
						Warning("TFBotSpawner: attribute error: '%s'\n",
							errors[i].Get());
					}
				}
			}
		}
		
		if (item_name == nullptr) {
			Warning("TFBotSpawner: need to specify ItemName in ItemAttributes.\n");
		} else {
			FOR_EACH_VEC(attrs, i) {
				static_attrib_t& attr_new = attrs[i];
				
				/* check if we already have an attr list for this item */
				bool found_item_old = false;
				FOR_EACH_VEC(ecattr.m_ItemAttrs, j) {
					CTFBot::EventChangeAttributes_t::item_attributes_t& item_old = ecattr.m_ItemAttrs[j];
					
					if (V_stricmp(item_old.strItemName, item_name) == 0) {
						/* check if we already have an attr list entry for this
						 * attribute on this item */
						bool found_attr_old = false;
						FOR_EACH_VEC(item_old.m_Attrs, k) {
							static_attrib_t& attr_old = item_old.m_Attrs[k];
							
							if (attr_new.m_iAttrIndex == attr_old.m_iAttrIndex) {
								attr_old.m_Value = attr_new.m_Value;
								
								found_attr_old = true;
								break;
							}
						}
						
						if (!found_attr_old) {
							item_old.m_Attrs.AddToHead(attr_new);
						}
						
						found_item_old = true;
						break;
					}
				}
				
				if (!found_item_old) {
					CTFBot::EventChangeAttributes_t::item_attributes_t item_new;
					item_new.m_strItemName = item_name;
					item_new.m_Attrs.AddToHead(attr_new);
					
					ecattr.m_ItemAttrs.AddToTail(item_new);
				}
			}
		}
		
		return true;
	}
	
	return false;
}
