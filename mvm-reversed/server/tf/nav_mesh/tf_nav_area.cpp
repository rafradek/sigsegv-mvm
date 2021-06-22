/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/nav_mesh/tf_nav_area.cpp
 */


ConVar tf_nav_show_incursion_distances("tf_nav_show_incursion_distance", "0", FCVAR_CHEAT,
	"Display travel distances from current spawn room (1=red, 2=blue)");
ConVar tf_nav_show_bomb_target_distance("tf_nav_show_bomb_target_distance", "0", FCVAR_CHEAT,
	"Display travel distances to bomb target (MvM mode)");
ConVar tf_nav_show_turf_ownership("tf_nav_show_turf_ownership", "0", FCVAR_CHEAT,
	"Color nav area by smallest incursion distance");
ConVar tf_nav_in_combat_duration("tf_nav_in_combat_duration", "30", FCVAR_CHEAT,
	"How long after gunfire occurs is this area still considered to be 'in combat'");
ConVar tf_nav_combat_build_rate("tf_nav_combat_build_rate", "0.05", FCVAR_CHEAT,
	"Gunfire/second increase (combat caps at 1.0)");
ConVar tf_nav_combat_decay_rate("tf_nav_combat_decay_rate", "0.022", FCVAR_CHEAT,
	"Decay/second toward zero");
ConVar tf_show_sniper_areas("tf_show_sniper_areas", "0", FCVAR_CHEAT);
ConVar tf_show_sniper_areas_safety_range("tf_show_sniper_areas_safety_range", "1000", FCVAR_CHEAT);
ConVar tf_show_incursion_range("tf_show_incursion_range", "0", FCVAR_CHEAT,
	"1 = red, 2 = blue");
ConVar tf_show_incursion_range_min("tf_show_incursion_range_min", "0", FCVAR_CHEAT,
	"Highlight areas with incursion distances between min and max cvar values");
ConVar tf_show_incursion_range_max("tf_show_incursion_range_max", "0", FCVAR_CHEAT,
	"Highlight areas with incursion distances between min and max cvar values");



