/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/bot/tf_bot_manager.cpp
 */


ConVar tf_bot_difficulty("tf_bot_difficulty", "1", FCVAR_NONE,
	"Defines the skill of bots joining the game.  Values are: 0=easy, 1=normal, 2=hard, 3=expert.");
ConVar tf_bot_quota("tf_bot_quota", "0", FCVAR_NONE,
	"Determines the total number of tf bots in the game.");
ConVar tf_bot_quota_mode("tf_bot_quota_mode", "normal", FCVAR_NONE,
	"Determines the type of quota. Allowed values: 'normal', 'fill', and 'match'. If 'fill', the server will adjust bots to keep N players in the game, where N is bot_quota. If 'match', the server will maintain a 1:N ratio of humans to bots, where N is bot_quota.");
ConVar tf_bot_join_after_player("tf_bot_join_after_player", "1", FCVAR_NONE,
	"If nonzero, bots wait until a player joins before entering the game.");
ConVar tf_bot_auto_vacate("tf_bot_auto_vacate", "1", FCVAR_NONE,
	"If nonzero, bots will automatically leave to make room for human players.");
ConVar tf_bot_offline_practice("tf_bot_offline_practice", "0", FCVAR_NONE,
	"Tells the server that it is in offline practice mode.");
ConVar tf_bot_melee_only("tf_bot_melee_only", "0", FCVAR_GAMEDLL,
	"If nonzero, TFBots will only use melee weapons");

ConCommand tf_bot_debug_log_command("tf_bot_debug_stuck_log", &tf_bot_debug_stuck_log,
	"Given a server logfile, visually display bot stuck locations.", FCVAR_CHEAT | FCVAR_GAMEDLL);
ConCommand tf_bot_debug_log_clear_command("tf_bot_debug_stuck_log_clear", &tf_bot_stuck_log_clear,
	"Clear currently loaded bot stuck data", FCVAR_CHEAT | FCVAR_GAMEDLL);


CTFBotManager sTFBotManager;


CTFBotManager *TheTFBots()
{
	/* uh... yeah... this isn't valid at all, since the pointer returned by
	 * TheNextBots() is not actually a CTFBotManager!
	 * (why the hell doesn't this return sTFBotManager, which is a valid
	 * CTFBotManager that is completely unused?) */
	return static_cast<CTFBotManager *>(TheNextBots());
}


void CTFBotManager::MaintainBotQuota()
{
	if (TheNavMesh->IsGenerating()) return;
	if (TFGameRules() != nullptr && TFGameRules()->IsInTraining()) return;
	
	if (gpGlobals->curtime < this->m_flQuotaTime) return;
	this->m_flQuotaTime = gpGlobals->curtime + 0.25f;
	
	if (!engine->IsDedicatedServer() && UTIL_GetListenServerHost() == nullptr) return;
	
	int n_players_total = 0;
	int n_tfbots_total  = 0;
	int n_tfbots_redblu = 0;
	int n_humans_redblu = 0; // n_humans[0]
	int n_humans_spec   = 0; // n_humans[1]
	
	for (int i = 1; i < gpGlobals->maxClients; ++i) {
		CBasePlayer *player = UTIL_PlayerByIndex(i);
		
		if (player == nullptr)     continue;
		if (!player->IsPlayer())   continue;
		if (ENTINDEX(player) == 0) continue;
		// TODO: another continue case
		
		auto bot = dynamic_cast<CTFBot *>(player);
		// TODO: name for bit 6 of CTFBot::AttributeType
		if (bot != nullptr && (bot->m_nBotAttrs & 0x40) != 0) {
			++n_tfbots_total;
			if (bot->GetTeamNumber() == TF_TEAM_RED || bot->GetTeamNumber() == TF_TEAM_BLUE) {
				++n_tfbots_redblu;
			}
		} else {
			if (player->GetTeamNumber() == TF_TEAM_RED || player->GetTeamNumber() == TF_TEAM_BLUE) {
				++n_humans_redblu;
			} else if (player->GetTeamNumber() == TEAM_SPECTATOR) {
				++n_humans_spec;
			}
		}
		
		++n_players_total;
	}
	
	int n_humans_total = n_players_total - n_tfbots_total;
	
	int n_desired = tf_bot_quota.GetInt();
	
	if (FStrEq(tf_bot_quota_mode.GetString(), "fill")) {
		n_desired = Max(0, n_desired - n_humans_redblu);
	} else if (FStrEq(tf_bot_quota_mode.GetString(), "match")) {
		n_desired = (int)Max(0.0f, tf_bot_quota.GetFloat() * n_humans_redblu);
	}
	
	if (tf_bot_join_after_player.GetBool()) {
		if (n_humans_redblu == 0) {
			n_desired = 0;
		}
	}
	
	if (tf_bot_auto_vacate.GetBool()) {
		n_desired = Min(n_desired, gpGlobals->maxClients - (n_humans_total + 1));
	} else {
		n_desired = Min(n_desired, gpGlobals->maxClients - n_humans_total);
	}
	
	if (n_desired > n_tfbots_redblu) {
		if (!TFGameRules()->WouldChangeUnbalanceTeams(TF_TEAM_BLUE, TEAM_UNASSIGNED) &&
			!TFGameRules()->WouldChangeUnbalanceTeams(TF_TEAM_RED, TEAM_UNASSIGNED)) {
			CTFBot *bot = this->GetAvailableBotFromPool();
			if (bot == nullptr) {
				bot = NextBotCreatePlayerBot<CTFBot>(GetRandomBotName(), true);
			}
			
			if (bot != nullptr) {
				// TODO: name for bit 6 of CTFBot::AttributeType
				bot->m_nBotAttrs |= 0x40;
				
				bot->HandleCommand_JoinTeam("auto");
				
				const char *classname;
				if (FStrEq(tf_bot_force_class.GetString(), "")) {
					classname = bot->GetNextSpawnClassname();
				} else {
					classname = tf_bot_force_class.GetString();
				}
				
				bot->HandleCommand_JoinClass(classname, true);
				
				char name[0x100];
				CreateBotName(bot->GetTeamNumber(), bot->GetPlayerClass()->GetClassIndex(), bot->m_iSkill, name, sizeof(name));
				engine->SetFakeClientConVarValue(ENTINDEX(bot), "name", name);
			}
		}
	} else if (n_desired < n_tfbots_redblu) {
		if (!UTIL_KickBotFromTeam(TEAM_UNASSIGNED)) {
			CTeam *team_red = GetGlobalTeam(TF_TEAM_RED);
			CTeam *team_blu = GetGlobalTeam(TF_TEAM_BLUE);
			
			/* there's probably some sort functor action going on here */
			int team_kick = TF_TEAM_BLUE;
			if (team_red->GetNumPlayers() >= team_blu->GetNumPlayers()) {
				team_kick = TF_TEAM_RED;
				if (team_red->GetNumPlayers() <= team_blu->GetNumPlayers()) {
					team_kick = TF_TEAM_BLUE;
					if (team_red->GetScore() >= team_blu->GetScore()) {
						team_kick = TF_TEAM_RED;
						if (team_red->GetScore() <= team_blu->GetScore()) {
							team_kick = RandomInt(0, 1) == 1 ? TF_TEAM_RED : TF_TEAM_BLUE;
						}
					}
				}
			}
			
			if (!UTIL_KickBotFromTeam(team_kick)) {
				UTIL_KickBotFromTeam(team_kick == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED);
			}
		}
	}
}
