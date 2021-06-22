/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_vision.cpp
 */


ConVar tf_bot_choose_target_interval("tf_bot_choose_target_interval", "0.3f", FCVAR_CHEAT,
	"How often, in seconds, a TFBot can reselect his target");
ConVar tf_bot_sniper_choose_target_interval("tf_bot_sniper_choose_target_interval", "3.0f", FCVAR_CHEAT,
	"How often, in seconds, a zoomed-in Sniper can reselect his target");


CTFBotVision::CTFBotVision(INextBot *nextbot)
{
}

CTFBotVision::~CTFBotVision()
{
}


void CTFBotVision::Update()
{
	if (TFGameRules()->IsMannVsMachineMode()) {
		if (!this->m_ctUpdate.IsElapsed()) {
			return;
		}
		
		this->m_ctUpdate.Start(RandomFloat(0.9f, 1.1f));
	}
	
	IVision::Update();
	
	CTFBot *actor = static_cast<CTFBot *>(this->GetBot()->GetEntity());
	if (actor == nullptr) {
		return;
	}
	
	CUtlVector<CTFPlayer *> enemies;
	CollectPlayers<CTFPlayer>(enemies, GetEnemyTeam(actor), true, false);
	
	FOR_EACH_VEC(enemies, i) {
		CTFPlayer *enemy = enemies[i];
		
		if (enemy->IsPlayerClass(TF_CLASS_SPY)) {
			const CKnownEntity *known = this->GetKnown(enemy);
			
			if (known != nullptr && (known->IsVisibleRecently() ||
				!player->m_Shared.InCond(TF_COND_DISGUISING))) {
				actor->ForgetSpy(enemy);
			}
		}
	}
}


void CTFBotVision::CollectPotentiallyVisibleEntities(CUtlVector<CBaseEntity *> *ents)
{
	VPROF_BUDGET("CTFBotVision::CollectPotentiallyVisibleEntities", "NextBot");
	
	ents->RemoveAll();
	
	for (int i = 0; i < gpGlobals->maxClients; ++i) {
		CBaseEntity *client = UTIL_PlayerByIndex(i);
		if (client == nullptr) {
			continue;
		}
		
		if (ENTINDEX(client) != 0 && client->IsPlayer() &&
			client->IsConnected() && client->IsAlive()) {
			ents->AddToTail(client);
		}
	}
	
	this->UpdatePotentiallyVisibleNPCVector();
	FOR_EACH_VEC(this->m_PVNPCs, i) {
		CBaseEntity *npc = this->m_PVNPCs[i]();
		ents->AddToTail(npc);
	}
}

float CTFBotVision::GetMaxVisionRange() const
{
	CTFBot *actor = static_cast<CTFBot *>(this->GetBot());
	
	if (actor->m_flVisionRange > 0.0f) {
		return actor->m_flVisionRange;
	} else {
		return 6000.0f;
	}
}

float CTFBotVision::GetMinRecognizeTime() const
{
	CTFBot *actor = static_cast<CTFBot *>(this->GetBot());
	
	switch (actor->m_iSkill) {
	default:
		return 1.00f;
	case CTFBot::DifficultyType::NORMAL:
		return 0.50f;
	case CTFBot::DifficultyType::HARD:
		return 0.30f;
	case CTFBot::DifficultyType::EXPERT:
		return 0.20f;
	}
}

bool CTFBotVision::IsIgnored(CBaseEntity *ent) const
{
	CTFBot *actor = static_cast<CTFBot *>(this->GetBot()->GetEntity());
	
	/* this is irrelevant CTFBotActionPoint stuff, which is only really used by
	 * CTFBotGenerator (for training mode or something) */
	if (actor->IsAttentionFocused() && !actor->IsAttentionFocusedOn(ent)) {
		return false;
	}
	
	/* players of the same team are never ignored */
	if (!actor->IsEnemy(ent)) {
		return false;
	}
	
	if ((ent->GetEffects() & EF_NODRAW) != 0) {
		return true;
	}
	
	CTFPlayer *player = ToTFPlayer(ent);
	if (player != nullptr) {
		switch (player->GetPlayerClass()->GetClassIndex()) {
		case TF_CLASS_SCOUT:
			if ((actor->m_nIgnoreMask & IGNORE_SCOUTS) != 0) {
				return true;
			}
			break;
		case TF_CLASS_SNIPER:
			if ((actor->m_nIgnoreMask & IGNORE_SNIPERS) != 0) {
				return true;
			}
			break;
		case TF_CLASS_SOLDIER:
			if ((actor->m_nIgnoreMask & IGNORE_SOLDIERS) != 0) {
				return true;
			}
			break;
		case TF_CLASS_DEMOMAN:
			if ((actor->m_nIgnoreMask & IGNORE_DEMOS) != 0) {
				return true;
			}
			break;
		case TF_CLASS_MEDIC:
			if ((actor->m_nIgnoreMask & IGNORE_MEDICS) != 0) {
				return true;
			}
			break;
		case TF_CLASS_HEAVYWEAPONS:
			if ((actor->m_nIgnoreMask & IGNORE_HEAVYS) != 0) {
				return true;
			}
			break;
		case TF_CLASS_PYRO:
			if ((actor->m_nIgnoreMask & IGNORE_PYROS) != 0) {
				return true;
			}
			break;
		case TF_CLASS_SPY:
			if ((actor->m_nIgnoreMask & IGNORE_SPIES) != 0) {
				return true;
			}
			break;
		case TF_CLASS_ENGINEER:
			if ((actor->m_nIgnoreMask & IGNORE_ENGIES) != 0) {
				return true;
			}
			break;
		}
		
		/* known spies are never ignored */
		if (actor->IsKnownSpy(player)) {
			return false;
		}
		
		/* players who are burning/jarated/bleeding, or who are cloaked and
		 * bump into something, are not ignored */
		// BUG: bots ignore milked and wet players
		if (player->m_Shared.InCond(TF_COND_BURNING) ||
			player->m_Shared.InCond(TF_COND_URINE) ||
			player->m_Shared.InCond(TF_COND_STEALTHED_BLINK) ||
			player->m_Shared.InCond(TF_COND_BLEEDING)) {
			return false;
		}
		
		/* condition 66, used for Halloween and not in MvM (may have been
		 * intended for use as a canteen, based on speculation) */
		if (player->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF_FADING)) {
			return true;
		}
		
		/* spies are only ignored when more than 75% cloaked */
		if (player->m_Shared.IsStealthed()) {
			return (player->m_Shared.GetPercentInvisible() >= 0.75f);
		}
		
		/* spies who are not fully disguised are not ignored */
		if (!player->m_Shared.InCond(TF_COND_DISGUISED) ||
			player->m_Shared.InCond(TF_COND_DISGUISING)) {
			return false;
		}
		
	 	/* every time a disguised spy saps anything (building or player),
	 	 * CTFWeaponBuilder::PrimaryAttack starts a CountdownTimer (which I have
	 	 * named CTFPlayer::m_ctSapNotice) with a 3.0 second duration; for that
	 	 * period of time, bots will not ignore you, despite your disguise */
	 	if (!player->m_ctSapNotice.IsElapsed()) {
			return false;
		}
		
		/* spies fully disguised as the bot's team are ignored */
		return (player->m_Shared.GetDisguiseTeam() == actor->GetTeamNumber());
	} else if (ent->IsBaseObject()) {
		CBaseObject *obj = static_cast<CBaseObject *>(ent);
		
		/* outside of MvM mode, sapped buildings are ignored */
		if (obj->HasSapper()) {
			if (TFGameRules() == nullptr) {
				return true;
			}
			
			return !TFGameRules->IsMannVsMachineMode();
		}
		
		/* buildings are ignored when being placed/carried */
		if (obj->m_bPlacing || obj->m_bIsCarried) {
			return true;
		}
		
		if (obj->GetType() == OBJ_SENTRYGUN &&
			(actor->m_nIgnoreMask & IGNORE_SENTRIES) != 0) {
			return true;
		}
		
		return false;
	}
	

}

bool CTFBotVision::IsVisibleEntityNoticed(CBaseEntity *ent) const
{
	CTFBot *actor = static_cast<CTFBot *>(this->GetBot()->GetEntity());
	
	CTFPlayer *player = ToTFPlayer(ent);
	if (player == nullptr) {
		return true;
	}
	
	/* players of the same team are always noticed */
	if (!actor->IsEnemy()) {
		return true;
	}
	
	/* players who are burning/jarated/bleeding, or who are cloaked and
	 * bump into something, are always noticed */
	// BUG: bots don't notice milked or wet players
	if (player->m_Shared.InCond(TF_COND_BURNING) ||
		player->m_Shared.InCond(TF_COND_URINE) ||
		player->m_Shared.InCond(TF_COND_STEALTHED_BLINK) ||
		player->m_Shared.InCond(TF_COND_BLEEDING)) {
		/* if cloaked while in one of these conditions, bots will immediately
		 * realize that the player is a spy */
		if (player->m_Shared.InCond(TF_COND_STEALTHED)) {
			actor->RealizeSpy(player);
		}
		
		return true;
	}
	
	/* condition 66, used for Halloween and not in MvM (may have been intended
	 * for use as a canteen, based on speculation) */
	if (player->m_Shared.InCond(TF_COND_STEALTHED_USER_BUFF_FADING)) {
		actor->ForgetSpy(player);
		return false;
	}
	
	if (player->m_Shared.IsStealthed()) {
		if (player->m_Shared.GetPercentInvisible() < 0.75f) {
			/* bots will notice, and immediately realize, spies that are less
			 * than 75% cloaked */
			actor->RealizeSpy(player);
			return true;
		} else {
			/* bots will not notice, and will immediately forget, spies that are
			 * more than 75% cloaked */
			actor->ForgetSpy(player);
			return false;
		}
	}
	
	/* the moment a spy becomes fully disguised, bots will stop noticing them
	 * forget them */
	if (TFGameRules()->IsMannVsMachineMode()) {
		CTFBot::SuspectedSpyInfo_t *suspected = actor->IsSuspectedSpy(player);
		if (suspected == nullptr || (!suspected->IsCurrentlySuspected() &&
			player->m_Shared.InCond(TF_COND_DISGUISED))) {
			if (player->m_Shared.GetDisguiseTeam() == actor->GetTeamNumber()) {
				actor->ForgetSpy(player);
				return false;
			}
		}
	}
	
	/* known spies are always noticed */
	if (actor->IsKnownSpy(player)) {
		return true;
	}
	
	/* outside of MvM mode, for 3.0 seconds after sapping anything, bots will
	 * notice spies and realize that they are a spy */
	if (!TFGameRules()->IsMannVsMachineMode() &&
		!player->m_ctSapNotice.IsElapsed()) {
		actor->RealizeSpy(player);
		return true;
	}
	
	/* bots will notice and realize spies who are partially disguised */
	if (player->m_Shared.InCond(TF_COND_DISGUISING)) {
		actor->RealizeSpy(player);
		return true;
	}
	
	/* bots will not notice spies disguised as their own team */
	if (player->m_Shared.InCond(TF_COND_DISGUISED)) {
		return (player->m_Shared.GetDisguiseTeam() != actor->GetTeamNumber());
	}
	
	return true;
}


void CTFBotVision::UpdatePotentiallyVisibleNPCVector()
{
	if (!this->m_ctUpdatePVNPCs.IsElapsed()) {
		return;
	}
	this->m_ctUpdatePVNPCs.Start(RandomFloat(3.0f, 4.0f));
	
	this->m_PVNPCs.RemoveAll();
	
	bool ignore_teleporters = false;
	if (TFGameRules()->IsMannVsMachineMode()) {
		CBaseCombatCharacter *actor = this->GetBot()->GetEntity();
		ignore_teleporters = (actor->GetTeamNumber() == TF_TEAM_BLUE);
	}
	
	for (int i = 0; i < IBaseObjectAutoList::AutoList().Count(); ++i) {
		CBaseObject *obj = static_cast<CBaseObject *>(IBaseObjectAutoList::AutoList()[i]);
		
		if (obj->GetType() == OBJ_SENTRYGUN ||
			(obj->GetType() == OBJ_DISPENSER && obj->ClassMatches("obj_dispenser") ||
			(obj->GetType() == OBJ_TELEPORTER && !ignore_teleporters)) {
			this->m_PVNPCs.AddToTail(obj);
		}
	}
	
	CUtlVector<INextBot *> nextbots;
	TheNextBots().CollectAllBots(&nextbots);
	
	FOR_EACH_VEC(nextbots, i) {
		CBaseCombatCharacter *ent = nextbots[i]->GetEntity();
		if (ent != nullptr && !ent->IsPlayer()) {
			this->m_PVNPCs.AddToTail(ent);
		}
	}
}
