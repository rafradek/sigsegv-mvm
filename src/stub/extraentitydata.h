#ifndef _INCLUDE_SIGSEGV_STUB_EXTRAENTITYDATA_H_
#define _INCLUDE_SIGSEGV_STUB_EXTRAENTITYDATA_H_

#include "stub/baseentity.h"
#ifdef SE_IS_TF2
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"
#else
#include "stub/baseplayer.h"
#include "stub/baseweapon.h"
#endif
#include "stub/projectiles.h"
#include "util/pooled_string.h"
#include <boost/algorithm/string.hpp>


class EntityModule
{
public:
    EntityModule() {}
    EntityModule(CBaseEntity *entity) {}

    virtual ~EntityModule() {}
};

class CustomVariable
{
public:
    CustomVariable(string_t key, const variant_t &value)
    {
        this->key = key;
        this->value = value;
    }

    CustomVariable(const char *key, const variant_t &value) : CustomVariable(AllocPooledString(key), value) {}

    string_t key;
    variant_t value;
};

struct CustomOutput
{
    string_t key;
    CBaseEntityOutput output;
};

class ExtraEntityData
{
public:
    ExtraEntityData(CBaseEntity *entity) {}

    virtual ~ExtraEntityData() {
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

    std::vector<CustomOutput> &GetCustomOutputs() {
        return custom_outputs;
    }

private:
    std::vector<std::pair<const char *, EntityModule *>> modules;
    std::vector<CustomVariable> custom_variables;
    std::vector<CustomOutput> custom_outputs;
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

    float lastShootTime = 0.0f;
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
    bool predict_target_speed   = true;
    float speed                 = 1.0f;
    float turn_power            = 0.0f;
    float min_dot_product       = -0.25f;
    float aim_time              = 9999.0f;
    float aim_start_time        = 0.0f;
    float acceleration          = 0.0f;
    float acceleration_time     = 9999.0f;
    float acceleration_start    = 0.0f;
    float gravity               = 0.0f;
    bool homed_in               = false;
    bool returning              = false;
    int return_to_sender        = 0;
    QAngle homed_in_angle;
};

class ExtraEntityDataProjectile : public ExtraEntityData
{
public:
    ExtraEntityDataProjectile(CBaseEntity *entity) : ExtraEntityData(entity) {}
    
    virtual ~ExtraEntityDataProjectile() {
        if (homing != nullptr) {
            delete homing;
        }
    }
    HomingRockets *homing = nullptr;
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
#ifdef SE_IS_TF2
    CHandle<CEconEntity> quickItemInLoadoutSlot[LOADOUT_POSITION_COUNT];
#endif

    float lastHurtSpeakTime = 0.0f;

    float lastForwardMove = 0.0f;
    float lastSideMove = 0.0f;
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

#ifdef SE_IS_TF2
class ExtraEntityDataWeaponSpawner : public ExtraEntityData
{
public:
    ExtraEntityDataWeaponSpawner(CBaseEntity *entity) : ExtraEntityData(entity) {}
    
    std::vector<CHandle<CBaseEntity>> m_SpawnedWeapons;
};
#endif

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
        else if (entity->IsCombatCharacter()) {
            data = entity->m_extraEntityData = new ExtraEntityDataCombatCharacter(entity);
        }
    }
    return static_cast<ExtraEntityDataWithAttributes *>(data);
}

inline ExtraEntityDataPlayer *GetExtraPlayerData(CBasePlayer *entity, bool create = true) {
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

inline ExtraEntityDataBot *GetExtraBotData(CBasePlayer *entity, bool create = true) {
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

#ifdef SE_IS_TF2
inline ExtraEntityDataWeaponSpawner *GetExtraWeaponSpawnerData(CBaseEntity *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = new ExtraEntityDataWeaponSpawner(entity);
    }
    return static_cast<ExtraEntityDataWeaponSpawner *>(data);
}
#endif

inline ExtraEntityDataCombatCharacter *GetExtraCombatCharacterData(CBaseCombatCharacter *entity, bool create = true) {
    ExtraEntityData *data = entity->m_extraEntityData;
    if (create && entity->m_extraEntityData == nullptr) {
        data = entity->m_extraEntityData = GetExtraEntityDataWithAttributes(entity, true);
    }
    return static_cast<ExtraEntityDataCombatCharacter *>(data);
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

#ifdef SE_IS_TF2
    if (entity->GetClassname() == PStr<"$weapon_spawner">()) {
        return entity->m_extraEntityData = new ExtraEntityDataWeaponSpawner(trigger);
    }
#endif

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
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    return var.value.String();
                }
            }
        }
    }
    return defValue;
}

template<FixedString lit>
inline float CBaseEntity::GetCustomVariableFloat(float defValue)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    if (var.value.FieldType() != FIELD_FLOAT) var.value.Convert(FIELD_FLOAT);
                    return var.value.Float();
                }
            }
        }
    }
    return defValue;
}

template<FixedString lit>
inline int CBaseEntity::GetCustomVariableInt(int defValue)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    if (var.value.FieldType() != FIELD_INTEGER) var.value.Convert(FIELD_INTEGER);
                    return var.value.Int();
                }
            }
        }
    }
    return defValue;
}

template<FixedString lit>
inline bool CBaseEntity::GetCustomVariableBool(bool defValue)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    if (var.value.FieldType() != FIELD_BOOLEAN) var.value.Convert(FIELD_BOOLEAN);
                    return var.value.Bool();
                }
            }
        }
    }
    return defValue;
}

template<FixedString lit>
inline Vector CBaseEntity::GetCustomVariableVector(const Vector &defValue)
{
   auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    Vector vec;
                    if (var.value.FieldType() != FIELD_VECTOR) var.value.Convert(FIELD_VECTOR);
                    var.value.Vector3D(vec);
                    return vec;
                }
            }
        }
    }
    return defValue;
}

template<FixedString lit>
inline QAngle CBaseEntity::GetCustomVariableAngle(const QAngle &defValue)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    QAngle ang;

                    if (var.value.FieldType() != FIELD_VECTOR) var.value.Convert(FIELD_VECTOR);
                    var.value.Vector3D(*((Vector *)ang.Base()));
                    
                    return ang;
                }
            }
        }
    }
    return defValue;
}

template<FixedString lit>
inline bool CBaseEntity::GetCustomVariableVariant(variant_t &value)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    value = var.value;
                    
                    return true;
                }
            }
        }
    }
    return false;
}

template<FixedString lit>
inline CBaseEntity *CBaseEntity::GetCustomVariableEntity(CBaseEntity *defValue)
{
   auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        if (!attrs.empty()) {
            static PooledString pooled(lit);
            for (auto &var : attrs) {
                if (var.key == pooled) {
                    if (var.value.FieldType() != FIELD_EHANDLE) var.value.Convert(FIELD_EHANDLE);
                    return var.value.Entity();
                }
            }
        }
    }
    return defValue;
}

inline bool CBaseEntity::GetCustomVariableByText(const char *key, variant_t &value)
{
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomVariables();
        for (auto &var : attrs) {
            if (STRING(var.key) == key || stricmp(STRING(var.key), key) == 0) {
                value = var.value;
                return true;
            }
        }
    }
    return false;
}

inline bool CBaseEntity::SetCustomVariable(const char *key, const variant_t &value, bool create, bool find)
{
    auto &list = GetExtraData(this)->GetCustomVariables();
    bool found = false;
    if (find) {
        for (auto &var : list) {
            if (STRING(var.key) == key || stricmp(key, STRING(var.key)) == 0) {
                var.value = value;
                found = true;
                return true;
            }
        }
    }
    if (!found && create) {
        list.emplace_back(key, value);
        return true;
    }
    return false;
}

inline bool CBaseEntity::SetCustomVariable(string_t key, const variant_t &value, bool create, bool find)
{
    auto &list = GetExtraData(this)->GetCustomVariables();
    bool found = false;
    if (find) {
        for (auto &var : list) {
            if (var.key == key) {
                var.value = value;
                found = true;
                return true;
            }
        }
    }
    if (!found && create) {
        list.emplace_back(key, value);
        return true;
    }
    return false;
}

template<FixedString lit>
inline bool CBaseEntity::SetCustomVariable(const variant_t &value, bool create, bool find)
{
    auto &list = GetExtraData(this)->GetCustomVariables();
    bool found = false;
    static PooledString pooled(lit);
    if (find) {
        for (auto &var : list) {
            if (var.key == pooled) {
                var.value = value;
                found = true;
                return true;
            }
        }
    }
    if (!found && create) {
        list.emplace_back((string_t)pooled, value);
        return true;
    }
    return false;
}

template<FixedString lit>
inline void CBaseEntity::FireCustomOutput(CBaseEntity *activator, CBaseEntity *caller, variant_t variant)
{
    static PooledString pooled(lit);
    auto data = this->GetExtraEntityData();
    if (data != nullptr) {
        auto &attrs = data->GetCustomOutputs();
        for (auto &var : attrs) {
            if (var.key == pooled) {
                var.output.FireOutput(variant, activator, caller);
                return;
            }
        }
    }
}
#endif