// convars (and other static/global variables and file-level stuff)
// TODO: determine which convars are accessed and which are dormant/unused


// shared/tf/tf_gamerules.cpp
// NOTE: skipped many unrelated convars here
ConVar tf_halloween_boss_spawn_interval("tf_halloween_boss_spawn_interval", "480", FCVAR_CHEAT, "Average interval between boss spawns, in seconds");
ConVar tf_halloween_boss_spawn_interval_variation("tf_halloween_boss_spawn_interval_variation", "60", FCVAR_CHEAT, "Variation of spawn interval +/-");
ConVar tf_halloween_eyeball_boss_spawn_interval("tf_halloween_eyeball_boss_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds");
ConVar tf_halloween_eyeball_boss_spawn_interval_variation("tf_halloween_eyeball_boss_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-");
ConVar tf_merasmus_spawn_interval("tf_merasmus_spawn_interval", "180", FCVAR_CHEAT, "Average interval between boss spawns, in seconds");
ConVar tf_merasmus_spawn_interval_variation("tf_merasmus_spawn_interval_variation", "30", FCVAR_CHEAT, "Variation of spawn interval +/-");
ConVar tf_halloween_zombie_mob_enabled("tf_halloween_zombie_mob_enabled", "0", FCVAR_CHEAT, "If set to 1, spawn zombie mobs on non-Halloween Valve maps");
ConVar tf_halloween_zombie_mob_spawn_interval("tf_halloween_zombie_mob_spawn_interval", "180", FCVAR_CHEAT, "Average interval between zombie mob spawns, in seconds");
ConVar tf_halloween_zombie_mob_spawn_count("tf_halloween_zombie_mob_spawn_count", "20", FCVAR_CHEAT, "How many zombies to spaw");
ConVar tf_forced_holiday("tf_forced_holiday", "0", FCVAR_REPLICATED, "Forced holiday, \n   Birthday = 1\n   Halloween = 2\n");
ConVar tf_item_based_forced_holiday("tf_item_based_forced_holiday", "0", FCVAR_REPLICATED);
ConVar tf_force_holidays_off("tf_force_holidays_off", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar tf_birthday("tf_birthday", "0", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_tournament_redteamname("mp_tournament_redteamname", "RED", FCVAR_REPLICATED | FCVAR_HIDDEN);
ConVar mp_tournament_blueteamname("mp_tournament_blueteamname", "BLU", FCVAR_REPLICATED | FCVAR_HIDDEN);
ConVar mp_tournament_stopwatch("mp_tournament_stopwatch", "1", FCVAR_REPLICATED | FCVAR_NOTIFY);
ConVar mp_tournament_readymode("mp_tournament_readymode", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable per-player ready status for tournament mode.");
ConVar mp_tournament_readymode_min("mp_tournament_readymode_min", "2", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required on the server before players can toggle ready status.");
ConVar mp_tournament_readymode_team_size("mp_tournament_readymode_team_size", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players required to be ready per-team before the game can begin.");
ConVar mp_tournament_readymode_countdown("mp_tournament_readymode_countdown", "10", FCVAR_REPLICATED | FCVAR_NOTIFY, "The number of seconds before a match begins when both teams are ready.");
ConVar tf_tournament_classlimit_scout("tf_tournament_classlimit_scout", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Scouts.\n");
ConVar tf_tournament_classlimit_sniper("tf_tournament_classlimit_sniper", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Snipers.\n");
ConVar tf_tournament_classlimit_soldier("tf_tournament_classlimit_soldier", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Soldiers.\n");
ConVar tf_tournament_classlimit_demoman("tf_tournament_classlimit_demoman", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Demomenz.\n");
ConVar tf_tournament_classlimit_medic("tf_tournament_classlimit_medic", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Medics.\n");
ConVar tf_tournament_classlimit_heavy("tf_tournament_classlimit_heavy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Heavies.\n");
ConVar tf_tournament_classlimit_pyro("tf_tournament_classlimit_pyro", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Pyros.\n");
ConVar tf_tournament_classlimit_spy("tf_tournament_classlimit_spy", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Spies.\n");
ConVar tf_tournament_classlimit_engineer("tf_tournament_classlimit_engineer", "-1", FCVAR_REPLICATED, "Tournament mode per-team class limit for Engineers.\n");
ConVar tf_tournament_classchange_allowed("tf_tournament_classchange_allowed", "1", FCVAR_REPLICATED, "Allow players to change class while the game is active?.");
ConVar tf_tournament_classchange_ready_allowed("tf_tournament_classchange_ready_allowed", "1", FCVAR_REPLICATED, "Allow players to change class after they are READY?.");
ConVar mp_humans_must_join_team("mp_humans_must_join_team", "any", FCVAR_REPLICATED, "Restricts human players to a single team {any, blue, red, spectator}");
ConVar tf_medieval("tf_medieval", "0", FCVAR_REPLICATED | FCVAR_NOTIFY, "Enable Medieval Mode.\n", true, 0.0f, true, 1.0f, &cc_tf_medieval_changed);
ConVar tf_mvm_min_players_to_start("tf_mvm_min_players_to_start", "3", FCVAR_REPLICATED | FCVAR_NOTIFY, "Minimum number of players connected to start a countdown timer");
ConVar tf_mvm_respec_enabled("tf_mvm_respec_enabled", "1", FCVAR_CHEAT | FCVAR_REPLICATED, "Allow players to refund credits spent on player and item upgrades.");
ConVar tf_mvm_respec_limit("tf_mvm_respec_limit", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "The total number of respecs a player can earn.  Default: 0 (no limit).");
ConVar tf_mvm_respec_credit_goal("tf_mvm_respec_credit_goal", "2000", FCVAR_CHEAT | FCVAR_REPLICATED, "When tf_mvm_respec_limit is non-zero, the total amount of money the team must collect to earn a respec credit.");
ConVar tf_grapplinghook_enable("tf_grapplinghook_enable", "0", FCVAR_REPLICATED);
ConCommand tf_mvm_popfile_command("tf_mvm_popfile", &tf_mvm_popfile, "Change to a target popfile for MvM", FCVAR_GAMEDLL);
ConVar tf_gamemode_mvm("tf_gamemode_mvm", "0", FCVAR_REPLICATED | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar tf_bot_count("tf_bot_count", "0", FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY);
ConVar tf_powerup_mode("tf_powerup_mode", "0", FCVAR_NOTIFY, "Enable/disable powerup mode. Not compatible with Mann Vs Machine mode", true, 0.0f, true, 1.0f, &cc_powerup_mode);
CAutoGameSystem g_TFGameModeHistory(nullptr);
// TODO: server class init for CTFGameRules
// TODO: server class init for CTFGameRulesProxy ("tf_gamerules")
ConCommand tf_halloween_force_boss_spawn_command
ConCommand tcc_spawn_merasmus_at_level_command

// shared/tf/tf_playerclass_shared.cpp
ConVar tf_mvm_bot_flag_carrier_movement_penalty("tf_mvm_bot_flag_carrier_movement_penalty", "0.5", FCVAR_CHEAT | FCVAR_REPLICATED);

// server/basecombatcharacter.cpp
ConVar nb_last_area_update_tolerance("nb_last_area_update_tolerance", "4.0", FCVAR_CHEAT, "Distance a character needs to travel in order to invalidate cached area");

// server/tf/tf_obj_sapper.cpp
ConVar tf_mvm_notice_sapped_squadmates_delay("tf_mvm_notice_sapped_squadmates_delay", "1", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "How long it takes for a squad leader to notice his squadmate was sapped");

// server/tf/tf_player.cpp
ConVar tf_deploying_bomb_time("tf_deploying_bomb_time", "1.90", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to deploy bomb before the point of no return.");
ConVar tf_deploying_bomb_delay_time("tf_deploying_bomb_delay_time", "0.0", FCVAR_CHEAT | FCVAR_DEVELOPMENTONLY, "Time to delay before deploying bomb.");
ConVar tf_mvm_death_penalty("tf_mvm_death_penalty", "0", FCVAR_CHEAT | FCVAR_NOTIFY | FCVAR_DEVELOPMENTONLY, "How much currency players lose when dying");
ConVar tf_nav_in_combat_range("tf_nav_in_combat_range", "1000", FCVAR_CHEAT);
ConCommand currency_give_command("currency_give", &currency_give, "Have some in-game money.", FCVAR_CHEAT);

// client/NextBot/C_NextBot.cpp
ConVar nb_shadow_dist("nb_shadow_dist", "400", FCVAR_NONE);
// TODO: client class init for NextBotCombatCharacter

// client/tf/player_vs_environment/c_tf_upgrades.cpp
ConVar tf_mvm_tabs_discovered("tf_mvm_tabs_discovered", "0", FCVAR_ARCHIVE, "Remember how many times players have clicked tabs.");

// client/tf/vgui/tf_classmenu.cpp
ConVar tf_mvm_classupgradehelpcount("tf_mvm_classupgradehelpcount", "0", FCVAR_DONTRECORD | FCVAR_ARCHIVE | FCVAR_HIDDEN | FCVAR_CLIENTDLL, "The number of times the player upgrade help dialog has displayed.");
