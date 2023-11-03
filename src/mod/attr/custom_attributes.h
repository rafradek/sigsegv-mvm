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
		IGNORED_BY_BOTS,
		ADD_MAXHEALTH,
		ADD_MAXHEALTH_NONBUFFED,
		MOVE_SPEED_AS_HEALTH_DECREASES,
		NO_CLIP,
		DISPLACE_TOUCHED_ENEMIES,

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
		IS_PASSIVE_WEAPON,
		MOD_MAX_PRIMARY_CLIP_OVERRIDE,
		AUTO_FIRES_FULL_CLIP,
		MOD_MAXHEALTH_DRAIN_RATE,
		MIDAIR_ACCURACY_MULT,
		ALT_FIRE_ATTACK,
		NO_ATTACK,
		MOD_AMMO_PER_SHOT,
		HOLD_FIRE_UNTIL_FULL_RELOAD,

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
		"not_solid",
		"ignored_by_bots",
		"add_maxhealth",
		"add_maxhealth_nonbuffed",
		"move_speed_as_health_decreases",
		"no_clip",
		"displace_touched_enemies"
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
		"passive_reload",
		"is_passive_weapon",
		"mod_max_primary_clip_override",
		"auto_fires_full_clip",
		"mod_maxhealth_drain_rate",
		"midair_accuracy_mult",
		"alt_fire_attack",
		"no_attack",
		"mod_ammo_per_shot",
		"hold_fire_until_full_reload"
	};

	// For calls outside of custom attributes mod, returns value if the mod is disabled
    float GetFastAttributeFloatExternal(CBaseEntity *entity, float value, int name);

	// For calls outside of custom attributes mod, returns value if the mod is disabled
    int GetFastAttributeIntExternal(CBaseEntity *entity, int value, int name);
	
	bool CanSapBuilding(CBaseObject *obj);

	bool IsCustomViewmodelAllowed(CTFPlayer *player);
}
#endif