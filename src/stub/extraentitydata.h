#include "stub/baseentity.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"

class ExtraEntityData
{
public:
    ExtraEntityData(CBaseEntity *entity) {}
};

class ExtraEntityDataWithAttributes : public ExtraEntityData
{
public:
    ExtraEntityDataWithAttributes(CBaseEntity *entity) : ExtraEntityData(entity) {}
//    float *fast_attribute_cache;
};

class ExtraEntityDataEconEntity : public ExtraEntityDataWithAttributes
{
public:
    ExtraEntityDataEconEntity(CBaseEntity *entity) : ExtraEntityDataWithAttributes(entity) {}
};


class ExtraEntityDataCombatCharacter : public ExtraEntityDataWithAttributes
{
public:
    ExtraEntityDataCombatCharacter(CBaseEntity *entity) : ExtraEntityDataWithAttributes(entity) {}
};

class ExtraEntityDataCombatWeapon : public ExtraEntityDataEconEntity
{
public:
    ExtraEntityDataCombatWeapon(CBaseEntity *entity) : ExtraEntityDataEconEntity(entity) {
    //    fast_attribute_cache = fast_attrib_cache_data;
    //    for (int i = 0; i < FastAttributes::ATTRIB_COUNT_ITEM; i++) {
    //        fast_attrib_cache_data[i] = FLT_MIN;
    //    }
    }

    //float[FastAttributes::ATTRIB_COUNT_ITEM] fast_attrib_cache_data;
};

struct HomingRockets
{
    bool enable                 = false;
    bool ignore_disguised_spies = true;
    bool ignore_stealthed_spies = true;
    bool follow_crosshair       = false;
    float speed                 = 1.0f;
    float turn_power            = 0.0f;
    float min_dot_product       = -0.25f;
    float aim_time              = 9999.0f;
    float acceleration          = 0.0f;
    float acceleration_time     = 9999.0f;
    float acceleration_start    = 0.0f;
    float gravity               = 0.0f;
};

class ExtraEntityDataProjectile : public ExtraEntityData
{
public:
    ExtraEntityDataProjectile(CBaseEntity *entity) : ExtraEntityData(entity) {}
    
    ~ExtraEntityDataProjectile() {
        if (homing != nullptr) {
            delete homing;
        }
    }
    HomingRockets *homing;
};

class ExtraEntityDataPlayer : public ExtraEntityDataCombatCharacter
{
public:
    ExtraEntityDataPlayer(CBaseEntity *entity) : ExtraEntityDataCombatCharacter(entity) {
    //    fast_attribute_cache = fast_attrib_cache_data;
    //    for (int i = 0; i < FastAttributes::ATTRIB_COUNT_PLAYER; i++) {
    //        fast_attrib_cache_data[i] = FLT_MIN;
    //    }
    }

    //float[FastAttributes::ATTRIB_COUNT_PLAYER] fast_attrib_cache_data;
};

class ExtraEntityDataBot : public ExtraEntityDataPlayer
{
public:
    ExtraEntityDataBot(CBaseEntity *entity) : ExtraEntityDataPlayer(entity) {}
};

inline ExtraEntityDataWithAttributes *GetExtraEntityDataWithAttributes(CBaseEntity *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        if (entity->IsPlayer()) {
            data = entity->m_extraEntityData = new ExtraEntityDataPlayer(entity);
        }
        else if (entity->IsBaseCombatWeapon()) {
            data = entity->m_extraEntityData = new ExtraEntityDataCombatWeapon(entity);
        }
    }
    return static_cast<ExtraEntityDataWithAttributes *>(data);
}

inline ExtraEntityDataPlayer *GetExtraPlayerData(CTFPlayer *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataPlayer(entity);
    }
    return static_cast<ExtraEntityDataPlayer *>(data);
}

inline ExtraEntityDataCombatWeapon *GetExtraWeaponData(CBaseCombatWeapon *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataCombatWeapon(entity);
    }
    return static_cast<ExtraEntityDataCombatWeapon *>(data);
}

inline ExtraEntityDataBot *GetExtraBotData(CTFPlayer *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataBot(entity);
    }
    return static_cast<ExtraEntityDataBot *>(data);
}

inline ExtraEntityDataProjectile *GetExtraProjectileData(CBaseProjectile *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataProjectile(entity);
    }
    return static_cast<ExtraEntityDataProjectile *>(data);
}

////////


namespace FastAttributes
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
		ATTRIB_COUNT_PLAYER,
	};
	enum FastAttributeClassItem
	{
		ALWAYS_CRIT,
		ADD_COND_ON_ACTIVE,
		MAX_AOE_TARGETS,
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
		"allow_bunny_hop"
	};

	static const char *fast_attribute_classes_item[ATTRIB_COUNT_ITEM] = {
		"always_crit",
		"add_cond_when_active",
		"max_aoe_targets"
	};
}