/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/tf_bot_temp.cpp
 */


ConVar bot_debug("bot_debug", "0", FCVAR_CHEAT, "Bot debugging.");
ConVar bot_forcefireweapon("bot_forcefireweapon", "", FCVAR_NONE,
	"Force bots with the specified weapon to fire.");
ConVar bot_forceattack("bot_forceattack", "0", FCVAR_NONE,
	"When on, all bots fire their guns.");
ConVar bot_forceattack2("bot_forceattack2", "0", FCVAR_NONE,
	"When firing, use attack2.");
ConVar bot_forceattack_down("bot_forceattack_down", "1", FCVAR_NONE,
	"When firing, don't tap fire, hold it down.");
ConVar bot_changeclass("bot_changeclass", "0", FCVAR_NONE,
	"Force all bots to change to the specified class.");
ConVar bot_dontmove("bot_dontmove", "0", FCVAR_CHEAT);
ConVar bot_saveme("bot_saveme", "0", FCVAR_CHEAT);
ConVar bot_mimic_inverse("bot_mimic_inverse", "0", FCVAR_NONE,
	"Bot uses usercmd of player by index.");
ConVar bot_mimic_yaw_offset("bot_mimic_yaw_offset", "180", FCVAR_NONE,
	"Offsets the bot yaw.");
ConVar bot_selectweaponslot("bot_selectweaponslot", "-1", FCVAR_CHEAT,
	"set to weapon slot that bot should switch to.");
ConVar bot_randomnames("bot_randomnames", "0", FCVAR_CHEAT);
ConVar bot_jump("bot_jump", "0", FCVAR_CHEAT,
	"Force all bots to repeatedly jump.");
ConVar bot_crouch("bot_crouch", "0", FCVAR_CHEAT,
	"Force all bots to crouch.");
ConVar bot_nav_turnspeed("bot_nav_turnspeed", "5", FCVAR_CHEAT,
	"Rate at which bots turn to face their targets.");
ConVar bot_nav_wpdistance("bot_nav_wpdistance", "16", FCVAR_CHEAT,
	"Distance to a waypoint within which a bot considers as having reached it.");
ConVar bot_nav_wpdeceldistance("bot_nav_wpdeceldistance", "128", FCVAR_CHEAT,
	"Distance to a waypoint to which a bot starts to decelerate to reach it.");
ConVar bot_nav_simplifypaths("bot_nav_simplifypaths", "1", FCVAR_CHEAT,
	"If set, bots will skip waypoints if they already see the waypoint post.");
ConVar bot_nav_useoffsetpaths("bot_nav_useoffsetpaths", "1", FCVAR_CHEAT,
	"If set, bots will generate waypoints on both sides of portals between waypoints when building paths.");
ConVar bot_nav_offsetpathinset("bot_nav_offsetpathinset", "20", FCVAR_CHEAT,
	"Distance into an area that waypoints should be generated when pathfinding through portals.");
ConVar bot_nav_usefeelers("bot_nav_usefeelers", "1", FCVAR_CHEAT,
	"If set, bots will extend feelers to their sides to find & avoid upcoming collisions.");
ConVar bot_nav_recomputetime("bot_nav_recomputetime", "0.5", FCVAR_CHEAT,
	"Delay before bots recompute their path to targets that have moved when moving to them.");
ConVar bot_com_meleerange("bot_com_meleerange", "80", FCVAR_CHEAT,
	"Distance to a target that a melee bot wants to be within to attack.");
ConVar bot_com_wpnrange("bot_com_wpnrange", "400", FCVAR_CHEAT,
	"Distance to a target that a ranged bot wants to be within to attack.");
ConVar bot_com_viewrange("bot_com_viewrange", "2000", FCVAR_CHEAT,
	"Distance within which bots looking for any enemies will find them.");

ConCommand bot_kick_command("bot_kick", &bot_kick,
	"Remove a bot by name, or an entire team (\"red\" or \"blue\"), or all bots (\"all\").", FCVAR_CHEAT);
ConCommand bot_command("bot", &bot,
	"Add a bot.", FCVAR_NONE);
ConCommand bot_hurt_command("bot_hurt", &bot_hurt,
	"Hurt a bot by team, or all bots (\"all\").", FCVAR_GAMEDLL);
ConCommand bot_sendcommand("bot_command", &cc_bot_sendcommand,
	"<bot id> <command string...>.  Sends specified command on behalf of specified bot", FCVAR_CHEAT);
ConCommand bot_kill("bot_kill", &cc_bot_kill,
	"Kills a bot. Usage: bot_kill <bot name>", FCVAR_CHEAT);
ConCommand bot_changeteams_command("bot_changeteams", &bot_changeteams,
	"Make all bots change teams", FCVAR_CHEAT);
ConCommand bot_refill_command("bot_refill", &bot_refill,
	"Refill all bot ammo counts", FCVAR_CHEAT);
ConCommand bot_whack_command("bot_whack", &bot_whack,
	"Deliver lethal damage from player to specified bot. Usage: bot_whack <bot name>", FCVAR_CHEAT);
ConCommand bot_teleport_command("bot_teleport", &bot_teleport,
	"Teleport the specified bot to the specified position & angles.\n\tFormat: bot_teleport <bot name> <X> <Y> <Z> <Pitch> <Yaw> <Roll>", FCVAR_CHEAT);
ConCommand bot_mirror_command("bot_mirror", &bot_mirror,
	"Forces the specified bot to be the same class, and use the same items, as you.", FCVAR_CHEAT);
ConCommand bot_selectweapon_command("bot_selectweapon", &cc_bot_selectweapon,
	"Force a bot to select a weapon in a slot. Usage: bot_selectweapon <bot name> <weapon slot>", FCVAR_CHEAT);
ConCommand bot_drop_command("bot_drop", &bot_drop,
	"Force the specified bot to drop his active weapon. Usage: bot_drop <bot name>", FCVAR_CHEAT);
ConCommand bot_moveto_command("bot_moveto", &bot_moveto,
	"Force the specified bot to move to the point under your crosshair. Usage: bot_moveto <bot name>", FCVAR_CHEAT);


// static g_BotData (CUtlVector<botdata_t::CommandInfo>)


// TODO: server class init for CTFBotController ("bot_controller")



