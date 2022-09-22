#ifndef _INCLUDE_SIGSEGV_MOD_ATTR_CUSTOM_ATTRIBUTES_H_
#define _INCLUDE_SIGSEGV_MOD_ATTR_CUSTOM_ATTRIBUTES_H_

namespace Mod::Attr::Custom_Attributes
{
	enum FastAttributeClassPlayer
	{
		STOMP_BUILDING_DAMAGE,
		STOMP_PLAYER_TIME,
		STOMP_PLAYER_DAMAGE,
		STOMP_PLAYER_FORCE,
		NOT_SOLID_TO_PLAYERS,
		MULT_MAX_OVERHEAL_SELF,
		MIN_RESPAWN_TIME,
		MULT_STEP_HEIGHT,
		IGNORE_PLAYER_CLIP,
		ALLOW_BUNNY_HOP,
		ALLOW_FRIENDLY_FIRE,
		MULT_DUCK_SPEED,
		MULT_CREDIT_COLLECT_RANGE,
		NOT_SOLID,

		// Add new entries above this line
		ATTRIB_COUNT_PLAYER,
	};
	enum FastAttributeClassItem
	{
		ALWAYS_CRIT,
		ADD_COND_ON_ACTIVE,
		MAX_AOE_TARGETS,
		DUCK_ACCURACY_MULT,
		CONTINOUS_ACCURACY_MULT,
		CONTINOUS_ACCURACY_TIME,
		CONTINOUS_ACCURACY_TIME_RECOVERY,
		MOVE_ACCURACY_MULT,
		ALT_FIRE_DISABLED,
		PASSIVE_RELOAD,
		
		// Add new entries above this line
		ATTRIB_COUNT_ITEM,
	};
	static const char *fast_attribute_classes_player[ATTRIB_COUNT_PLAYER] = {
		"stomp_building_damage", 
		"stomp_player_time", 
		"stomp_player_damage", 
		"stomp_player_force", 
		"not_solid_to_players",
		"mult_max_ovelheal_self",
		"min_respawn_time",
		"mult_step_height",
		"ignore_player_clip",
		"allow_bunny_hop",
		"allow_friendly_fire",
		"mult_duck_speed",
		"mult_credit_collect_range",
		"not_solid"
	};

	static const char *fast_attribute_classes_item[ATTRIB_COUNT_ITEM] = {
		"always_crit",
		"add_cond_when_active",
		"max_aoe_targets",
		"duck_accuracy_mult",
		"continous_accuracy_mult",
		"continous_accuracy_time",
		"continous_accuracy_time_recovery",
		"move_accuracy_mult",
		"unimplemented_altfire_disabled",
		"passive_reload"
	};

	// For calls outside of custom attributes mod, returns value if the mod is disabled
    float GetFastAttributeFloatExternal(CBaseEntity *entity, float value, int name);

	// For calls outside of custom attributes mod, returns value if the mod is disabled
    int GetFastAttributeIntExternal(CBaseEntity *entity, int value, int name);
}

#endif