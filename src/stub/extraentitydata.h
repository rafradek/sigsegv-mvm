#include "stub/baseentity.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#include "stub/projectiles.h"
#include "util/pooled_string.h"


class EntityModule
{
public:
    EntityModule() {}
    EntityModule(CBaseEntity *entity) {}
};

struct CustomVariable
{
    string_t key;
    string_t value;
    float value_float;
};

class ExtraEntityData
{
public:
    ExtraEntityData(CBaseEntity *entity) {}

    ~ExtraEntityData() {
        for (auto module : modules) {
            delete module.second;
        }
    }

    void AddModule(const char *name, EntityModule *module) {
        for (auto it = modules.begin(); it != modules.end(); it++) {
            if (it->first == name) {
                delete it->second;
                modules.erase(it);
                break;
            }
        }
        modules.push_back({name, module});
    }

    EntityModule *GetModule(const char *name) {
        for (auto &module : modules) {
            if (module.first == name) {
                return module.second;
            }
        }
        return nullptr;
    }

    void RemoveModule(const char *name) {
        for (auto it = modules.begin(); it != modules.end(); it++) {
            if (it->first == name) {
                delete it->second;
                modules.erase(it);
                break;
            }
        }
    }

    std::vector<CustomVariable> &GetCustomVariables() {
        return custom_variables;
    }

private:
    std::vector<std::pair<const char *, EntityModule *>> modules;
    std::vector<CustomVariable> custom_variables;
};

class ExtraEntityDataWithAttributes : public ExtraEntityData
{
public:
    ExtraEntityDataWithAttributes(CBaseEntity *entity) : ExtraEntityData(entity) {}
    // float *fast_attribute_cache;

    // ~ExtraEntityDataWithAttributes() {
    //     delete[] fast_attribute_cache;
    // }
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

class HomingRockets : public EntityModule
{
public:
    HomingRockets() {}
    HomingRockets(CBaseEntity *entity) {}
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

class ExtraEntityDataFuncRotating : public ExtraEntityData
{
public:
    ExtraEntityDataFuncRotating(CBaseEntity *entity) : ExtraEntityData(entity) {}
    
    CHandle<CBaseEntity> m_hRotateTarget;
};

class ExtraEntityDataTriggerDetector : public ExtraEntityData
{
public:
    ExtraEntityDataTriggerDetector(CBaseEntity *entity) : ExtraEntityData(entity) {}
    
    CHandle<CBaseEntity> m_hLastTarget;
    CHandle<CBaseEntity> m_hYRotateEntity;
    CHandle<CBaseEntity> m_hXRotateEntity;
    bool m_bHasTarget;
};

class ExtraEntityDataWeaponSpawner : public ExtraEntityData
{
public:
    ExtraEntityDataWeaponSpawner(CBaseEntity *entity) : ExtraEntityData(entity) {}
    
    std::vector<CHandle<CBaseEntity>> m_SpawnedWeapons;
};

/////////////

inline ExtraEntityDataWithAttributes *GetExtraEntityDataWithAttributes(CBaseEntity *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        if (entity->IsPlayer()) {
            data = entity->m_extraEntityData = new ExtraEntityDataPlayer(entity);
        }
        else if (entity->IsBaseCombatWeapon()) {
            data = entity->m_extraEntityData = new ExtraEntityDataCombatWeapon(entity);
        }
        else if (entity->IsWearable()) {
            data = entity->m_extraEntityData = new ExtraEntityDataWithAttributes(entity);
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

inline ExtraEntityDataFuncRotating *GetExtraFuncRotatingData(CFuncRotating *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataFuncRotating(entity);
    }
    return static_cast<ExtraEntityDataFuncRotating *>(data);
}

inline ExtraEntityDataTriggerDetector *GetExtraTriggerDetectorData(CBaseEntity *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataTriggerDetector(entity);
    }
    return static_cast<ExtraEntityDataTriggerDetector *>(data);
}

inline ExtraEntityDataWeaponSpawner *GetExtraWeaponSpawnerData(CBaseEntity *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataWeaponSpawner(entity);
    }
    return static_cast<ExtraEntityDataWeaponSpawner *>(data);
}


template< typename DataClass, typename EntityClass>
inline DataClass *GetExtraData(EntityClass *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new DataClass(entity);
    }
    return static_cast<DataClass *>(data);
}

template<typename DataClass>
inline DataClass *GetExtraData(CBaseEntity *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new DataClass(entity);
    }
    return static_cast<DataClass *>(data);
}

inline ExtraEntityData *CreateExtraData(CBaseEntity *entity) {
    static PooledString weapon_spawner("$weapon_spawner");
    ExtraEntityData *data = GetExtraEntityDataWithAttributes(entity, true);
    if (data != nullptr) {
        return data;
    }

    auto projectile = rtti_cast<CBaseProjectile *>(entity);
    if (projectile != nullptr) {
        return entity->m_extraEntityData = new ExtraEntityDataProjectile(projectile);
    }

    auto rotating = rtti_cast<CFuncRotating *>(entity);
    if (rotating != nullptr) {
        return entity->m_extraEntityData = new ExtraEntityDataFuncRotating(rotating);
    }

    auto trigger = rtti_cast<CBaseTrigger *>(entity);
    if (trigger != nullptr) {
        return entity->m_extraEntityData = new ExtraEntityDataTriggerDetector(trigger);
    }

    if (entity->GetClassname() == weapon_spawner) {
        return entity->m_extraEntityData = new ExtraEntityDataWeaponSpawner(trigger);
    }

    return entity->m_extraEntityData = new ExtraEntityData(entity);
}

inline ExtraEntityData *GetExtraData(CBaseEntity *entity, bool create = true) {
    if (!create || entity->m_extraEntityData != nullptr) {
        return entity->m_extraEntityData;
    }

    return CreateExtraData(entity);
}

////////

template<class ModuleType>
inline ModuleType *CBaseEntity::GetEntityModule(const char* name)
{
    auto data = this->GetExtraEntityData();
    return data != nullptr ? static_cast<ModuleType *>(data->GetModule(name)) : nullptr;
}

template<class ModuleType>
inline ModuleType *CBaseEntity::GetOrCreateEntityModule(const char* name)
{
    auto data = GetExtraData(this);
    auto module = data->GetModule(name);
    if (module == nullptr) {
        module = new ModuleType(this);
        data->AddModule(name, module);
    }
    return static_cast<ModuleType *>(module);
}

inline void CBaseEntity::AddEntityModule(const char* name, EntityModule *module)
{
    GetExtraData(this)->AddModule(name, module);
}

inline void CBaseEntity::RemoveEntityModule(const char* name)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        data->RemoveModule(name);
    }
}

inline std::vector<CustomVariable> &GetCustomVariables(CBaseEntity *entity)
{
    return GetExtraData(entity)->GetCustomVariables();
}

template<FixedString lit>
inline const char *CBaseEntity::GetCustomVariable(const char *defValue)
{
    static PooledString pooled(lit);
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        for (auto &var : attrs) {
            if (var.key == pooled) {
                return STRING(var.value);
            }
        }
    }
    return defValue;
}

template<FixedString lit>
inline float CBaseEntity::GetCustomVariableFloat(float defValue)
{
    static PooledString pooled(lit);
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        for (auto &var : attrs) {
            if (var.key == pooled) {
                return var.value_float;
            }
        }
    }
    return defValue;
}

inline const char *CBaseEntity::GetCustomVariableByText(const char *key, const char *defValue)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        for (auto &var : attrs) {
            if (STRING(var.key) == key || stricmp(STRING(var.key), key) == 0) {
                return STRING(var.value);
            }
        }
    }
    return defValue;
}

inline void CBaseEntity::SetCustomVariable(const char *key, const char *value)
{
    auto &list = GetExtraData(this)->GetCustomVariables();
    bool found = false;
    for (auto &var : list) {
        if (STRING(var.key) == key || stricmp(key, STRING(var.key)) == 0) {
            var.value = AllocPooledString(value);
            var.value_float = strtof(value, nullptr);
            found = true;
            break;
        }
    }
    if (!found) {
        list.push_back({AllocPooledString(key), AllocPooledString(value), strtof(value, nullptr)});
    }
}