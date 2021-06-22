/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBot.cpp
 */


ConVar NextBotStop("nb_stop", "0", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Stop all NextBots");

ConCommand nb_command_command("nb_command", &nb_command,
	"Sends a command string to all bots", FCVAR_CHEAT);
ConCommand nb_delete_all_command("nb_delete_all", &nb_delete_all,
	"Delete all non-player NextBot entities.", FCVAR_CHEAT);
ConCommand nb_move_to_cursor_command("nb_move_to_cursor", &nb_move_to_cursor,
	"Tell all NextBots to move to the cursor position", FCVAR_CHEAT);


// TODO: server class init for NextBotCombatCharacter



