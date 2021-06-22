/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotManager.cpp
 */


ConVar nb_update_frequency("nb_update_frequency", ".1", FCVAR_CHEAT);
ConVar nb_update_framelimit("nb_update_framelimit", "15", FCVAR_CHEAT);
ConVar nb_update_maxslide("nb_update_maxslide", "2", FCVAR_CHEAT);
ConVar nb_update_debug("nb_update_debug", "0", FCVAR_CHEAT);

ConCommand SetDebug("nb_debug", &CC_SetDebug,
	"Debug NextBots.  Categories are: BEHAVIOR, LOOK_AT, PATH, ANIMATION, LOCOMOTION, VISION, HEARING, EVENTS, ERRORS.", FCVAR_CHEAT);
ConCommand SetDebugFilter("nb_debug_filter", &CC_SetDebugFilter,
	"Add items to the NextBot debug filter. Items can be entindexes or part of the indentifier of one or more bots.", FCVAR_CHEAT);
ConCommand SelectBot("nb_select", &CC_SelectBot,
	"Select the bot you are aiming at for further debug operations.", FCVAR_CHEAT);
ConCommand ForceLookAt("nb_force_look_at", &CC_ForceLookAt,
	"Force selected bot to look at the local player's position", FCVAR_CHEAT);
ConCommand WarpSelectedHere("nb_warp_selected_here", &CC_WarpSelectedHere,
	"Teleport the selected bot to your cursor position", FCVAR_CHEAT);


static const char *const debugTypeName[] = {
	"BEHAVIOR",
	"LOOK_AT",
	"PATH",
	"ANIMATION",
	"LOCOMOTION",
	"VISION",
	"HEARING",
	"EVENTS",
	"ERRORS",
	nullptr,
};


static int g_nBlockedSlides = 0;
static int g_nSlid = 0;
static int g_nRun = 0;


NextBotManager *NextBotManager::sInstance = nullptr;


NextBotManager& TheNextBots()
{
	static NextBotManager manager;
	return manager;
}
