/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/nav_mesh/tf_nav_mesh.cpp
 */


ConVar tf_show_in_combat_areas("tf_show_in_combat_areas", "0", FCVAR_CHEAT);
ConVar tf_show_enemy_invasion_areas("tf_show_enemy_invasion_areas", "0", FCVAR_CHEAT,
	"Highlight areas where the enemy team enters the visible environment of the local player");
ConVar tf_show_blocked_areas("tf_show_blocked_areas", "0", FCVAR_CHEAT,
	"Highlight areas that are considered blocked for TF-specific reasons");
ConVar tf_show_incursion_flow("tf_show_incursion_flow", "0", FCVAR_CHEAT);
ConVar tf_show_incursion_flow_range("tf_show_incursion_flow_range", "150", FCVAR_CHEAT,
	"1 = red, 2 = blue");
ConVar tf_show_incursion_flow_gradient("tf_show_incursion_flow_gradient", "0", FCVAR_CHEAT,
	"1 = red, 2 = blue");
ConVar tf_show_mesh_decoration("tf_show_mesh_decoration", "0", FCVAR_CHEAT,
	"Highlight special areas");
ConVar tf_show_mesh_decoration_manual("tf_show_mesh_decoration_manual", "0", FCVAR_CHEAT,
	"Highlight special areas marked by hand");
ConVar tf_show_sentry_danger("tf_show_sentry_danger", "0", FCVAR_CHEAT,
	"Show sentry danger areas. 1:Use m_sentryAreas. 2:Check all nav areas.");
ConVar tf_show_actor_potential_visibility("tf_show_actor_potential_visibility", "0", FCVAR_CHEAT);
ConVar tf_show_control_points("tf_show_control_points", "0", FCVAR_CHEAT);
ConVar tf_show_bomb_drop_areas("tf_show_bomb_drop_areas", "0", FCVAR_CHEAT);
ConVar tf_bot_min_setup_gate_defend_range("tf_bot_min_setup_gate_defend_range", "750", FCVAR_CHEAT,
	"How close from the setup gate(s) defending bots can take up positions. Areas closer than this will be in cover to ambush.");
ConVar tf_bot_max_setup_gate_defend_range("tf_bot_max_setup_gate_defend_range", "2000", FCVAR_CHEAT,
	"How far from the setup gate(s) defending bots can take up positions");
ConVar tf_bot_min_setup_gate_sniper_defend_range("tf_bot_min_setup_gate_sniper_defend_range", "1500", FCVAR_CHEAT,
	"How far from the setup gate(s) a defending sniper will take up position");
ConVar tf_show_gate_defense_areas("tf_show_gate_defense_areas", "0", FCVAR_CHEAT);
ConVar tf_show_point_defense_areas("tf_show_point_defense_areas", "0", FCVAR_CHEAT);
ConVar tf_select_ambush_areas_radius("tf_select_ambush_areas_radius", "750", FCVAR_CHEAT);
ConVar tf_select_ambush_areas_close_range("tf_select_ambush_areas_close_range", "300", FCVAR_CHEAT);
ConVar tf_select_ambush_areas_max_enemy_exposure_area("tf_select_ambush_areas_max_enemy_exposure_area", "500000", FCVAR_CHEAT);

ConCommand tf_select_ambush_areas("tf_select_ambush_areas", &CMD_SelectAmbushAreas,
	"Add good ambush spots to the selected set. For debugging.", FCVAR_CHEAT | FCVAR_GAMEDLL);



