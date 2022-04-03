#include "mod.h"
#include "stub/baseentity.h"
#include "stub/entities.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
#include "stub/tf_shareddefs.h"
#include "stub/misc.h"
#include "stub/strings.h"
#include "stub/server.h"
#include "stub/objects.h"
#include "stub/extraentitydata.h"
#include "util/pooled_string.h"
#include "util/scope.h"
#include "util/iterate.h"
#include "util/misc.h"
#include "util/clientmsg.h"
#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <regex>
#include <string_view>
#include <optional>
#include <charconv>
#include "stub/sendprop.h"
#include "mod/pop/popmgr_extensions.h"
#include "util/vi.h"
#include "util/expression_eval.h"

namespace Mod::Etc::Mapentity_Additions
{

    static const char *SPELL_TYPE[] = {
        "Fireball",
        "Ball O' Bats",
        "Healing Aura",
        "Pumpkin MIRV",
        "Superjump",
        "Invisibility",
        "Teleport",
        "Tesla Bolt",
        "Minify",
        "Meteor Shower",
        "Summon Monoculus",
        "Summon Skeletons"
    };

    class CaseMenuHandler : public IMenuHandler
    {
    public:

        CaseMenuHandler(CTFPlayer * pPlayer, CLogicCase *pProvider) : IMenuHandler() {
            this->player = pPlayer;
            this->provider = pProvider;
        }

        void OnMenuSelect(IBaseMenu *menu, int client, unsigned int item) {

            if (provider == nullptr)
                return;
                
            const char *info = menu->GetItemInfo(item, nullptr);

            provider->FireCase(item + 1, player);
            variant_t variant;
            provider->FireCustomOutput<"onselect">(player, provider, variant);
        }

        virtual void OnMenuCancel(IBaseMenu *menu, int client, MenuCancelReason reason)
		{
            if (provider == nullptr)
                return;

            variant_t variant;
            provider->m_OnDefault->FireOutput(variant, player, provider);
		}

        virtual void OnMenuEnd(IBaseMenu *menu, MenuEndReason reason)
		{
            menu->Destroy(false);
		}

        void OnMenuDestroy(IBaseMenu *menu) {
            DevMsg("Menu destroy\n");
            delete this;
        }

        CHandle<CTFPlayer> player;
        CHandle<CLogicCase> provider;
    };
    
    struct SendPropCacheEntry {
        ServerClass *serverClass;
        std::string name;
        int offset;
        bool isVecAxis;
        SendProp *prop;
    };

    struct DatamapCacheEntry {
        datamap_t *datamap;
        std::string name;
        int offset;
        fieldtype_t fieldType;
        int size;
    };

    std::vector<SendPropCacheEntry> send_prop_cache;
    std::vector<DatamapCacheEntry> datamap_cache;
    std::vector<std::pair<string_t, CHandle<CBaseEntity>>> entity_listeners;

    PooledString trigger_detector_class("$trigger_detector");
    void AddModuleByName(CBaseEntity *entity, const char *name);

    bool ReadVectorIndexFromString(std::string &name, int &vecOffset)
    {
        size_t nameSize = name.size();
        vecOffset = -1;
        if (nameSize > 2 && name.at(nameSize - 2) == '$') {
            switch(name.at(nameSize - 1)) {
                case 'x': case 'X': vecOffset = 0; break;
                case 'y': case 'Y': vecOffset = 1; break;
                case 'z': case 'Z': vecOffset = 2; break;
            }
            if (vecOffset != -1) {
                name.resize(nameSize - 2);
            }
        }
        return vecOffset != -1;
    }

    inline void ConvertToVectorIndex(variant_t &value, int vecOffset)
    {
        if (vecOffset != -1) {
            Vector vec;
            value.Vector3D(vec);
            value.SetFloat(vec[vecOffset]);
        }
    }

    void SetCustomVariable(CBaseEntity *entity, const char *key, variant_t &value)
    {
        if (value.FieldType() == FIELD_STRING) {
            ParseNumberOrVectorFromString(value.String(), value);
        }
        
        std::string nameNoArray = key;
        int vecOffset;
        ReadVectorIndexFromString(nameNoArray, vecOffset);
        if (vecOffset != -1) {
            Vector vec;
            entity->GetCustomVariableByText(nameNoArray.c_str(), value);
            value.Vector3D(vec);
            vec[vecOffset] = value.Float();
            value.SetVector3D(vec);
        }

        entity->SetCustomVariable(nameNoArray.c_str(), value);
    }

    bool FindSendProp(int& off, SendTable *s_table, const char *name, SendProp *&prop, int index = -1)
    {
        for (int i = 0; i < s_table->GetNumProps(); ++i) {
            SendProp *s_prop = s_table->GetProp(i);
            
            if (s_prop->GetName() != nullptr && strcmp(s_prop->GetName(), name) == 0) {
                off += s_prop->GetOffset();
                if (index >= 0) {
                    if (s_prop->GetDataTable() != nullptr && index < s_table->GetNumProps()) {
                        prop = s_prop->GetDataTable()->GetProp(index);
                        off += prop->GetOffset();
                        return true;
                    }
                    if (s_prop->IsInsideArray()) {
                        auto prop_array = s_prop->GetDataTable()->GetProp(i + 1);
                        if (prop_array != nullptr && prop_array->GetType() == DPT_Array && index < prop_array->GetNumElements()) {
                            off += prop_array->GetElementStride();
                        }
                    }
                }
                prop = s_prop;
                return true;
            }
            
            if (s_prop->GetDataTable() != nullptr) {
                off += s_prop->GetOffset();
                if (FindSendProp(off, s_prop->GetDataTable(), name, prop, index)) {
                    return true;
                }
                off -= s_prop->GetOffset();
            }
        }
        
        return false;
    }

    bool ReadArrayIndexFromString(std::string &name, int &arrayPos, int &offset, bool &isVecAxis)
    {
        size_t arrayStr = name.find('$');
        const char *vecChar = nullptr;
        if (arrayStr != std::string::npos) {
            StringToIntStrict(name.c_str() + arrayStr + 1, arrayPos, 0, &vecChar);
            name.resize(arrayStr);
            if (vecChar != nullptr) {
                switch (*vecChar) {
                    case 'x': case 'X': isVecAxis = true; break;
                    case 'y': case 'Y': isVecAxis = true; offset += 4; break;
                    case 'z': case 'Z': isVecAxis = true; offset += 8; break;
                }
            }
            return true;
        }
        return false;
    }

    SendPropCacheEntry &GetSendPropOffset(ServerClass *serverClass, const char *name) {
        
        for (auto &entry : send_prop_cache) {
            if (entry.serverClass == serverClass && entry.name == name) {
                return entry;
            }
        }

        std::string nameNoArray = name;
        int arrayPos = -1;
        int offset = 0;
        bool isVecAxis = false;
        ReadArrayIndexFromString(nameNoArray, arrayPos, offset, isVecAxis);

        SendProp *prop = nullptr;
        FindSendProp(offset,serverClass->m_pTable, nameNoArray.c_str(), prop, arrayPos);
        if (prop == nullptr) {
            offset = 0;
        }
        else if (prop->GetArrayProp() != nullptr) {
            prop = prop->GetArrayProp();
        }
        
        send_prop_cache.push_back({serverClass, name, offset, isVecAxis, prop});
        return send_prop_cache.back();
    }

    DatamapCacheEntry &GetDataMapOffset(datamap_t *datamap, const char *name) {
        
        for (auto &entry : datamap_cache) {
            if (entry.datamap == datamap && entry.name == name) {
                return entry;
            }
        }

        std::string nameNoArray = name;
        int arrayPos = 0;
        int extraOffset = 0;
        bool isVecAxis = false;
        ReadArrayIndexFromString(nameNoArray, arrayPos, extraOffset, isVecAxis);

        for (datamap_t *dmap = datamap; dmap != NULL; dmap = dmap->baseMap) {
            // search through all the readable fields in the data description, looking for a match
            for (int i = 0; i < dmap->dataNumFields; i++) {
                if (dmap->dataDesc[i].fieldName != nullptr && strcmp(dmap->dataDesc[i].fieldName, nameNoArray.c_str()) == 0) {
                    fieldtype_t fieldType = isVecAxis ? FIELD_FLOAT : dmap->dataDesc[i].fieldType;
                    int offset = dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ] + extraOffset;
                    
                    offset += clamp(arrayPos, 0, (int)dmap->dataDesc[i].fieldSize) * (dmap->dataDesc[i].fieldSizeInBytes / dmap->dataDesc[i].fieldSize);

                    datamap_cache.push_back({datamap, name, offset, fieldType, dmap->dataDesc[i].fieldSize});
                    return datamap_cache.back();
                }
            }
        }

        datamap_cache.push_back({datamap, name, 0, FIELD_VOID, 0});
        return datamap_cache.back();
    }

    void ParseCustomOutput(CBaseEntity *entity, const char *name, const char *value) {
        std::string namestr = name;
        boost::algorithm::to_lower(namestr);
    //  DevMsg("Add custom output %d %s %s\n", entity, namestr.c_str(), value);
        entity->AddCustomOutput(namestr.c_str(), value);
        variant_t variant;
        variant.SetString(AllocPooledString(value));
        SetCustomVariable(entity, namestr.c_str(), variant);

        if (FStrEq(name, "modules")) {
            std::string str(value);
            boost::tokenizer<boost::char_separator<char>> tokens(str, boost::char_separator<char>(","));

            for (auto &token : tokens) {
                AddModuleByName(entity,token.c_str());
            }
        }
        if (entity->GetClassname() == PStr<"$entity_spawn_detector">() && FStrEq(name, "name")) {
            bool found = false;
            for (auto &pair : entity_listeners) {
                if (pair.second == entity) {
                    pair.first = AllocPooledString(value);
                    found = true;
                    break;
                }
            }
            if (!found) {
                entity_listeners.push_back({AllocPooledString(value), entity});
            }
        }
    }

    void FireFormatInput(CLogicCase *entity, CBaseEntity *activator, CBaseEntity *caller)
    {
        std::string fmtstr = STRING(entity->m_nCase[15]);
        unsigned int pos = 0;
        unsigned int index = 1;
        while ((pos = fmtstr.find('%', pos)) != std::string::npos ) {
            if (pos != fmtstr.size() - 1 && fmtstr[pos + 1] == '%') {
                fmtstr.erase(pos, 1);
                pos++;
            }
            else
            {
                string_t str = entity->m_nCase[index - 1];
                fmtstr.replace(pos, 1, STRING(str));
                index++;
                pos += strlen(STRING(str));
            }
        }

        variant_t variant1;
        variant1.SetString(AllocPooledString(fmtstr.c_str()));
        entity->m_OnDefault->FireOutput(variant1, activator, entity);
    }

    enum GetInputType {
        ANY,
        VARIABLE,
        KEYVALUE,
        DATAMAP,
        SENDPROP
    };

    CStandardSendProxies* sendproxies = nullptr;
    bool SetEntityVariable(CBaseEntity *entity, GetInputType type, const char *name, variant_t &variable) {
        bool found = false;

        if (type == ANY) {
            found = SetEntityVariable(entity, DATAMAP, name, variable) ||
                    SetEntityVariable(entity, SENDPROP, name, variable) ||
                    SetEntityVariable(entity, KEYVALUE, name, variable) ||
                    SetEntityVariable(entity, VARIABLE, name, variable);
        }
        else if (type == VARIABLE) {
            SetCustomVariable(entity, name, variable);
            found = true;
        }
        else if (type == KEYVALUE) {
            std::string nameNoArray = name;
            int vecOffset;
            ReadVectorIndexFromString(nameNoArray, vecOffset);
            if (vecOffset != -1) {
                Vector vec;
                variant_t variant;
                entity->ReadKeyField(nameNoArray.c_str(), &variant);
                variable.Vector3D(vec);
                variable.Convert(FIELD_FLOAT);
                vec[vecOffset] = variable.Float();
                variable.SetVector3D(vec);
            }
            else {
                entity->KeyValue(nameNoArray.c_str(), variable.String());
            }
            found = false;
        }
        else if (type == DATAMAP) {
            auto &entry = GetDataMapOffset(entity->GetDataDescMap(), name);

            if (entry.offset > 0) {
                if (entry.fieldType == FIELD_CHARACTER && entry.size > 1) {
                    V_strncpy(((char*)entity) + entry.offset, variable.String(), entry.size);
                }
                else {
                    variable.Convert(entry.fieldType);
                    variable.SetOther(((char*)entity) + entry.offset);
                }
                found = true;
            }
        }
        else if (type == SENDPROP) {
            auto &entry = GetSendPropOffset(entity->GetServerClass(), name);

            if (entry.offset > 0) {

                int offset = entry.offset;
                auto propType = entry.prop->GetType();
                if (propType == DPT_Int) {
                    
                    if (entry.prop->m_nBits == 21 && (entry.prop->GetFlags() & SPROP_UNSIGNED)) {
                        variable.Convert(FIELD_EHANDLE);
                        *(CHandle<CBaseEntity>*)(((char*)entity) + offset) = variable.Entity();
                    }
                    else {
                        variable.Convert(FIELD_INTEGER);
                        auto proxyfn = entry.prop->GetProxyFn();
                        if (proxyfn == sendproxies->m_Int8ToInt32 || proxyfn == sendproxies->m_UInt8ToInt32) {
                            *(char*)(((char*)entity) + offset) = variable.Int();
                        }
                        else if (proxyfn == sendproxies->m_Int16ToInt32 || proxyfn == sendproxies->m_UInt16ToInt32) {
                            *(short*)(((short*)entity) + offset) = variable.Int();
                        }
                        else {
                            *(int*)(((char*)entity) + offset) = variable.Int();
                        }
                    }
                }
                else if (propType == DPT_Float || entry.isVecAxis) {
                    variable.Convert(FIELD_FLOAT);
                    *(float*)(((char*)entity) + offset) = variable.Float();
                }
                else if (propType == DPT_String) {
                    *(string_t*)(((char*)entity) + offset) = AllocPooledString(variable.String());
                }
                else if (propType == DPT_Vector) {
                    variable.Convert(FIELD_VECTOR);
                    Vector tmpVec;
                    variable.Vector3D(tmpVec);
                    *(Vector*)(((char*)entity) + offset) = tmpVec;
                }
                entity->NetworkStateChanged();
                found = true;
            }
        }
        return found;
    }

    bool GetEntityVariable(CBaseEntity *entity, GetInputType type, const char *name, variant_t &variable) {
        bool found = false;

        if (type == ANY) {
            found = GetEntityVariable(entity, VARIABLE, name, variable) ||
                    GetEntityVariable(entity, DATAMAP, name, variable) ||
                    GetEntityVariable(entity, SENDPROP, name, variable) ||
                    GetEntityVariable(entity, KEYVALUE, name, variable);
        }
        else if (type == VARIABLE) {
            std::string nameNoArray = name;
            int vecOffset;
            ReadVectorIndexFromString(nameNoArray, vecOffset);
            if (entity->GetCustomVariableByText(nameNoArray.c_str(), variable)) {
                found = true;
                ConvertToVectorIndex(variable, vecOffset);
            }
        }
        else if (type == KEYVALUE) {
            if (name[0] == '$') {
                return GetEntityVariable(entity, VARIABLE, name + 1, variable);
            }
            std::string nameNoArray = name;
            int vecOffset;
            ReadVectorIndexFromString(nameNoArray, vecOffset);
            found = entity->ReadKeyField(nameNoArray.c_str(), &variable);
            ConvertToVectorIndex(variable, vecOffset);
        }
        else if (type == DATAMAP) {
            auto &entry = GetDataMapOffset(entity->GetDataDescMap(), name);
            if (entry.offset > 0) {
                if (entry.fieldType == FIELD_CHARACTER && entry.size > 1) {
                    variable.SetString(AllocPooledString(((char*)entity) + entry.offset));
                }
                else {
                    variable.Set(entry.fieldType, ((char*)entity) + entry.offset);
                }
                if (entry.fieldType == FIELD_POSITION_VECTOR) {
                    variable.Convert(FIELD_VECTOR);
                }
                else if (entry.fieldType == FIELD_CLASSPTR) {
                    variable.Convert(FIELD_EHANDLE);
                }
                else if ((entry.fieldType == FIELD_CHARACTER && entry.size == 1) || (entry.fieldType == FIELD_SHORT)) {
                    variable.Convert(FIELD_INTEGER);
                }
                
                found = true;
            }
        }
        else if (type == SENDPROP) {
            auto &entry = GetSendPropOffset(entity->GetServerClass(), name);

            if (entry.offset > 0) {
                int offset = entry.offset;
                auto propType = entry.prop->GetType();
                if (propType == DPT_Int) {
                    if (entry.prop->m_nBits == 21 && (entry.prop->GetFlags() & SPROP_UNSIGNED)) {
                        variable.Set(FIELD_EHANDLE, (CHandle<CBaseEntity>*)(((char*)entity) + offset));
                    }
                    else {
                        auto proxyfn = entry.prop->GetProxyFn();
                        if (proxyfn == sendproxies->m_Int8ToInt32 || proxyfn == sendproxies->m_UInt8ToInt32) {
                            variable.SetInt(*(char*)(((char*)entity) + offset));
                        }
                        else if (proxyfn == sendproxies->m_Int16ToInt32 || proxyfn == sendproxies->m_UInt16ToInt32) {
                            variable.SetInt(*(short*)(((char*)entity) + offset));
                        }
                        else {
                            variable.SetInt(*(int*)(((char*)entity) + offset));
                        }
                    }
                }
                else if (propType == DPT_Float || entry.isVecAxis) {
                    variable.SetFloat(*(float*)(((char*)entity) + offset));
                }
                else if (propType == DPT_String) {
                    variable.SetString(*(string_t*)(((char*)entity) + offset));
                }
                else if (propType == DPT_Vector) {
                    variable.SetVector3D(*(Vector*)(((char*)entity) + offset));
                }
                found = true;
            }
        }
        return found;
    }

    void FireGetInput(CBaseEntity *entity, GetInputType type, const char *name, CBaseEntity *activator, CBaseEntity *caller, variant_t &value) {
        char param_tokenized[256] = "";
        V_strncpy(param_tokenized, value.String(), sizeof(param_tokenized));
        char *targetstr = strtok(param_tokenized,"|");
        char *action = strtok(NULL,"|");
        char *defvalue = strtok(NULL,"|");
        
        variant_t variable;

        if (targetstr != nullptr && action != nullptr) {

            bool found = GetEntityVariable(entity, type, name, variable);

            if (!found && defvalue != nullptr) {
                variable.SetString(AllocPooledString(defvalue));
            }

            if (found || defvalue != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, entity, activator, caller)) != nullptr ;) {
                    target->AcceptInput(action, activator, entity, variable, 0);
                }
            }
        }
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_InputIgnitePlayer, inputdata_t &inputdata)
    {
        CTFPlayer *activator = inputdata.pActivator != nullptr && inputdata.pActivator->IsPlayer() ? ToTFPlayer(inputdata.pActivator) : reinterpret_cast<CTFPlayer *>(this);
        reinterpret_cast<CTFPlayer *>(this)->m_Shared->Burn(activator, nullptr, 10.0f);
    }

    THINK_FUNC_DECL(RotatingFollowEntity)
    {
        auto rotating = reinterpret_cast<CFuncRotating *>(this);
        auto data = GetExtraFuncRotatingData(rotating);
        if (data->m_hRotateTarget == nullptr)
            return;

        auto lookat = rotating->GetCustomVariable<"lookat">();
        Vector targetVec;
        if (FStrEq(lookat, PStr<"origin">())) {
            targetVec = data->m_hRotateTarget->GetAbsOrigin();
        } 
        else if (FStrEq(lookat, PStr<"center">())) {
            targetVec = data->m_hRotateTarget->WorldSpaceCenter();
        } 
        else {
            targetVec = data->m_hRotateTarget->EyePosition();
        }

        targetVec -= rotating->GetAbsOrigin();
        targetVec += data->m_hRotateTarget->GetAbsVelocity() * gpGlobals->frametime;
        float projectileSpeed = rotating->GetCustomVariableFloat<"projectilespeed">();
        if (projectileSpeed != 0) {
            targetVec += (targetVec.Length() / projectileSpeed) * data->m_hRotateTarget->GetAbsVelocity();
        }

        QAngle angToTarget;
	    VectorAngles(targetVec, angToTarget);

        float aimOffset = rotating->GetCustomVariableFloat<"aimoffset">();
        if (aimOffset != 0) {
            angToTarget.x -= Vector(targetVec.x, targetVec.y, 0.0f).Length() * aimOffset;
        }

        float angleDiff;
        float angle = 0;
        QAngle moveAngle = rotating->m_vecMoveAng;
        QAngle angles = rotating->GetAbsAngles();

        float limit = rotating->GetCustomVariableFloat<"limit">();
        if (limit != 0.0f) {
            angToTarget.x = clamp(AngleNormalize(angToTarget.x), -limit, limit);
            angToTarget.y = clamp(AngleNormalize(angToTarget.y), -limit, limit);
        }
        if (moveAngle == QAngle(1,0,0)) {
            angleDiff = AngleDiff(angles.x, angToTarget.x);
            angle = rotating->GetLocalAngles().x;
        }
        else {
            angleDiff = AngleDiff(angles.y, angToTarget.y);
            angle = rotating->GetLocalAngles().y;
        }

        float speed = rotating->m_flMaxSpeed;
        if (abs(angleDiff) < rotating->m_flMaxSpeed/66) {
            speed = 0;
            if (moveAngle == QAngle(1,0,0)) {
                rotating->SetAbsAngles(QAngle(angToTarget.x, angles.y, angles.z));
            }
            else {
                rotating->SetAbsAngles(QAngle(angles.x, angToTarget.y, angles.z));
            }
        }

        if (angleDiff > 0) {
            speed *= -1.0f;
        }

        if (speed != rotating->m_flTargetSpeed) {
            rotating->m_bReversed = angleDiff > 0;
            rotating->SetTargetSpeed(abs(speed));
        }
        
        rotating->SetNextThink(gpGlobals->curtime + 0.01f, "RotatingFollowEntity");
    }

    class RotatorModule : public EntityModule
    {
    public:
        CHandle<CBaseEntity> m_hRotateTarget;
    };

    THINK_FUNC_DECL(RotatorModuleTick)
    {
        auto data = this->GetEntityModule<RotatorModule>("rotator");
        if (data == nullptr || data->m_hRotateTarget == nullptr)
            return;

        auto lookat = this->GetCustomVariable<"lookat">("eyes");
        Vector targetVec;
        auto aimEntity = data->m_hRotateTarget;
        if (FStrEq(lookat, PStr<"origin">())) {
            targetVec = data->m_hRotateTarget->GetAbsOrigin();
        } 
        else if (FStrEq(lookat, PStr<"center">())) {
            targetVec = data->m_hRotateTarget->WorldSpaceCenter();
        } 
        else if (FStrEq(lookat, PStr<"aim">())) {
            Vector fwd;
            Vector dest;
            AngleVectors(data->m_hRotateTarget->EyeAngles(), &fwd);
            VectorMA(data->m_hRotateTarget->EyePosition(), 8192.0f, fwd, dest);
            trace_t tr;
            UTIL_TraceLine(data->m_hRotateTarget->EyePosition(), dest, MASK_SHOT, data->m_hRotateTarget, COLLISION_GROUP_NONE, &tr);

            targetVec = tr.endpos;
            aimEntity = tr.m_pEnt;
        } 
        else {
            targetVec = data->m_hRotateTarget->EyePosition();
        }

        targetVec -= this->GetAbsOrigin();
        if (aimEntity != nullptr) {
            targetVec += aimEntity->GetAbsVelocity() * gpGlobals->frametime;
        }
        float projectileSpeed = this->GetCustomVariableFloat<"projectilespeed">();
        if (projectileSpeed != 0) {
            targetVec += (targetVec.Length() / projectileSpeed) * data->m_hRotateTarget->GetAbsVelocity();
        }

        QAngle angToTarget;
	    VectorAngles(targetVec, angToTarget);

        float aimOffset = this->GetCustomVariableFloat<"aimoffset">();
        if (aimOffset != 0) {
            angToTarget.x -= Vector(targetVec.x, targetVec.y, 0.0f).Length() * aimOffset;
        }

        QAngle angles = this->GetAbsAngles();

        bool velocitymode = this->GetCustomVariableFloat<"velocitymode">();
        float speedx = this->GetCustomVariableFloat<"rotationspeedx">();
        float speedy = this->GetCustomVariableFloat<"rotationspeedy">();
        if (!velocitymode) {
            speedx *= gpGlobals->frametime;
            speedy *= gpGlobals->frametime;
        }
        angToTarget.x = ApproachAngle(angToTarget.x, angles.x, speedx);
        angToTarget.y = ApproachAngle(angToTarget.y, angles.y, speedy);

        float limitx = this->GetCustomVariableFloat<"rotationlimitx">();
        if (limitx != 0.0f) {
            angToTarget.x = clamp(AngleNormalize(angToTarget.x), -limitx, limitx);
        }
        
        float limity = this->GetCustomVariableFloat<"rotationlimity">();
        if (limity != 0.0f) {
            angToTarget.y = clamp(AngleNormalize(angToTarget.y), -limity, limity);
        }

        if (!velocitymode) {
            this->SetAbsAngles(QAngle(angToTarget.x, angToTarget.y, angles.z));
        }
        else {
            angToTarget = QAngle(angToTarget.x - angles.x, angToTarget.y - angles.y, 0.0f);
            this->SetLocalAngularVelocity(angToTarget);
        }
        
        this->SetNextThink(gpGlobals->curtime + 0.01f, "RotatorModuleTick");
    }

    class ForwardVelocityModule : public EntityModule
    {
    public:
        ForwardVelocityModule(CBaseEntity *entity);
    };

    THINK_FUNC_DECL(ForwardVelocityTick)
    {
        if (this->GetEntityModule<ForwardVelocityModule>("forwardvelocity") == nullptr)
            return;
            
        Vector fwd;
        AngleVectors(this->GetAbsAngles(), &fwd);
        
        fwd *= this->GetCustomVariableFloat<"forwardspeed">();

        if (this->GetCustomVariableFloat<"directmode">() != 0) {
            this->SetAbsOrigin(this->GetAbsOrigin() + fwd * gpGlobals->frametime);
        }
        else {
            IPhysicsObject *pPhysicsObject = this->VPhysicsGetObject();
            if (pPhysicsObject) {
                pPhysicsObject->SetVelocity(&fwd, nullptr);
            }
            else {
                this->SetAbsVelocity(fwd);
            }
        }
        this->SetNextThink(gpGlobals->curtime + 0.01f, "ForwardVelocityTick");
    }

    ForwardVelocityModule::ForwardVelocityModule(CBaseEntity *entity)
    {
        if (entity->GetNextThink("ForwardVelocityTick") < gpGlobals->curtime) {
            THINK_FUNC_SET(entity, ForwardVelocityTick, gpGlobals->curtime + 0.01);
        }
    }

    class DroppedWeaponModule : public EntityModule
    {
    public:
        DroppedWeaponModule(CBaseEntity *entity) : EntityModule(entity) {}

        CHandle<CBaseEntity> m_hWeaponSpawner;
        int ammo = -1;
        int clip = -1;
        float energy = FLT_MIN;
        float charge = FLT_MIN;
    };

    class FakeParentModule : public EntityModule
    {
    public:
        FakeParentModule(CBaseEntity *entity) : EntityModule(entity) {}
        CHandle<CBaseEntity> m_hParent;
        bool m_bParentSet = false;
    };

    class MathVectorModule : public EntityModule
    {
    public:
        MathVectorModule(CBaseEntity *entity) : EntityModule(entity) {}
        union {
            Vector value;
            QAngle valueAng;
        };
    };

    THINK_FUNC_DECL(FakeParentModuleTick)
    {
        auto data = this->GetEntityModule<FakeParentModule>("fakeparent");
        if (data == nullptr || data->m_hParent == nullptr) return;

        if (data->m_hParent == nullptr && data->m_bParentSet) {
            variant_t variant;
            this->FireCustomOutput<"onfakeparentkilled">(this, this, variant);
            data->m_bParentSet = false;
        }

        if (data->m_hParent == nullptr) return;

        Vector pos;
        QAngle ang;
        CBaseEntity *parent =data->m_hParent;

        auto bone = this->GetCustomVariable<"bone">();
        auto attachment = this->GetCustomVariable<"attachment">();
        bool posonly = this->GetCustomVariableFloat<"positiononly">();
        bool rotationonly = this->GetCustomVariableFloat<"rotationonly">();
        Vector offset = this->GetCustomVariableVector<"fakeparentoffset">();
        QAngle offsetangle = this->GetCustomVariableAngle<"fakeparentrotation">();
        matrix3x4_t transform;

        if (bone != nullptr) {
            CBaseAnimating *anim = rtti_cast<CBaseAnimating *>(parent);
            anim->GetBoneTransform(anim->LookupBone(bone), transform);
        }
        else if (attachment != nullptr){
            CBaseAnimating *anim = rtti_cast<CBaseAnimating *>(parent);
            anim->GetAttachment(anim->LookupAttachment(attachment), transform);
        }
        else{
            transform = parent->EntityToWorldTransform();
        }

        if (!rotationonly) {
            VectorTransform(offset, transform, pos);
            this->SetAbsOrigin(pos);
        }

        if (!posonly) {
            MatrixAngles(transform, ang);
            ang += offsetangle;
            this->SetAbsAngles(ang);
        }

        this->SetNextThink(gpGlobals->curtime + 0.01f, "FakeParentModuleTick");
    }

    class AimFollowModule : public EntityModule
    {
    public:
        AimFollowModule(CBaseEntity *entity) : EntityModule(entity) {}
        CHandle<CBaseEntity> m_hParent;
    };

    THINK_FUNC_DECL(AimFollowModuleTick)
    {
        auto data = this->GetEntityModule<AimFollowModule>("aimfollow");
        if (data == nullptr || data->m_hParent == nullptr) return;

        
        Vector fwd;
        Vector dest;
        AngleVectors(data->m_hParent->EyeAngles(), &fwd);
        VectorMA(data->m_hParent->EyePosition(), 8192.0f, fwd, dest);
        trace_t tr;
        CTraceFilterSkipTwoEntities filter(data->m_hParent, this, COLLISION_GROUP_NONE);
        UTIL_TraceLine(data->m_hParent->EyePosition(), dest, MASK_SHOT, &filter, &tr);

        Vector targetVec = tr.endpos;
        
        this->SetAbsOrigin(targetVec);
        bool rotationfollow = this->GetCustomVariableFloat<"rotationfollow">();
        if (rotationfollow) {
            this->SetAbsAngles(data->m_hParent->EyeAngles());
        }
        this->SetNextThink(gpGlobals->curtime + 0.01f, "AimFollowModuleTick");
    }

    void AddModuleByName(CBaseEntity *entity, const char *name)
    {
        if (FStrEq(name, "rotator")) {
            entity->AddEntityModule("rotator", new RotatorModule());
        }
        else if (FStrEq(name, "forwardvelocity")) {
            entity->AddEntityModule("forwardvelocity", new ForwardVelocityModule(entity));
        }
        else if (FStrEq(name, "fakeparent")) {
            entity->AddEntityModule("fakeparent", new FakeParentModule(entity));
        }
        else if (FStrEq(name, "aimfollow")) {
            entity->AddEntityModule("aimfollow", new AimFollowModule(entity));
        }
    }

    PooledString logic_case_classname("logic_case");
    PooledString tf_gamerules_classname("tf_gamerules");
    PooledString player_classname("player");
    PooledString point_viewcontrol_classname("point_viewcontrol");
    PooledString weapon_spawner_classname("$weapon_spawner");

    bool allow_create_dropped_weapon = false;
    bool HandleCustomInput(CBaseEntity *ent, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
    {
        if (ent->GetClassname() == logic_case_classname) {
            CLogicCase *logic_case = static_cast<CLogicCase *>(ent);
            if (strnicmp(szInputName, "FormatInput", strlen("$FormatInput")) == 0) {
                int num = strtol(szInputName + strlen("$FormatInput"), nullptr, 10);
                if (num > 0 && num < 16) {
                    logic_case->m_nCase[num - 1] = AllocPooledString(Value.String());
                    FireFormatInput(logic_case, pActivator, pCaller);
                    
                    return true;
                }
            }
            else if (strnicmp(szInputName, "FormatInputNoFire", strlen("FormatInputNoFire")) == 0) {
                int num = strtol(szInputName + strlen("FormatInputNoFire"), nullptr, 10);
                if (num > 0 && num < 16) {
                    logic_case->m_nCase[num - 1] = AllocPooledString(Value.String());
                    return true;
                }
            }
            else if (FStrEq(szInputName, "FormatString")) {
                logic_case->m_nCase[15] = AllocPooledString(Value.String());
                FireFormatInput(logic_case, pActivator, pCaller);
                return true;
            }
            else if (FStrEq(szInputName, "FormatStringNoFire")) {
                logic_case->m_nCase[15] = AllocPooledString(Value.String());
                return true;
            }
            else if (FStrEq(szInputName, "Format")) {
                FireFormatInput(logic_case, pActivator, pCaller);
                return true;
            }
            else if (FStrEq(szInputName, "TestSigsegv")) {
                ent->m_OnUser1->FireOutput(Value, pActivator, pCaller);
                return true;
            }
            else if (FStrEq(szInputName, "ToInt")) {
                variant_t convert;
                convert.SetInt(strtol(Value.String(), nullptr, 10));
                logic_case->m_OnDefault->FireOutput(convert, pActivator, ent);
                return true;
            }
            else if (FStrEq(szInputName, "ToFloat")) {
                variant_t convert;
                convert.SetFloat(strtof(Value.String(), nullptr));
                logic_case->m_OnDefault->FireOutput(convert, pActivator, ent);
                return true;
            }
            else if (FStrEq(szInputName, "CallerToActivator")) {
                logic_case->m_OnDefault->FireOutput(Value, pCaller, ent);
                return true;
            }
            else if (FStrEq(szInputName, "GetKeyValueFromActivator")) {
                variant_t variant;
                pActivator->ReadKeyField(Value.String(), &variant);
                logic_case->m_OnDefault->FireOutput(variant, pActivator, ent);
                return true;
            }
            else if (FStrEq(szInputName, "GetConVar")) {
                ConVarRef cvref(Value.String());
                if (cvref.IsValid() && cvref.IsFlagSet(FCVAR_REPLICATED) && !cvref.IsFlagSet(FCVAR_PROTECTED)) {
                    variant_t variant;
                    variant.SetFloat(cvref.GetFloat());
                    logic_case->m_OnDefault->FireOutput(variant, pActivator, ent);
                }
                return true;
            }
            else if (FStrEq(szInputName, "GetConVarString")) {
                ConVarRef cvref(Value.String());
                if (cvref.IsValid() && cvref.IsFlagSet(FCVAR_REPLICATED) && !cvref.IsFlagSet(FCVAR_PROTECTED)) {
                    variant_t variant;
                    variant.SetString(AllocPooledString(cvref.GetString()));
                    logic_case->m_OnDefault->FireOutput(variant, pActivator, ent);
                }
                return true;
            }
            else if (FStrEq(szInputName, "DisplayMenu")) {
                
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                    if (target != nullptr && target->IsPlayer() && !ToTFPlayer(target)->IsBot()) {
                        CaseMenuHandler *handler = new CaseMenuHandler(ToTFPlayer(target), logic_case);
                        IBaseMenu *menu = menus->GetDefaultStyle()->CreateMenu(handler);

                        int i;
                        for (i = 1; i < 16; i++) {
                            string_t str = logic_case->m_nCase[i - 1];
                            const char *name = STRING(str);
                            if (strlen(name) != 0) {
                                bool enabled = name[0] != '!';
                                ItemDrawInfo info1(enabled ? name : name + 1, enabled ? ITEMDRAW_DEFAULT : ITEMDRAW_DISABLED);
                                menu->AppendItem("it", info1);
                            }
                            else {
                                break;
                            }
                        }
                        if (i < 11) {
                            menu->SetPagination(MENU_NO_PAGINATION);
                        }

                        variant_t variant;
                        ent->ReadKeyField("Case16", &variant);
                        
                        char param_tokenized[256];
                        V_strncpy(param_tokenized, variant.String(), sizeof(param_tokenized));
                        
                        char *name = strtok(param_tokenized,"|");
                        char *timeout = strtok(NULL,"|");

                        menu->SetDefaultTitle(name);

                        char *flag;
                        while ((flag = strtok(NULL,"|")) != nullptr) {
                            if (FStrEq(flag, "Cancel")) {
                                menu->SetMenuOptionFlags(menu->GetMenuOptionFlags() | MENUFLAG_BUTTON_EXIT);
                            }
                        }

                        menu->Display(ENTINDEX(target), timeout == nullptr ? 0 : atoi(timeout));
                    }
                }
                return true;
            }
            else if (FStrEq(szInputName, "HideMenu")) {
                auto target = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
                if (target != nullptr && target->IsPlayer()) {
                    menus->GetDefaultStyle()->CancelClientMenu(ENTINDEX(target), false);
                }
            }
            else if (FStrEq(szInputName, "BitTest")) {
                Value.Convert(FIELD_INTEGER);
                int val = Value.Int();
                for (int i = 1; i <= 16; i++) {
                    string_t str = logic_case->m_nCase[i - 1];

                    if (val & atoi(STRING(str))) {
                        logic_case->FireCase(i, pActivator);
                    }
                    else {
                        break;
                    }
                }
            }
            else if (FStrEq(szInputName, "BitTestAll")) {
                Value.Convert(FIELD_INTEGER);
                int val = Value.Int();
                for (int i = 1; i <= 16; i++) {
                    string_t str = logic_case->m_nCase[i - 1];

                    int test = atoi(STRING(str));
                    if ((val & test) == test) {
                        logic_case->FireCase(i, pActivator);
                    }
                    else {
                        break;
                    }
                }
            }
        }
        else if (ent->GetClassname() == tf_gamerules_classname) {
            if (stricmp(szInputName, "StopVO") == 0) {
                TFGameRules()->BroadcastSound(SOUND_FROM_LOCAL_PLAYER, Value.String(), SND_STOP);
                return true;
            }
            else if (stricmp(szInputName, "StopVORed") == 0) {
                TFGameRules()->BroadcastSound(TF_TEAM_RED, Value.String(), SND_STOP);
                return true;
            }
            else if (stricmp(szInputName, "StopVOBlue") == 0) {
                TFGameRules()->BroadcastSound(TF_TEAM_BLUE, Value.String(), SND_STOP);
                return true;
            }
            else if (stricmp(szInputName, "SetBossHealthPercentage") == 0) {
                Value.Convert(FIELD_FLOAT);
                float val = Value.Float();
                if (g_pMonsterResource.GetRef() != nullptr)
                    g_pMonsterResource->m_iBossHealthPercentageByte = (int) (val * 255.0f);
                return true;
            }
            else if (stricmp(szInputName, "SetBossState") == 0) {
                Value.Convert(FIELD_INTEGER);
                int val = Value.Int();
                if (g_pMonsterResource.GetRef() != nullptr)
                    g_pMonsterResource->m_iBossState = val;
                return true;
            }
            else if (stricmp(szInputName, "AddCurrencyGlobal") == 0) {
                Value.Convert(FIELD_INTEGER);
                int val = atoi(Value.Int());
                TFGameRules()->DistributeCurrencyAmount(val, nullptr, true, true, false);
                return true;
            }
        }
        else if (ent->GetClassname() == player_classname) {
            if (stricmp(szInputName, "AllowClassAnimations") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr) {
                    player->GetPlayerClass()->m_bUseClassAnimations = Value.Bool();
                }
                return true;
            }
            else if (stricmp(szInputName, "SwitchClass") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                
                if (player != nullptr) {
                    // Disable setup to allow class changing during waves in mvm
                    static ConVarRef endless("tf_mvm_endless_force_on");
                    endless.SetValue(true);

                    int index = strtol(Value.String(), nullptr, 10);
                    if (index > 0 && index < 10) {
                        player->HandleCommand_JoinClass(g_aRawPlayerClassNames[index]);
                    }
                    else {
                        player->HandleCommand_JoinClass(Value.String());
                    }
                    endless.SetValue(false);
                }
                return true;
            }
            else if (stricmp(szInputName, "SwitchClassInPlace") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                
                if (player != nullptr) {
                    // Disable setup to allow class changing during waves in mvm
                    static ConVarRef endless("tf_mvm_endless_force_on");
                    endless.SetValue(true);

                    Vector pos = player->GetAbsOrigin();
                    QAngle ang = player->GetAbsAngles();
                    Vector vel = player->GetAbsVelocity();

                    int index = strtol(Value.String(), nullptr, 10);
                    int oldState = player->m_Shared->GetState();
                    player->m_Shared->SetState(TF_STATE_DYING);
                    if (index > 0 && index < 10) {
                        player->HandleCommand_JoinClass(g_aRawPlayerClassNames[index]);
                    }
                    else {
                        player->HandleCommand_JoinClass(Value.String());
                    }
                    player->m_Shared->SetState(oldState);
                    endless.SetValue(false);
                    player->ForceRespawn();
                    player->Teleport(&pos, &ang, &vel);
                    
                }
                return true;
            }
            else if (stricmp(szInputName, "ForceRespawn") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr) {
                    if (player->GetTeamNumber() >= TF_TEAM_RED && player->GetPlayerClass() != nullptr && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED) {
                        player->ForceRespawn();
                    }
                    else {
                        player->m_bAllowInstantSpawn = true;
                    }
                }
                return true;
            }
            else if (stricmp(szInputName, "ForceRespawnDead") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr && !player->IsAlive()) {
                    if (player->GetTeamNumber() >= TF_TEAM_RED && player->GetPlayerClass() != nullptr && player->GetPlayerClass()->GetClassIndex() != TF_CLASS_UNDEFINED) {
                        player->ForceRespawn();
                    }
                    else {
                        player->m_bAllowInstantSpawn = true;
                    }
                }
                return true;
            }
            else if (stricmp(szInputName, "DisplayTextCenter") == 0) {
                using namespace std::string_literals;
                std::string text{Value.String()};
                text = std::regex_replace(text, std::regex{"\\{newline\\}", std::regex_constants::icase}, "\n");
                text = std::regex_replace(text, std::regex{"\\{player\\}", std::regex_constants::icase}, ToTFPlayer(ent)->GetPlayerName());
                text = std::regex_replace(text, std::regex{"\\{activator\\}", std::regex_constants::icase}, 
                        (pActivator != nullptr && pActivator->IsPlayer() ? ToTFPlayer(pActivator) : ToTFPlayer(ent))->GetPlayerName());
                gamehelpers->TextMsg(ENTINDEX(ent), TEXTMSG_DEST_CENTER, text.c_str());
                return true;
            }
            else if (stricmp(szInputName, "DisplayTextChat") == 0) {
                using namespace std::string_literals;
                std::string text{"\x01"s + Value.String() + "\x01"s};
                text = std::regex_replace(text, std::regex{"\\{reset\\}", std::regex_constants::icase}, "\x01");
                text = std::regex_replace(text, std::regex{"\\{blue\\}", std::regex_constants::icase}, "\x07" "99ccff");
                text = std::regex_replace(text, std::regex{"\\{red\\}", std::regex_constants::icase}, "\x07" "ff3f3f");
                text = std::regex_replace(text, std::regex{"\\{green\\}", std::regex_constants::icase}, "\x07" "99ff99");
                text = std::regex_replace(text, std::regex{"\\{darkgreen\\}", std::regex_constants::icase}, "\x07" "40ff40");
                text = std::regex_replace(text, std::regex{"\\{yellow\\}", std::regex_constants::icase}, "\x07" "ffb200");
                text = std::regex_replace(text, std::regex{"\\{grey\\}", std::regex_constants::icase}, "\x07" "cccccc");
                text = std::regex_replace(text, std::regex{"\\{newline\\}", std::regex_constants::icase}, "\n");
                text = std::regex_replace(text, std::regex{"\\{player\\}", std::regex_constants::icase}, ToTFPlayer(ent)->GetPlayerName());
                text = std::regex_replace(text, std::regex{"\\{activator\\}", std::regex_constants::icase}, 
                        (pActivator != nullptr && pActivator->IsPlayer() ? ToTFPlayer(pActivator) : ToTFPlayer(ent))->GetPlayerName());
                auto pos{text.find("{")};
                while(pos != std::string::npos){
                    if(text.substr(pos).length() > 7){
                        text[pos] = '\x07';
                        text.erase(pos+7, 1);
                        pos = text.find("{");
                    } else break; 
                }
                gamehelpers->TextMsg(ENTINDEX(ent), TEXTMSG_DEST_CHAT , text.c_str());
                return true;
            }
            else if (stricmp(szInputName, "Suicide") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr) {
                    player->CommitSuicide(false, true);
                }
                return true;
            }
            else if (stricmp(szInputName, "ChangeAttributes") == 0) {
                CTFBot *bot = ToTFBot(ent);
                if (bot != nullptr) {
                    auto *attrib = bot->GetEventChangeAttributes(Value.String());
                    if (attrib != nullptr){
                        bot->OnEventChangeAttributes(attrib);
                    }
                }
                return true;
            }
            else if (stricmp(szInputName, "RollCommonSpell") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                CBaseEntity *weapon = player->GetEntityForLoadoutSlot(LOADOUT_POSITION_ACTION);
                
                if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) return true;

                CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
                spellbook->RollNewSpell(0);

                return true;
            }
            else if (stricmp(szInputName, "SetSpell") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                CBaseEntity *weapon = player->GetEntityForLoadoutSlot(LOADOUT_POSITION_ACTION);
                
                if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) return true;
                
                const char *str = Value.String();
                int index = strtol(str, nullptr, 10);
                for (int i = 0; i < ARRAYSIZE(SPELL_TYPE); i++) {
                    if (FStrEq(str, SPELL_TYPE[i])) {
                        index = i;
                    }
                }

                CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
                spellbook->m_iSelectedSpellIndex = index;
                spellbook->m_iSpellCharges = 1;

                return true;
            }
            else if (stricmp(szInputName, "AddSpell") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                
                CBaseEntity *weapon = player->GetEntityForLoadoutSlot(LOADOUT_POSITION_ACTION);
                
                if (weapon == nullptr || !FStrEq(weapon->GetClassname(), "tf_weapon_spellbook")) return true;
                
                const char *str = Value.String();
                int index = strtol(str, nullptr, 10);
                for (int i = 0; i < ARRAYSIZE(SPELL_TYPE); i++) {
                    if (FStrEq(str, SPELL_TYPE[i])) {
                        index = i;
                    }
                }

                CTFSpellBook *spellbook = rtti_cast<CTFSpellBook *>(weapon);
                if (spellbook->m_iSelectedSpellIndex != index) {
                    spellbook->m_iSpellCharges = 0;
                }
                spellbook->m_iSelectedSpellIndex = index;
                spellbook->m_iSpellCharges += 1;

                return true;
            }
            else if (stricmp(szInputName, "AddCond") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                int index = 0;
                float duration = -1.0f;
                sscanf(Value.String(), "%d %f", &index, &duration);
                if (player != nullptr) {
                    player->m_Shared->AddCond((ETFCond)index, duration);
                }
                return true;
            }
            else if (stricmp(szInputName, "RemoveCond") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                int index = strtol(Value.String(), nullptr, 10);
                if (player != nullptr) {
                    player->m_Shared->RemoveCond((ETFCond)index);
                }
                return true;
            }
            else if (stricmp(szInputName, "AddPlayerAttribute") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                char param_tokenized[256];
                V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                
                char *attr = strtok(param_tokenized,"|");
                char *value = strtok(NULL,"|");

                if (player != nullptr) {
                    player->AddCustomAttribute(attr, atof(value), -1.0f);
                }
                return true;
            }
            else if (stricmp(szInputName, "RemovePlayerAttribute") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr) {
                    player->RemoveCustomAttribute(Value.String());
                }
                return true;
            }
            else if (stricmp(szInputName, "GetPlayerAttribute") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr) {
                    char param_tokenized[256] = "";
                    V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                    char *attrName = strtok(param_tokenized,"|");
                    char *targetstr = strtok(NULL,"|");
                    char *action = strtok(NULL,"|");
                    char *defvalue = strtok(NULL,"|");
                    CEconItemAttribute * attr = player->GetAttributeList()->GetAttributeByName(attrName);
                    variant_t variable;
                    bool found = false;
                    if (attr != nullptr) {
                        char buf[256];
                        attr->GetStaticData()->ConvertValueToString(*attr->GetValuePtr(), buf, sizeof(buf));
                        variable.SetString(AllocPooledString(buf));
                        found = true;
                    }
                    else {
                        variable.SetString(AllocPooledString(defvalue));
                    }

                    if (targetstr != nullptr && action != nullptr) {
                        if (found || defvalue != nullptr) {
                            for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, ent, pActivator, pCaller)) != nullptr ;) {
                                target->AcceptInput(action, pActivator, ent, variable, 0);
                            }
                        }
                    }
                }
                return true;
            }
            else if (stricmp(szInputName, "GetItemAttribute") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                if (player != nullptr) {
                    char param_tokenized[256] = "";
                    V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                    char *itemSlot = strtok(param_tokenized,"|");
                    char *attrName = strtok(NULL,"|");
                    char *targetstr = strtok(NULL,"|");
                    char *action = strtok(NULL,"|");
                    char *defvalue = strtok(NULL,"|");

                    bool found = false;

                    variant_t variable;
                    if (itemSlot != nullptr) {
                        int slot = 0;
                        CEconEntity *item = nullptr;
                        if (StringToIntStrict(itemSlot, slot)) {
                            if (slot != -1) {
                                item = GetEconEntityAtLoadoutSlot(player, slot);
                            }
                            else {
                                item = player->GetActiveTFWeapon();
                            }
                        }
                        else {
                            ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                                if (entity->GetItem() != nullptr && FStrEq(entity->GetItem()->GetItemDefinition()->GetName(), itemSlot)) {
                                    item = entity;
                                }
                            });
                        }
                        
                        if (item != nullptr) {
                            CEconItemAttribute * attr = player->GetAttributeList()->GetAttributeByName(attrName);
                            if (attr != nullptr) {
                                char buf[256];
                                attr->GetStaticData()->ConvertValueToString(*attr->GetValuePtr(), buf, sizeof(buf));
                                variable.SetString(AllocPooledString(buf));
                                found = true;
                            }
                        }
                    }

                    if (!found) {
                        variable.SetString(AllocPooledString(defvalue));
                    }

                    if (targetstr != nullptr && action != nullptr) {
                        if (found || defvalue != nullptr) {
                            for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, ent, pActivator, pCaller)) != nullptr ;) {
                                target->AcceptInput(action, pActivator, ent, variable, 0);
                            }
                        }
                    }
                }
                return true;
            }
            else if (strnicmp(szInputName, "GetKey$", strlen("GetKey$")) == 0) {
                FireGetInput(ent, KEYVALUE, szInputName + strlen("GetKey$"), pActivator, pCaller, Value);
                return true;
            }
            else if (stricmp(szInputName, "AddItemAttribute") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                char param_tokenized[256];
                V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                
                char *attr = strtok(param_tokenized,"|");
                char *value = strtok(NULL,"|");
                char *slot = strtok(NULL,"|");

                if (player != nullptr) {
                    CEconEntity *item = nullptr;
                    if (slot != nullptr) {
                        ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                            if (entity->GetItem() != nullptr && FStrEq(entity->GetItem()->GetItemDefinition()->GetName(), slot)) {
                                item = entity;
                            }
                        });
                        if (item == nullptr)
                            item = GetEconEntityAtLoadoutSlot(player, atoi(slot));
                    }
                    else {
                        item = player->GetActiveTFWeapon();
                    }
                    if (item != nullptr) {
                        CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(attr);
                        if (attr_def == nullptr) {
                            int idx = -1;
                            if (StringToIntStrict(attr, idx)) {
                                attr_def = GetItemSchema()->GetAttributeDefinition(idx);
                            }
                        }
                        item->GetItem()->GetAttributeList().AddStringAttribute(attr_def, value);

                    }
                }
                return true;
            }
            else if (stricmp(szInputName, "RemoveItemAttribute") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                char param_tokenized[256];
                V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                
                char *attr = strtok(param_tokenized,"|");
                char *slot = strtok(NULL,"|");

                if (player != nullptr) {
                    CEconEntity *item = nullptr;
                    if (slot != nullptr) {
                        ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                            if (entity->GetItem() != nullptr && FStrEq(entity->GetItem()->GetItemDefinition()->GetName(), Value.String())) {
                                item = entity;
                            }
                        });
                        if (item == nullptr)
                            item = GetEconEntityAtLoadoutSlot(player, atoi(slot));
                    }
                    else {
                        item = player->GetActiveTFWeapon();
                    }
                    if (item != nullptr) {
                        CEconItemAttributeDefinition *attr_def = GetItemSchema()->GetAttributeDefinitionByName(attr);
                        if (attr_def == nullptr) {
                            int idx = -1;
                            if (StringToIntStrict(attr, idx)) {
                                attr_def = GetItemSchema()->GetAttributeDefinition(idx);
                            }
                        }
                        item->GetItem()->GetAttributeList().RemoveAttribute(attr_def);

                    }
                }
                return true;
            }
            else if (stricmp(szInputName, "PlaySoundToSelf") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                CRecipientFilter filter;
                filter.AddRecipient(player);
                
                if (!enginesound->PrecacheSound(Value.String(), true))
                    CBaseEntity::PrecacheScriptSound(Value.String());

                EmitSound_t params;
                params.m_pSoundName = Value.String();
                params.m_flSoundTime = 0.0f;
                params.m_pflSoundDuration = nullptr;
                params.m_bWarnOnDirectWaveReference = true;
                CBaseEntity::EmitSound(filter, ENTINDEX(player), params);
                return true;
            }
            else if (stricmp(szInputName, "IgnitePlayerDuration") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_FLOAT);
                CTFPlayer *activator = pActivator != nullptr && pActivator->IsPlayer() ? ToTFPlayer(pActivator) : player;
                player->m_Shared->Burn(activator, nullptr, Value.Float());
                return true;
            }
            else if (stricmp(szInputName, "WeaponSwitchSlot") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_INTEGER);
                player->Weapon_Switch(player->Weapon_GetSlot(Value.Int()));
                return true;
            }
            else if (stricmp(szInputName, "WeaponStripSlot") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_INTEGER);
                int slot = Value.Int();
                CBaseCombatWeapon *weapon = player->GetActiveTFWeapon();
                if (slot != -1) {
                    weapon = player->Weapon_GetSlot(slot);
                }
                if (weapon != nullptr)
                    weapon->Remove();
                return true;
            }
            else if (stricmp(szInputName, "RemoveItem") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                ForEachTFPlayerEconEntity(player, [&](CEconEntity *entity){
                    if (entity->GetItem() != nullptr && FStrEq(entity->GetItem()->GetItemDefinition()->GetName(), Value.String())) {
                        if (entity->MyCombatWeaponPointer() != nullptr) {
                            player->Weapon_Detach(entity->MyCombatWeaponPointer());
                        }
                        entity->Remove();
                    }
                });
                return true;
            }
            else if (stricmp(szInputName, "GiveItem") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                GiveItemByName(player, Value.String());
                return true;
            }
            else if (stricmp(szInputName, "DropItem") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                Value.Convert(FIELD_INTEGER);
                int slot = Value.Int();
                CBaseCombatWeapon *weapon = player->GetActiveTFWeapon();
                if (slot != -1) {
                    weapon = player->Weapon_GetSlot(slot);
                }

                if (weapon != nullptr) {
                    CEconItemView *item_view = weapon->GetItem();

                    allow_create_dropped_weapon = true;
                    auto dropped = CTFDroppedWeapon::Create(player, player->EyePosition(), vec3_angle, weapon->GetWorldModel(), item_view);
                    if (dropped != nullptr)
                        dropped->InitDroppedWeapon(player, static_cast<CTFWeaponBase *>(weapon), false, false);

                    allow_create_dropped_weapon = false;
                }
            }
            else if (stricmp(szInputName, "SetCurrency") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                player->RemoveCurrency(player->GetCurrency() - atoi(Value.String()));
            }
            else if (stricmp(szInputName, "AddCurrency") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                player->RemoveCurrency(atoi(Value.String()) * -1);
            }
            else if (stricmp(szInputName, "RemoveCurrency") == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                player->RemoveCurrency(atoi(Value.String()));
            }
            else if (strnicmp(szInputName, "CurrencyOutput", 15) == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                int cost = atoi(szInputName + 15);
                if(player->GetCurrency() >= cost){
                    char param_tokenized[2048] = "";
                    V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                    if(strcmp(param_tokenized, "") != 0){
                        char *target = strtok(param_tokenized,",");
                        char *action = NULL;
                        char *value = NULL;
                        if(target != NULL)
                            action = strtok(NULL,",");
                        if(action != NULL)
                            value = strtok(NULL,"");
                        if(value != NULL){
                            CEventQueue &que = g_EventQueue;
                            variant_t actualvalue;
                            string_t stringvalue = AllocPooledString(value);
                            actualvalue.SetString(stringvalue);
                            que.AddEvent(STRING(AllocPooledString(target)), STRING(AllocPooledString(action)), actualvalue, 0.0, player, player, -1);
                        }
                    }
                    player->RemoveCurrency(cost);
                }
                return true;
            }
            else if (strnicmp(szInputName, "CurrencyInvertOutput", 21) == 0) {
                CTFPlayer *player = ToTFPlayer(ent);
                int cost = atoi(szInputName + 21);
                if(player->GetCurrency() < cost){
                    char param_tokenized[2048] = "";
                    V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
                    if(strcmp(param_tokenized, "") != 0){
                        char *target = strtok(param_tokenized,",");
                        char *action = NULL;
                        char *value = NULL;
                        if(target != NULL)
                            action = strtok(NULL,",");
                        if(action != NULL)
                            value = strtok(NULL,"");
                        if(value != NULL){
                            CEventQueue &que = g_EventQueue;
                            variant_t actualvalue;
                            string_t stringvalue = AllocPooledString(value);
                            actualvalue.SetString(stringvalue);
                            que.AddEvent(STRING(AllocPooledString(target)), STRING(AllocPooledString(action)), actualvalue, 0.0, player, player, -1);
                        }
                    }
                    //player->RemoveCurrency(cost);
                }
                return true;
            }
            else if (stricmp(szInputName, "RefillAmmo") == 0) {
                CTFPlayer* player = ToTFPlayer(ent);
                for(int i = 0; i < 7; ++i){
                    player->SetAmmoCount(player->GetMaxAmmo(i), i);
                }
                return true;
            }
            else if(stricmp(szInputName, "Regenerate") == 0){
                CTFPlayer* player = ToTFPlayer(ent);
                player->Regenerate(true);
                return true;
            }
            else if(stricmp(szInputName, "BotCommand") == 0){
                CTFBot* bot = ToTFBot(ent);
                if (bot != nullptr)
                    bot->MyNextBotPointer()->OnCommandString(Value.String());
                return true;
            }
            else if(stricmp(szInputName, "ResetInventory") == 0){
                CTFPlayer* player = ToTFPlayer(ent);
                
                player->GiveDefaultItemsNoAmmo();

            }
            else if(stricmp(szInputName, "PlaySequence") == 0){
                CTFPlayer* player = ToTFPlayer(ent);
                player->PlaySpecificSequence(Value.String());

                return true;
            }
#ifdef GCC11
            else if(stricmp(szInputName, "TauntFromItem") == 0){
                CTFPlayer* player{ToTFPlayer(ent)};
                auto view{CEconItemView::Create()};
                const std::string_view input{Value.String()};
                auto index{vi::from_str<int>(Value.String())};
                const auto v{vi::split_str(input, "|")};
                const auto do_taunt{[&view, &player](int index) -> void {
                    view->Init(index);
                    player->PlayTauntSceneFromItem(view);
                    CEconItemView::Destroy(view);
                }};
                if(index && (v.size() < 2)){
                    do_taunt(*index);
                } else {
                    if(v.size() > 1){
                        index = {vi::from_str<int>(v[0])};
                        const auto value{v[1]};
                        if(index && (value.length() > 0)){
                            std::string_view no_op{value};
                            no_op.remove_prefix(1);
                            const auto remove_op{vi::from_str<float>(no_op)};
                            auto original{vi::from_str<float>(value)};
                            
                            const std::unordered_map<char,
                                        std::function<void()>> ops{
                                {'+', [&player, &remove_op]{
                                        player->m_flTauntAttackTime += *remove_op; 
                                    }
                                },
                                {'-', [&player, &remove_op]{
                                        player->m_flTauntAttackTime -= *remove_op; 
                                    }
                                },
                                {'*', [&player, &remove_op]{
                                        player->m_flTauntAttackTime =
                                            gpGlobals->curtime + 
                                            (player->m_flTauntAttackTime -
                                             gpGlobals->curtime) * 
                                            *remove_op; 
                                    }
                                }
                            };
                            if(value[0] == 'i'){
                                do_taunt(*index);
                                player->m_flTauntAttackTime = 0.1f;
                                original = {};
                            }
                            for(const auto& [op, func] : ops){
                                if((value[0] == op) && remove_op){
                                    do_taunt(*index);
                                    func();
                                    original = {};
                                    break;
                                }
                            }
                            if(original){
                                do_taunt(*index);
                                player->m_flTauntAttackTime =
                                    gpGlobals->curtime + *original;
                            }
                        }
                    }
                }
            }
            else if(stricmp(szInputName, "TauntIndexConcept") == 0){
                CTFPlayer* player{ToTFPlayer(ent)};
                const std::string_view input{Value.String()};
                auto v{vi::split_str(input, "|")};
                if(v.size() > 1){
                    auto index{
                        vi::from_str<int>(v[0])
                    };
                    auto taunt_concept{
                        vi::from_str<int>(v[1])
                    };
                    if(index && taunt_concept) 
                        player->Taunt(*index, *taunt_concept);
                }
            }
            
#endif
        }
        else if (ent->GetClassname() == point_viewcontrol_classname) {
            auto camera = static_cast<CTriggerCamera *>(ent);
            if (stricmp(szInputName, "EnableAll") == 0) {
                ForEachTFPlayer([&](CTFPlayer *player) {
                    if (player->IsBot())
                        return;
                    else {

                        camera->m_hPlayer = player;
                        camera->Enable();
                        camera->m_spawnflags |= 512;
                    }
                });
                return true;
            }
            else if (stricmp(szInputName, "DisableAll") == 0) {
                ForEachTFPlayer([&](CTFPlayer *player) {
                    if (player->IsBot())
                        return;
                    else {
                        camera->m_hPlayer = player;
                        camera->Disable();
                        player->m_takedamage = player->IsObserver() ? 0 : 2;
                        camera->m_spawnflags &= ~(512);
                    }
                });
                return true;
            }
            else if (stricmp(szInputName, "SetTarget") == 0) {
                camera->m_hTarget = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
                return true;
            }
        }
        else if (ent->GetClassname() == trigger_detector_class) {
            if (stricmp(szInputName, "targettest") == 0) {
                auto data = GetExtraTriggerDetectorData(ent);
                if (data->m_hLastTarget != nullptr) {
                    ent->FireCustomOutput<"targettestpass">(data->m_hLastTarget, ent, Value);
                }
                else {
                    ent->FireCustomOutput<"targettestfail">(nullptr, ent, Value);
                }
                return true;
            }
        }
        else if (ent->GetClassname() == weapon_spawner_classname) {
            if (stricmp(szInputName, "DropWeapon") == 0) {
                
                auto data = GetExtraData<ExtraEntityDataWeaponSpawner>(ent);
                auto name = ent->GetCustomVariable<"item">();
                auto item_def = GetItemSchema()->GetItemDefinitionByName(name);

                if (item_def != nullptr) {
                    auto item = CEconItemView::Create();
                    item->Init(item_def->m_iItemDefIndex, item_def->m_iItemQuality, 9999, 0);
                    item->m_iItemID = (RandomInt(INT_MIN, INT_MAX) << 16) + ENTINDEX(ent);
                    Mod::Pop::PopMgr_Extensions::AddCustomWeaponAttributes(name, item);
                    auto &vars = GetCustomVariables(ent);
                    for (auto &var : vars) {
                        auto attr_def = GetItemSchema()->GetAttributeDefinitionByName(STRING(var.key));
                        if (attr_def != nullptr) {
                            item->GetAttributeList().AddStringAttribute(attr_def, var.value.String());
                        }
                    }
                    auto weapon = CTFDroppedWeapon::Create(nullptr, ent->EyePosition(), vec3_angle, item->GetPlayerDisplayModel(1, 2), item);
                    if (weapon != nullptr) {
                        if (weapon->VPhysicsGetObject() != nullptr) {
                            weapon->VPhysicsGetObject()->SetMass(25.0f);

                            if (ent->GetCustomVariableFloat<"nomotion">() != 0) {
                                weapon->VPhysicsGetObject()->EnableMotion(false);
                            }
                        }
                        auto weapondata = weapon->GetOrCreateEntityModule<DroppedWeaponModule>("droppedweapon");
                        weapondata->m_hWeaponSpawner = ent;
                        weapondata->ammo = ent->GetCustomVariableFloat<"ammo">(-1);
                        weapondata->clip = ent->GetCustomVariableFloat<"clip">(-1);
                        weapondata->energy = ent->GetCustomVariableFloat<"energy">(FLT_MIN);
                        weapondata->charge = ent->GetCustomVariableFloat<"charge">(FLT_MAX);

                        data->m_SpawnedWeapons.push_back(weapon);
                    }
                    CEconItemView::Destroy(item);
                }
                return true;
            }
            else if (stricmp(szInputName, "RemoveDroppedWeapons") == 0) {
                auto data = GetExtraWeaponSpawnerData(ent);
                for (auto weapon : data->m_SpawnedWeapons) {
                    if (weapon != nullptr) {
                        weapon->Remove();
                    }
                }
                data->m_SpawnedWeapons.clear();
                return true;
            }
        }
        else if (ent->GetClassname() == PStr<"prop_vehicle_driveable">()) {
            if (stricmp(szInputName, "EnterVehicle") == 0) {
                auto target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
                auto vehicle = rtti_cast<CPropVehicleDriveable *>(ent);
                if (ToTFPlayer(target) != nullptr && vehicle != nullptr) {
                    
                    Vector delta = target->GetAbsOrigin() - ent->GetAbsOrigin();
                    
                    QAngle angToTarget;
                    VectorAngles(delta, angToTarget);
                    ToTFPlayer(target)->SnapEyeAngles(angToTarget);
                    
                    CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
                    serverVehicle->HandlePassengerEntry(ToTFPlayer(target), true);
                }
            }
            if (stricmp(szInputName, "ExitVehicle") == 0) {
                auto vehicle = rtti_cast<CPropVehicleDriveable *>(ent);
                if (vehicle != nullptr && vehicle->m_hPlayer != nullptr) {
                    CBaseServerVehicle *serverVehicle = vehicle->m_pServerVehicle;
                    serverVehicle->HandlePassengerExit(vehicle->m_hPlayer);
                }
            }
        }
        else if (ent->GetClassname() == PStr<"$math_vector">()) {
            auto mathVector = ent->GetOrCreateEntityModule<MathVectorModule>("math_vector");
            auto vecValue = ent->GetCustomVariableVector<"value">();
            if (stricmp(szInputName, "Set") == 0) {
                Value.Convert(FIELD_VECTOR);
                ent->SetCustomVariable("value", Value);
                return true;
            }
            else if (stricmp(szInputName, "Add") == 0) {
                Value.Convert(FIELD_VECTOR);
                Vector vec;
                Value.Vector3D(vec);
                vec += vecValue;
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "AddScalar") == 0) {
                Value.Convert(FIELD_FLOAT);
                Vector vec = vecValue + Vector(Value.Float(), Value.Float(), Value.Float());
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "Subtract") == 0) {
                Value.Convert(FIELD_VECTOR);
                Vector vec;
                Value.Vector3D(vec);
                vec -= vecValue;
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "SubtractScalar") == 0) {
                Value.Convert(FIELD_FLOAT);
                Vector vec = vecValue - Vector(Value.Float(), Value.Float(), Value.Float());
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "Multiply") == 0) {
                Value.Convert(FIELD_VECTOR);
                Vector vec;
                Value.Vector3D(vec);
                vec *= vecValue;
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "MultiplyScalar") == 0) {
                Value.Convert(FIELD_FLOAT);
                Vector vec = vecValue * Value.Float();
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "Divide") == 0) {
                Value.Convert(FIELD_VECTOR);
                Vector vec;
                Value.Vector3D(vec);
                vec /= vecValue;
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "DivideScalar") == 0) {
                Value.Convert(FIELD_FLOAT);
                Vector vec = vecValue / Value.Float();
                Value.SetVector3D(vec);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "DotProduct") == 0) {
                if (Value.Convert(FIELD_VECTOR)) {
                    Vector vec;
                    Value.Vector3D(vec);
                    Value.SetFloat(DotProduct(vec, vecValue));
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
                return true;
            }
            else if (stricmp(szInputName, "CrossProduct") == 0) {
                if (Value.Convert(FIELD_VECTOR)) {
                    Vector vec;
                    Value.Vector3D(vec);
                    Vector out;
                    CrossProduct(vec, vecValue, out);
                    Value.SetVector3D(out);
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
                return true;
            }
            else if (stricmp(szInputName, "Distance") == 0) {
                if (Value.Convert(FIELD_VECTOR)) {
                    Vector vec;
                    Value.Vector3D(vec);
                    Value.SetFloat(vec.DistTo(vecValue));
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
                return true;
            }
            else if (stricmp(szInputName, "DistanceToEntity") == 0) {
                auto target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
                if (target != nullptr) {
                    Value.SetFloat(vecValue.DistTo(target->GetAbsOrigin()));
                    ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                }
                return true;
            }
            else if (stricmp(szInputName, "RotateVector") == 0) {
                Value.Convert(FIELD_VECTOR);
                QAngle ang;
                Value.Vector3D(*reinterpret_cast<Vector *>(&ang));
                Vector out;
                VectorRotate(vecValue, ang, out);
                Value.SetVector3D(out);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "Length") == 0) {
                Value.SetFloat(vecValue.Length());
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "ToAngles") == 0) {
                QAngle out;
                VectorAngles(vecValue, out);
                Value.SetVector3D(*reinterpret_cast<Vector *>(&out));
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "Normalize") == 0) {
                Vector out = vecValue.Normalized();
                Value.SetVector3D(out);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "ToForwardVector") == 0) {
                Vector out;
                AngleVectors(*reinterpret_cast<QAngle *>(&vecValue), &out);
                Value.SetVector3D(out);
                ent->FireCustomOutput<"outvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "GetX") == 0) {
                Value.SetFloat(vecValue.x);
                ent->FireCustomOutput<"getvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "GetY") == 0) {
                Value.SetFloat(vecValue.y);
                ent->FireCustomOutput<"getvalue">(pActivator, ent, Value);
                return true;
            }
            else if (stricmp(szInputName, "GetZ") == 0) {
                Value.SetFloat(vecValue.z);
                ent->FireCustomOutput<"getvalue">(pActivator, ent, Value);
                return true;
            }
        }
        if (stricmp(szInputName, "FireUserAsActivator1") == 0) {
            ent->m_OnUser1->FireOutput(Value, ent, ent);
            return true;
        }
        else if (stricmp(szInputName, "FireUserAsActivator2") == 0) {
            ent->m_OnUser2->FireOutput(Value, ent, ent);
            return true;
        }
        else if (stricmp(szInputName, "FireUserAsActivator3") == 0) {
            ent->m_OnUser3->FireOutput(Value, ent, ent);
            return true;
        }
        else if (stricmp(szInputName, "FireUserAsActivator4") == 0) {
            ent->m_OnUser4->FireOutput(Value, ent, ent);
            return true;
        }
        else if (stricmp(szInputName, "FireUser5") == 0) {
            ent->FireCustomOutput<"onuser5">(pActivator, ent, Value);
            return true;
        }
        else if (stricmp(szInputName, "FireUser6") == 0) {
            ent->FireCustomOutput<"onuser6">(pActivator, ent, Value);
            return true;
        }
        else if (stricmp(szInputName, "FireUser7") == 0) {
            ent->FireCustomOutput<"onuser7">(pActivator, ent, Value);
            return true;
        }
        else if (stricmp(szInputName, "FireUser8") == 0) {
            ent->FireCustomOutput<"onuser8">(pActivator, ent, Value);
            return true;
        }
        else if (stricmp(szInputName, "TakeDamage") == 0) {
            Value.Convert(FIELD_INTEGER);
            int damage = Value.Int();
            CBaseEntity *attacker = ent;
            
            CTakeDamageInfo info(attacker, attacker, nullptr, vec3_origin, ent->GetAbsOrigin(), damage, DMG_PREVENT_PHYSICS_FORCE, 0 );
            ent->TakeDamage(info);
            return true;
        }
        else if (stricmp(szInputName, "AddHealth") == 0) {
            Value.Convert(FIELD_INTEGER);
            CBaseEntity *attacker = ent;
            ent->TakeHealth(Value.Int(), DMG_GENERIC);
            return true;
        }
        else if (stricmp(szInputName, "TakeDamageFromActivator") == 0) {
            Value.Convert(FIELD_INTEGER);
            int damage = Value.Int();
            CBaseEntity *attacker = pActivator;
            
            CTakeDamageInfo info(attacker, attacker, nullptr, vec3_origin, ent->GetAbsOrigin(), damage, DMG_PREVENT_PHYSICS_FORCE, 0 );
            ent->TakeDamage(info);
            return true;
        }
        else if (stricmp(szInputName, "SetModelOverride") == 0) {
            int replace_model = CBaseEntity::PrecacheModel(Value.String());
            if (replace_model != -1) {
                for (int i = 0; i < MAX_VISION_MODES; ++i) {
                    ent->SetModelIndexOverride(i, replace_model);
                }
            }
            return true;
        }
        else if (stricmp(szInputName, "SetModel") == 0) {
            CBaseEntity::PrecacheModel(Value.String());
            ent->SetModel(Value.String());
            return true;
        }
        else if (stricmp(szInputName, "SetModelSpecial") == 0) {
            int replace_model = CBaseEntity::PrecacheModel(Value.String());
            if (replace_model != -1) {
                ent->SetModelIndex(replace_model);
            }
            return true;
        }
        else if (stricmp(szInputName, "SetOwner") == 0) {
            auto owner = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
            if (owner != nullptr) {
                ent->SetOwnerEntity(owner);
            }
            return true;
        }
        else if (stricmp(szInputName, "GetKeyValue") == 0) {
            variant_t variant;
            ent->ReadKeyField(Value.String(), &variant);
            ent->m_OnUser1->FireOutput(variant, pActivator, ent);
            return true;
        }
        else if (stricmp(szInputName, "InheritOwner") == 0) {
            auto owner = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
            if (owner != nullptr) {
                ent->SetOwnerEntity(owner->GetOwnerEntity());
            }
            return true;
        }
        else if (stricmp(szInputName, "InheritParent") == 0) {
            auto owner = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
            if (owner != nullptr) {
                ent->SetParent(owner->GetMoveParent(), -1);
            }
            return true;
        }
        else if (stricmp(szInputName, "MoveType") == 0) {
            variant_t variant;
            int val1=0;
            int val2=MOVECOLLIDE_DEFAULT;

            sscanf(Value.String(), "%d,%d", &val1, &val2);
            ent->SetMoveType((MoveType_t)val1, (MoveCollide_t)val2);
            return true;
        }
        else if (stricmp(szInputName, "PlaySound") == 0) {
            
            if (!enginesound->PrecacheSound(Value.String(), true))
                CBaseEntity::PrecacheScriptSound(Value.String());

            ent->EmitSound(Value.String());
            return true;
        }
        else if (stricmp(szInputName, "StopSound") == 0) {
            ent->StopSound(Value.String());
            return true;
        }
        else if (stricmp(szInputName, "SetLocalOrigin") == 0) {
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            ent->SetLocalOrigin(vec);
            return true;
        }
        else if (stricmp(szInputName, "SetLocalAngles") == 0) {
            Value.Convert(FIELD_VECTOR);
            QAngle vec;
            Value.Vector3D(*reinterpret_cast<Vector *>(&vec));
            ent->SetLocalAngles(vec);
            return true;
        }
        else if (stricmp(szInputName, "SetLocalVelocity") == 0) {
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            ent->SetLocalVelocity(vec);
            return true;
        }
        else if (stricmp(szInputName, "TeleportToEntity") == 0) {
            auto target = servertools->FindEntityByName(nullptr, Value.String(), ent, pActivator, pCaller);
            if (target != nullptr) {
                Vector targetpos = target->GetAbsOrigin();
                ent->Teleport(&targetpos, nullptr, nullptr);
            }
            return true;
        }
        else if (stricmp(szInputName, "MoveRelative") == 0) {
            Value.Convert(FIELD_VECTOR);
            Vector vec;
            Value.Vector3D(vec);
            vec = vec + ent->GetLocalOrigin();
            ent->SetLocalOrigin(vec);
            return true;
        }
        else if (stricmp(szInputName, "RotateRelative") == 0) {
            Value.Convert(FIELD_VECTOR);
            QAngle vec;
            Value.Vector3D(*reinterpret_cast<Vector *>(&vec));
            vec = vec + ent->GetLocalAngles();
            ent->SetLocalAngles(vec);
            return true;
        }
        else if (stricmp(szInputName, "TestEntity") == 0) {
            for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                auto filter = rtti_cast<CBaseFilter *>(ent);
                if (filter != nullptr && filter->PassesFilter(pCaller, target)) {
                    filter->m_OnPass->FireOutput(Value, pActivator, target);
                }
            }
            return true;
        }
        else if (stricmp(szInputName, "StartTouchEntity") == 0) {
            auto filter = rtti_cast<CBaseTrigger *>(ent);
            if (filter != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                    filter->StartTouch(target);
                }
            }
            return true;
        }
        else if (stricmp(szInputName, "EndTouchEntity") == 0) {
            auto filter = rtti_cast<CBaseTrigger *>(ent);
            if (filter != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, Value.String(), ent, pActivator, pCaller)) != nullptr ;) {
                    filter->EndTouch(target);
                }
            }
            return true;
        }
        else if (stricmp(szInputName, "RotateTowards") == 0) {
            auto rotating = rtti_cast<CFuncRotating *>(ent);
            if (rotating != nullptr) {
                CBaseEntity *target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
                if (target != nullptr) {
                    auto data = GetExtraFuncRotatingData(rotating);
                    data->m_hRotateTarget = target;

                    if (rotating->GetNextThink("RotatingFollowEntity") < gpGlobals->curtime) {
                        THINK_FUNC_SET(rotating, RotatingFollowEntity, gpGlobals->curtime + 0.1);
                    }
                }
            }
            auto data = ent->GetEntityModule<RotatorModule>("rotator");
            if (data != nullptr) {
                CBaseEntity *target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
                if (target != nullptr) {
                    data->m_hRotateTarget = target;

                    if (ent->GetNextThink("RotatingFollowEntity") < gpGlobals->curtime) {
                        Msg("install rotator\n");
                        THINK_FUNC_SET(ent, RotatorModuleTick, gpGlobals->curtime + 0.1);
                    }
                }
            }
            return true;
        }
        else if (stricmp(szInputName, "StopRotateTowards") == 0) {
            auto data = ent->GetEntityModule<RotatorModule>("rotator");
            if (data != nullptr) {
                data->m_hRotateTarget = nullptr;
            }
            return true;
        }
        else if (stricmp(szInputName, "SetForwardVelocity") == 0) {
            Vector fwd;
            AngleVectors(ent->GetAbsAngles(), &fwd);
            fwd *= strtof(Value.String(), nullptr);

            IPhysicsObject *pPhysicsObject = ent->VPhysicsGetObject();
            if (pPhysicsObject) {
                pPhysicsObject->SetVelocity(&fwd, nullptr);
            }
            else {
                ent->SetAbsVelocity(fwd);
            }
            
            return true;
        }
        else if (stricmp(szInputName, "FaceEntity") == 0) {
            CBaseEntity *target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
            if (target != nullptr) {
                Vector delta = target->GetAbsOrigin() - ent->GetAbsOrigin();
                
                QAngle angToTarget;
                VectorAngles(delta, angToTarget);
                ent->SetAbsAngles(angToTarget);
                if (ToTFPlayer(ent) != nullptr) {
                    ToTFPlayer(ent)->SnapEyeAngles(angToTarget);
                }
            }
            
            return true;
        }
        else if (stricmp(szInputName, "SetFakeParent") == 0) {
            auto data = ent->GetEntityModule<FakeParentModule>("fakeparent");
            if (data != nullptr) {
                CBaseEntity *target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
                if (target != nullptr) {
                    data->m_hParent = target;
                    data->m_bParentSet = true;
                    if (ent->GetNextThink("FakeParentModuleTick") < gpGlobals->curtime) {
                        THINK_FUNC_SET(ent, FakeParentModuleTick, gpGlobals->curtime + 0.01);
                    }
                }
            }
            
            return true;
        }
        else if (stricmp(szInputName, "SetAimFollow") == 0) {
            auto data = ent->GetEntityModule<AimFollowModule>("aimfollow");
            if (data != nullptr) {
                CBaseEntity *target = servertools->FindEntityGeneric(nullptr, Value.String(), ent, pActivator, pCaller);
                if (target != nullptr) {
                    data->m_hParent = target;
                    if (ent->GetNextThink("AimFollowModuleTick") < gpGlobals->curtime) {
                        THINK_FUNC_SET(ent, AimFollowModuleTick, gpGlobals->curtime + 0.01);
                    }
                }
            }
            
            return true;
        }
        else if (stricmp(szInputName, "ClearFakeParent") == 0) {
            auto data = ent->GetEntityModule<FakeParentModule>("fakeparent");
            if (data != nullptr) {
                data->m_hParent = nullptr;
                data->m_bParentSet = false;
            }
            
            return true;
        }
        else if (strnicmp(szInputName, "SetVar$", strlen("SetVar$")) == 0) {
            SetEntityVariable(ent, VARIABLE, szInputName + strlen("SetVar$"), Value);
            return true;
        }
        else if (strnicmp(szInputName, "GetVar$", strlen("GetVar$")) == 0) {
            FireGetInput(ent, VARIABLE, szInputName + strlen("GetVar$"), pActivator, pCaller, Value);
            return true;
        }
        else if (strnicmp(szInputName, "SetKey$", strlen("SetKey$")) == 0) {
            SetEntityVariable(ent, KEYVALUE, szInputName + strlen("SetKey$"), Value);
            return true;
        }
        else if (strnicmp(szInputName, "GetKey$", strlen("GetKey$")) == 0) {
            FireGetInput(ent, KEYVALUE, szInputName + strlen("GetKey$"), pActivator, pCaller, Value);
            return true;
        }
        else if (strnicmp(szInputName, "SetData$", strlen("SetData$")) == 0) {
            SetEntityVariable(ent, DATAMAP, szInputName + strlen("SetData$"), Value);
            return true;
        }
        else if (strnicmp(szInputName, "GetData$", strlen("GetData$")) == 0) {
            FireGetInput(ent, DATAMAP, szInputName + strlen("GetData$"), pActivator, pCaller, Value);
            return true;
        }
        else if (strnicmp(szInputName, "SetProp$", strlen("SetProp$")) == 0) {
            SetEntityVariable(ent, SENDPROP, szInputName + strlen("SetProp$"), Value);
            return true;
        }
        else if (strnicmp(szInputName, "GetProp$", strlen("GetProp$")) == 0) {
            FireGetInput(ent, SENDPROP, szInputName + strlen("GetProp$"), pActivator, pCaller, Value);
            return true;
        }
        else if (stricmp(szInputName, "GetEntIndex") == 0) {
            char param_tokenized[256] = "";
            V_strncpy(param_tokenized, Value.String(), sizeof(param_tokenized));
            char *targetstr = strtok(param_tokenized,"|");
            char *action = strtok(NULL,"|");
            
            variant_t variable;
            variable.SetInt(ent->entindex());
            if (targetstr != nullptr && action != nullptr) {
                for (CBaseEntity *target = nullptr; (target = servertools->FindEntityGeneric(target, targetstr, ent, pActivator, pCaller)) != nullptr ;) {
                    target->AcceptInput(action, pActivator, ent, variable, 0);
                }
            }
            return true;
        }
        else if (stricmp(szInputName, "AddModule") == 0) {
            AddModuleByName(ent, Value.String());
            return true;
        }
        else if (stricmp(szInputName, "RemoveModule") == 0) {
            ent->RemoveEntityModule(Value.String());
            return true;
        }
        else if (stricmp(szInputName, "RemoveOutput") == 0) {
            const char *name = Value.String();
            auto datamap = ent->GetDataDescMap();
            for (datamap_t *dmap = datamap; dmap != NULL; dmap = dmap->baseMap) {
                // search through all the readable fields in the data description, looking for a match
                for (int i = 0; i < dmap->dataNumFields; i++) {
                    if ((dmap->dataDesc[i].flags & FTYPEDESC_OUTPUT) && stricmp(dmap->dataDesc[i].externalName, name) == 0) {
                        ((CBaseEntityOutput*)(((char*)ent) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ]))->DeleteAllElements();
                        return true;
                    }
                }
            }
            ent->RemoveCustomOutput(name+1);
            return true;
        }
        else if (stricmp(szInputName, "CancelPending") == 0) {
            g_EventQueue.GetRef().CancelEvents(ent);
            return true;
        }
        return false;
    }

	DETOUR_DECL_MEMBER(bool, CBaseEntity_AcceptInput, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
    {
        CBaseEntity *ent = reinterpret_cast<CBaseEntity *>(this);
        if (Value.FieldType() == FIELD_STRING) {
            const char *str = Value.String();
            if (str[0] == '$' && str[1] == '$' && str[2] == '=') {
                Evaluation eval(Value);
                variant_t var2;
                eval.Evaluate(str + 3, ent, pActivator, pCaller, var2);
            }
        }
        if (szInputName[0] == '$' && HandleCustomInput(ent, szInputName + 1, pActivator, pCaller, Value, outputID)) {
            return true;
        }

        return DETOUR_MEMBER_CALL(CBaseEntity_AcceptInput)(szInputName, pActivator, pCaller, Value, outputID);
    }

    void ActivateLoadedInput()
    {
        DevMsg("ActivateLoadedInput\n");
        auto entity = servertools->FindEntityByName(nullptr, "sigsegv_load");
        
        if (entity != nullptr) {
            variant_t variant1;
            variant1.SetString(NULL_STRING);

            entity->AcceptInput("FireUser1", UTIL_EntityByIndex(0), UTIL_EntityByIndex(0) ,variant1,-1);
        }
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_CleanUpMap)
	{
		DETOUR_MEMBER_CALL(CTFGameRules_CleanUpMap)();
        ActivateLoadedInput();
	}

    CBaseEntity *DoSpecialParsing(const char *szName, CBaseEntity *pStartEntity, const std::function<CBaseEntity *(CBaseEntity *, const char *)>& functor)
    {
        if (szName[0] == '@' && szName[1] != '\0') {
            if (szName[2] == '@') {
                const char *realname = szName + 3;
                CBaseEntity *nextentity = pStartEntity;
                // Find parent of entity
                if (szName[1] == 'p') {
                    static CBaseEntity *last_parent = nullptr;
                    if (pStartEntity == nullptr)
                        last_parent = nullptr;

                    while (true) {
                        last_parent = functor(last_parent, realname); 
                        nextentity = last_parent;
                        if (nextentity != nullptr) {
                            if (nextentity->GetMoveParent() != nullptr) {
                                return nextentity->GetMoveParent();
                            }
                            else {
                                continue;
                            }
                        }
                        else {
                            return nullptr;
                        }
                    }
                }
                // Find children of entity
                else if (szName[1] == 'c') {
                    bool skipped = false;
                    while (true) {
                        if (pStartEntity != nullptr && !skipped ) {
                            if (pStartEntity->NextMovePeer() != nullptr) {
                                return pStartEntity->NextMovePeer();
                            }
                            else{
                                pStartEntity = pStartEntity->GetMoveParent();
                            }
                        }
                        pStartEntity = functor(pStartEntity, realname); 
                        if (pStartEntity == nullptr) {
                            return nullptr;
                        }
                        else {
                            if (pStartEntity->FirstMoveChild() != nullptr) {
                                return pStartEntity->FirstMoveChild();
                            }
                            else {
                                skipped = true;
                                continue;
                            }
                        }
                    }
                }
                // Find entity with filter
                else if (szName[1] == 'f') {
                    bool skipped = false;
                    
                    std::string filtername = realname;
                    int atSplit = filtername.find('@');
                    if (atSplit != std::string::npos) {
                        realname += atSplit + 1;
                        filtername.resize(atSplit);

                        CBaseFilter *filter = rtti_cast<CBaseFilter *>(servertools->FindEntityByName(nullptr, realname));
                        if (filter != nullptr) {
                            while (true) {
                                pStartEntity = functor(pStartEntity, realname);
                                if (pStartEntity == nullptr) return nullptr;

                                if (filter->PassesFilter(pStartEntity, pStartEntity)) return pStartEntity;
                            }
                        }
                    }
                }
                // Find entity from entity variable
                else if (szName[1] == 'e') {
                    bool skipped = false;
                    
                    std::string varname = realname;

                    int atSplit = varname.find('@');
                    if (atSplit != std::string::npos) {
                        realname += atSplit + 1;
                        varname.resize(atSplit);
                        
                        static CBaseEntity *last_entity = nullptr;
                        if (pStartEntity == nullptr)
                            last_entity = nullptr;

                        while (true) {
                            last_entity = functor(last_entity, realname); 
                            nextentity = last_entity;
                            if (nextentity != nullptr) {
                                variant_t variant;
                                variant_t variant2;
                                
                                if ((GetEntityVariable(nextentity, DATAMAP, varname.c_str(), variant) && variant.Entity() != nullptr) || 
                                    (GetEntityVariable(nextentity, SENDPROP, varname.c_str(), variant) && variant.Entity() != nullptr) || 
                                    (GetEntityVariable(nextentity, VARIABLE, varname.c_str(), variant) && variant.Entity() != nullptr)) {
                                        
                                    return variant.Entity();
                                }
                                else {
                                    continue;
                                }
                            }
                            else {
                                return nullptr;
                            }
                        }
                    }
                }
            }
            else if (szName[1] == 'b' && szName[2] == 'b') {
                Vector min;
                Vector max;
                int scannum = sscanf(szName+3, "%f %f %f %f %f %f", &min.x, &min.y, &min.z, &max.x, &max.y, &max.z);
                if (scannum == 6) {
                    const char *realname = strchr(szName + 3, '@');
                    if (realname != nullptr) {
                        realname += 1;
                        while (true) {
                            pStartEntity = functor(pStartEntity, realname); 
                            if (pStartEntity != nullptr && !pStartEntity->GetAbsOrigin().WithinAABox(min, max)) {
                                continue;
                            }
                            else {
                                return pStartEntity;
                            }
                        }
                    }
                }
            }
        }

        return functor(pStartEntity, szName);
    }

    string_t last_find_entity_name_str = NULL_STRING;
    const char *last_find_entity_name = nullptr;
    bool last_find_entity_wildcard = false;
    ConVar cvar_fast_lookup("sig_etc_fast_entity_name_lookup", "1", FCVAR_NONE, "Converts all entity names to lowercase for faster lookup", 
        [](IConVar *pConVar, const char *pOldValue, float flOldValue){
            // Immediately convert every name and classname to lowercase
            if (static_cast<ConVar *>(pConVar)->GetBool()) {
                ForEachEntity([](CBaseEntity *entity){
                    if (entity->GetEntityName() != NULL_STRING) {
                        char *lowercase = stackalloc(strlen(STRING(entity->GetEntityName())) + 1);
                        StrLowerCopy(STRING(entity->GetEntityName()), lowercase);
                        entity->SetName(AllocPooledString(lowercase));
                    }
                    if (entity->GetClassnameString() != NULL_STRING) {
                        char *lowercase = stackalloc(strlen(STRING(entity->GetClassnameString())) + 1);
                        StrLowerCopy(STRING(entity->GetClassnameString()), lowercase);
                        entity->SetClassname(AllocPooledString(lowercase));
                    }
                });
            }
		});

    DETOUR_DECL_MEMBER(CBaseEntity *, CGlobalEntityList_FindEntityByClassname, CBaseEntity *pStartEntity, const char *szName)
	{
        if (szName == nullptr || szName[0] == '\0') return nullptr;

        if (szName[0] == '@') return DoSpecialParsing(szName, pStartEntity, [&](CBaseEntity *entity, const char *realname) {return servertools->FindEntityByClassname(entity, realname);});

        if (!cvar_fast_lookup.GetBool()) return DETOUR_MEMBER_CALL(CGlobalEntityList_FindEntityByClassname)(pStartEntity, szName);

		auto entList = reinterpret_cast<CBaseEntityList *>(this);

        string_t lowercaseStr;
        if (szName == last_find_entity_name) {
            lowercaseStr = last_find_entity_name_str;
        }
        else {
            int length = strlen(szName);
            last_find_entity_name = szName;
            last_find_entity_wildcard = szName[length - 1] == '*';
            char *lowercase = stackalloc(length + 1);
            StrLowerCopy(szName, lowercase);
            last_find_entity_name_str = lowercaseStr = AllocPooledString(lowercase);
        }
        
        const CEntInfo *pInfo = pStartEntity ? entList->GetEntInfoPtr(pStartEntity->GetRefEHandle())->m_pNext : entList->FirstEntInfo();

        if (!last_find_entity_wildcard) {
            for ( ;pInfo; pInfo = pInfo->m_pNext ) {
                CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
                if (!ent) {
                    DevWarning( "NULL entity in global entity list!\n" );
                    continue;
                }

                if (ent->GetClassnameString() == lowercaseStr) {
                    return ent;
                }
            }
        }
        else {
            for ( ;pInfo; pInfo = pInfo->m_pNext ) {
                CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
                if (!ent) {
                    DevWarning( "NULL entity in global entity list!\n" );
                    continue;
                }

                if (NamesMatchCaseSensitve(STRING(lowercaseStr), ent->GetClassnameString())) {
                    return ent;
                }
            }
        }
		return nullptr;
    }

    DETOUR_DECL_MEMBER(CBaseEntity *, CGlobalEntityList_FindEntityByName, CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter)
	{
        if (szName == nullptr || szName[0] == '\0') return nullptr;

        if (szName[0] == '@') return DoSpecialParsing(szName, pStartEntity, [&](CBaseEntity *entity, const char *realname) {return servertools->FindEntityByName(entity, realname, pSearchingEntity, pActivator, pCaller, pFilter);});

        if (szName[0] == '!' || !cvar_fast_lookup.GetBool()) return DETOUR_MEMBER_CALL(CGlobalEntityList_FindEntityByName)(pStartEntity, szName, pSearchingEntity, pActivator, pCaller, pFilter);
        
        auto entList = reinterpret_cast<CBaseEntityList *>(this);

        string_t lowercaseStr;
        if (szName == last_find_entity_name) {
            lowercaseStr = last_find_entity_name_str;
        }
        else {
            int length = strlen(szName);
            last_find_entity_name = szName;
            last_find_entity_wildcard = szName[length - 1] == '*';
            char *lowercase = stackalloc(length + 1);
            StrLowerCopy(szName, lowercase);
            last_find_entity_name_str = lowercaseStr = AllocPooledString(lowercase);
        }
        
        const CEntInfo *pInfo = pStartEntity ? entList->GetEntInfoPtr(pStartEntity->GetRefEHandle())->m_pNext : entList->FirstEntInfo();

        if (!last_find_entity_wildcard) {
            for ( ;pInfo; pInfo = pInfo->m_pNext ) {
                CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
                if (!ent) {
                    DevWarning( "NULL entity in global entity list!\n" );
                    continue;
                }

                if (ent->GetEntityName() == lowercaseStr)
                {
                    if (pFilter && !pFilter->ShouldFindEntity(ent)) continue;

                    return ent;
                }
            }
        }
        else {
            for ( ;pInfo; pInfo = pInfo->m_pNext ) {
                CBaseEntity *ent = (CBaseEntity *)pInfo->m_pEntity;
                if (!ent) {
                    DevWarning( "NULL entity in global entity list!\n" );
                    continue;
                }
                
                if (NamesMatchCaseSensitve(STRING(lowercaseStr), ent->GetEntityName()))
                {
                    if (pFilter && !pFilter->ShouldFindEntity(ent)) continue;

                    return ent;
                }
            }
        }
		return nullptr;
	}

    DETOUR_DECL_MEMBER(void, CTFMedigunShield_RemoveShield)
	{
        CTFMedigunShield *shield = reinterpret_cast<CTFMedigunShield *>(this);
        int spawnflags = shield->m_spawnflags;
        //DevMsg("ShieldRemove %d f\n", spawnflags);
        
        if (spawnflags & 2) {
            DevMsg("Spawnflags is 3\n");
            shield->SetModel("models/props_mvm/mvm_player_shield2.mdl");
        }

        if (!(spawnflags & 1)) {
            //DevMsg("Spawnflags is 0\n");
        }
        else{
            //DevMsg("Spawnflags is not 0\n");
            shield->SetBlocksLOS(false);
            return;
        }

        
		DETOUR_MEMBER_CALL(CTFMedigunShield_RemoveShield)();
	}

    DETOUR_DECL_MEMBER(void, CTFMedigunShield_UpdateShieldPosition)
	{   
		DETOUR_MEMBER_CALL(CTFMedigunShield_UpdateShieldPosition)();
	}

    DETOUR_DECL_MEMBER(void, CTFMedigunShield_ShieldThink)
	{
        
		DETOUR_MEMBER_CALL(CTFMedigunShield_ShieldThink)();
	}
    
    RefCount rc_CTriggerHurt_HurtEntity;
    DETOUR_DECL_MEMBER(bool, CTriggerHurt_HurtEntity, CBaseEntity *other, float damage)
	{
        SCOPED_INCREMENT(rc_CTriggerHurt_HurtEntity);
		return DETOUR_MEMBER_CALL(CTriggerHurt_HurtEntity)(other, damage);
	}
    
    RefCount rc_CBaseEntity_TakeDamage;
    DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, CTakeDamageInfo &info)
	{
        SCOPED_INCREMENT(rc_CBaseEntity_TakeDamage);
		//DevMsg("Take damage damage %f\n", info.GetDamage());
        if (rc_CTriggerHurt_HurtEntity) {
            auto owner = info.GetAttacker()->GetOwnerEntity();
            if (owner != nullptr && owner->IsPlayer()) {
                info.SetAttacker(owner);
                info.SetInflictor(owner);
            }
        }
        CBaseEntity *entity = reinterpret_cast<CBaseEntity *>(this);
        bool alive = entity->IsAlive();
        int health_pre = entity->GetHealth();
		auto damage = DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(info);
        if (damage != 0 && health_pre - entity->GetHealth() != 0) {
            variant_t variant;
            variant.SetInt(health_pre - entity->GetHealth());
            entity->FireCustomOutput<"ondamagereceived">(info.GetAttacker() != nullptr ? info.GetAttacker() : entity, entity, variant);
        }
        else {
            variant_t variant;
            variant.SetInt(info.GetDamage());
            entity->FireCustomOutput<"ondamageblocked">(info.GetAttacker() != nullptr ? info.GetAttacker() : entity, entity, variant);
        }
        if (alive && !entity->IsAlive()) {
            variant_t variant;
            variant.SetInt(damage);
            entity->FireCustomOutput<"ondeath">(info.GetAttacker() != nullptr ? info.GetAttacker() : entity, entity, variant);
        }
        return damage;
	}

    DETOUR_DECL_MEMBER(int, CBaseCombatCharacter_OnTakeDamage, CTakeDamageInfo &info)
	{

        info.SetDamage(-100);
        return DETOUR_MEMBER_CALL(CBaseCombatCharacter_OnTakeDamage)(info);
    }

    DETOUR_DECL_MEMBER(void, CBaseObject_InitializeMapPlacedObject)
	{
        DETOUR_MEMBER_CALL(CBaseObject_InitializeMapPlacedObject)();
    
        auto sentry = reinterpret_cast<CBaseObject *>(this);
        variant_t variant;
        sentry->ReadKeyField("spawnflags", &variant);
		int spawnflags = variant.Int();

        if (spawnflags & 64) {
			sentry->SetModelScale(0.75f);
			sentry->m_bMiniBuilding = true;
	        sentry->SetHealth(sentry->GetHealth() * 0.66f);
            sentry->SetMaxHealth(sentry->GetMaxHealth() * 0.66f);
            sentry->m_nSkin += 2;
            sentry->SetBodygroup( sentry->FindBodygroupByName( "mini_sentry_light" ), 1 );
		}
	}

    DETOUR_DECL_MEMBER(void, CBasePlayer_CommitSuicide, bool explode , bool force)
	{
        auto player = reinterpret_cast<CBasePlayer *>(this);
        // No commit suicide if the camera is active
        CBaseEntity *view = player->m_hViewEntity;
        if (rtti_cast<CTriggerCamera *>(view) != nullptr) {
            return;
        }
        DETOUR_MEMBER_CALL(CBasePlayer_CommitSuicide)(explode, force);
	}

	DETOUR_DECL_STATIC(CTFDroppedWeapon *, CTFDroppedWeapon_Create, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner, const char *pszModelName, const CEconItemView *pItemView)
	{
		// this is really ugly... we temporarily override m_bPlayingMannVsMachine
		// because the alternative would be to make a patch
		
		bool is_mvm_mode = TFGameRules()->IsMannVsMachineMode();

		if (allow_create_dropped_weapon) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		}
		
		auto result = DETOUR_STATIC_CALL(CTFDroppedWeapon_Create)(vecOrigin, vecAngles, pOwner, pszModelName, pItemView);
		
		if (allow_create_dropped_weapon) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(is_mvm_mode);
		}
		
		return result;
	}

    DETOUR_DECL_MEMBER(void, CBaseEntity_UpdateOnRemove)
	{
		auto entity = reinterpret_cast<CBaseEntity *>(this);

        auto name = STRING(entity->GetEntityName());
		if (name[0] == '!' && name[1] == '$') {
            variant_t variant;
            entity->m_OnUser4->FireOutput(variant, entity, entity);
        }
        
        variant_t variant;
        variant.SetInt(entity->entindex());
        entity->FireCustomOutput<"onkilled">(entity, entity, variant);

		DETOUR_MEMBER_CALL(CBaseEntity_UpdateOnRemove)();
	}

    CBaseEntity *parse_ent = nullptr;
    DETOUR_DECL_STATIC(bool, ParseKeyvalue, void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue)
	{
		bool result = DETOUR_STATIC_CALL(ParseKeyvalue)(pObject, pFields, iNumFields, szKeyName, szValue);
        if (!result && szKeyName[0] == '$') {
            ParseCustomOutput(parse_ent, szKeyName + 1, szValue);
            result = true;
        }
        return result;
	}

    DETOUR_DECL_MEMBER(bool, CBaseEntity_KeyValue, const char *szKeyName, const char *szValue)
	{
        parse_ent = reinterpret_cast<CBaseEntity *>(this);
        if (cvar_fast_lookup.GetBool()) {
            if (FStrEq(szKeyName, "targetname")) {
                char *lowercase = stackalloc(strlen(szValue) + 1);
                StrLowerCopy(szValue, lowercase);
                parse_ent->SetName(AllocPooledString(lowercase));
                return true;
            }
            else if (FStrEq(szKeyName, "classname")) {
                char *lowercase = stackalloc(strlen(szValue) + 1);
                StrLowerCopy(szValue, lowercase);
                parse_ent->SetClassname(AllocPooledString(lowercase));
                return true;
            }
        }
        return DETOUR_MEMBER_CALL(CBaseEntity_KeyValue)(szKeyName, szValue);
	}

    DETOUR_DECL_MEMBER(void, CBaseEntity_PostConstructor, const char *classname)
	{
        if (cvar_fast_lookup.GetBool() && !IsStrLower(classname)) {
            char *lowercase = stackalloc(strlen(classname) + 1);
            StrLowerCopy(classname, lowercase, 255);
            parse_ent->SetClassname(AllocPooledString(lowercase));
        }
        return DETOUR_MEMBER_CALL(CBaseEntity_PostConstructor)(classname);
    }

    PooledString filter_keyvalue_class("$filter_keyvalue");
    PooledString filter_variable_class("$filter_variable");
    PooledString filter_datamap_class("$filter_datamap");
    PooledString filter_sendprop_class("$filter_sendprop");
    PooledString filter_proximity_class("$filter_proximity");
    PooledString filter_bbox_class("$filter_bbox");
    PooledString filter_itemname_class("$filter_itemname");
    PooledString filter_specialdamagetype_class("$filter_specialdamagetype");
    
    PooledString empty("");
    PooledString less("less than");
    PooledString equal("equal");
    PooledString greater("greater than");
    PooledString less_or_equal("less than or equal");
    PooledString greater_or_equal("greater than or equal");

    DETOUR_DECL_MEMBER(bool, CBaseFilter_PassesFilterImpl, CBaseEntity *pCaller, CBaseEntity *pEntity)
	{
        auto filter = reinterpret_cast<CBaseEntity *>(this);
        const char *classname = filter->GetClassname();
        if (classname[0] == '$') {
            if (classname == filter_variable_class || classname == filter_datamap_class || classname == filter_sendprop_class || classname == filter_keyvalue_class) {
                GetInputType type = KEYVALUE;

                if (classname == filter_variable_class) {
                    type = VARIABLE;
                } else if (classname == filter_datamap_class) {
                    type = DATAMAP;
                } else if (classname == filter_sendprop_class) {
                    type = SENDPROP;
                }

                const char *name = filter->GetCustomVariable<"name">();
                variant_t valuecmp;
                filter->GetCustomVariableVariant<"value">(valuecmp);
                const char *compare = filter->GetCustomVariable<"compare">();

                variant_t variable; 
                bool found = GetEntityVariable(pEntity, type, name, variable);

                if (found) {
                    if (valuecmp.FieldType() == FIELD_STRING) {
                        ParseNumberOrVectorFromString(valuecmp.String(), valuecmp);
                    }
                    if (variable.FieldType() == FIELD_STRING) {
                        ParseNumberOrVectorFromString(variable.String(), variable);
                    }
                    if (valuecmp.FieldType() == FIELD_INTEGER && variable.FieldType() == FIELD_FLOAT) {
                        valuecmp.Convert(FIELD_FLOAT);
                    }
                    else if (valuecmp.FieldType() == FIELD_FLOAT && variable.FieldType() == FIELD_INTEGER) {
                        variable.Convert(FIELD_FLOAT);
                    }
                    
                    
                    if (compare == nullptr || compare == empty || compare == equal) {
                        if (valuecmp.FieldType() == FIELD_INTEGER) {
                            return valuecmp.Int() == variable.Int();
                        }
                        else if (valuecmp.FieldType() == FIELD_FLOAT) {
                            return valuecmp.Float() == variable.Float();
                        }
                        else if (valuecmp.FieldType() == FIELD_STRING) {
                            return valuecmp.String() == variable.String();
                        }
                        else if (valuecmp.FieldType() == FIELD_VECTOR) {
                            Vector vec1;
                            Vector vec2;
                            valuecmp.Vector3D(vec1);
                            variable.Vector3D(vec2);
                            return vec1 == vec2;
                        }
                        else if (valuecmp.FieldType() == FIELD_EHANDLE) {
                            return valuecmp.Entity() == variable.Entity();
                        }
                    }
                    else {
                        if (compare == less) {
                            return valuecmp.FieldType() == FIELD_FLOAT ? valuecmp.Float() < variable.Float() : valuecmp.Int() < variable.Int();
                        }
                        else if (compare == less_or_equal) {
                            return valuecmp.FieldType() == FIELD_FLOAT ? valuecmp.Float() <= variable.Float() : valuecmp.Int() <= variable.Int();
                        }
                        else if (compare == greater) {
                            return valuecmp.FieldType() == FIELD_FLOAT ? valuecmp.Float() > variable.Float() : valuecmp.Int() > variable.Int();
                        }
                        else if (compare == greater_or_equal) {
                            return valuecmp.FieldType() == FIELD_FLOAT ? valuecmp.Float() >= variable.Float() : valuecmp.Int() >= variable.Int();
                        }
                    }
                }
                return false;
            }
            else if(classname == filter_proximity_class) {
                const char *target = filter->GetCustomVariable<"target">();
                float range = filter->GetCustomVariableFloat<"range">();
                range *= range;
                Vector center;
                if (!UTIL_StringToVectorAlt(center, target)) {
                    CBaseEntity *ent = servertools->FindEntityByName(nullptr, target);
                    if (ent == nullptr) return false;

                    center = ent->GetAbsOrigin();
                }

                return center.DistToSqr(pEntity->GetAbsOrigin()) <= range;
            }
            else if(classname == filter_bbox_class) {
                const char *target = filter->GetCustomVariable<"target">();

                Vector min = filter->GetCustomVariableVector<"min">();
                Vector max = filter->GetCustomVariableVector<"max">();

                Vector center;
                if (!UTIL_StringToVectorAlt(center, target)) {
                    CBaseEntity *ent = servertools->FindEntityByName(nullptr, target);
                    if (ent == nullptr) return false;

                    center = ent->GetAbsOrigin();
                }

                return pEntity->GetAbsOrigin().WithinAABox(min + center, max + center);
            }
        }
        return DETOUR_MEMBER_CALL(CBaseFilter_PassesFilterImpl)(pCaller, pEntity);
	}

    void OnCameraRemoved(CTriggerCamera *camera)
    {
        if (camera->m_spawnflags & 512) {
            ForEachTFPlayer([&](CTFPlayer *player) {
                if (player->IsBot())
                    return;
                else {
                    camera->m_hPlayer = player;
                    camera->Disable();
                    player->m_takedamage = player->IsObserver() ? 0 : 2;
                }
            });
        }
    }

    DETOUR_DECL_MEMBER(void, CTriggerCamera_D0)
	{
        OnCameraRemoved(reinterpret_cast<CTriggerCamera *>(this));
    }

    DETOUR_DECL_MEMBER(void, CTriggerCamera_D2)
	{
        OnCameraRemoved(reinterpret_cast<CTriggerCamera *>(this));
    }

    DETOUR_DECL_MEMBER(void, CFuncRotating_InputStop, inputdata_t *inputdata)
	{
        auto data = GetExtraFuncRotatingData(reinterpret_cast<CFuncRotating *>(this), false);
        if (data != nullptr) {
            data->m_hRotateTarget = nullptr;
        }
        DETOUR_MEMBER_CALL(CFuncRotating_InputStop)(inputdata);
    }

    THINK_FUNC_DECL(DetectorTick)
    {
        auto data = GetExtraTriggerDetectorData(this);
        auto trigger = reinterpret_cast<CBaseTrigger *>(this);
        // The target was killed
        if (data->m_bHasTarget && data->m_hLastTarget == nullptr) {
            variant_t variant;
            this->FireCustomOutput<"onlosttargetall">(nullptr, this, variant);
        }

        // Find nearest target entity
        bool los = trigger->GetCustomVariableFloat<"checklineofsight">() != 0;

        float minDistance = trigger->GetCustomVariableFloat<"radius">(65000);
        minDistance *= minDistance;

        float fov = FastCos(DEG2RAD(Clamp(trigger->GetCustomVariableFloat<"fov">(180.0f), 0.0f, 180.0f)));

        CBaseEntity *nearestEntity = nullptr;
        touchlink_t *root = reinterpret_cast<touchlink_t *>(this->GetDataObject(1));
        if (root) {
            touchlink_t *link = root->nextLink;

            // Keep target mode, aim at entity until its dead
            if (data->m_hLastTarget != nullptr && trigger->GetCustomVariableFloat<"keeptarget">() != 0) {
                bool inTouch = false;
                while (link != root) {
                    if (link->entityTouched == data->m_hLastTarget) {
                        inTouch = true;
                        break;
                    }
                    link = link->nextLink;
                }
                
                if (inTouch) {
                    Vector delta = data->m_hLastTarget->EyePosition() - this->GetAbsOrigin();
                    Vector fwd;
                    AngleVectors(this->GetAbsAngles(), &fwd);
                    float distance = delta.LengthSqr();
                    if (distance < minDistance && DotProduct(delta.Normalized(), fwd.Normalized()) > fov) {
                        bool inSight = true;
                        if (los) {
                            trace_t tr;
                            UTIL_TraceLine(data->m_hLastTarget->EyePosition(), this->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, data->m_hLastTarget, COLLISION_GROUP_NONE, &tr);
                            inSight = !tr.DidHit() || tr.m_pEnt == this;
                        }
						
						if (inSight) {
                            nearestEntity = data->m_hLastTarget;
                        }
                    }
                    
                }
            }

            // Pick closest target
            if (nearestEntity == nullptr) {
                link = root->nextLink;
                while (link != root) {
                    CBaseEntity *entity = link->entityTouched;

                    if ((entity != nullptr) && trigger->PassesTriggerFilters(entity)) {
                        Vector delta = entity->EyePosition() - this->GetAbsOrigin();
                        Vector fwd;
                        AngleVectors(this->GetAbsAngles(), &fwd);
                        float distance = delta.LengthSqr();
                        if (distance < minDistance && DotProduct(delta.Normalized(), fwd.Normalized()) > fov) {
                            bool inSight = true;
                            if (los) {
                                trace_t tr;
                                UTIL_TraceLine(entity->EyePosition(), this->GetAbsOrigin(), MASK_SOLID_BRUSHONLY, entity, COLLISION_GROUP_NONE, &tr);
                                inSight = !tr.DidHit() || tr.m_pEnt == this;
                            }
                            
                            if (inSight) {
                                minDistance = distance;
                                nearestEntity = entity;
                            }
                        }
                    }

                    link = link->nextLink;
                }
            }
        }

        if (nearestEntity != data->m_hLastTarget) {
            variant_t variant;
            if (nearestEntity != nullptr) {
                if (data->m_hLastTarget != nullptr) {
                    this->FireCustomOutput<"onlosttarget">(data->m_hLastTarget, this, variant);
                }
                this->FireCustomOutput<"onnewtarget">(nearestEntity, this, variant);
            }
            else {
               this->FireCustomOutput<"onlosttargetall">(data->m_hLastTarget, this, variant);
            }
        }

        data->m_hLastTarget = nearestEntity;
        data->m_bHasTarget = nearestEntity != nullptr;

        this->SetNextThink(gpGlobals->curtime, "DetectorTick");
    }

    DETOUR_DECL_MEMBER(void, CBaseTrigger_Activate)
	{
        auto trigger = reinterpret_cast<CBaseEntity *>(this);
        if (trigger->GetClassname() == trigger_detector_class) {
            auto data = GetExtraTriggerDetectorData(trigger);
            THINK_FUNC_SET(trigger, DetectorTick, gpGlobals->curtime);
        }
        DETOUR_MEMBER_CALL(CBaseTrigger_Activate)();
    }

    THINK_FUNC_DECL(WeaponSpawnerTick)
    {
        auto data = GetExtraData<ExtraEntityDataWeaponSpawner>(this);
        
        this->SetNextThink(gpGlobals->curtime + 0.1f, "WeaponSpawnerTick");
    }

    DETOUR_DECL_MEMBER(void, CPointTeleport_Activate)
	{
        auto spawner = reinterpret_cast<CBaseEntity *>(this);
        if (spawner->GetClassname() == weapon_spawner_classname) {
            THINK_FUNC_SET(spawner, WeaponSpawnerTick, gpGlobals->curtime + 0.1f);
        }
        DETOUR_MEMBER_CALL(CPointTeleport_Activate)();
    }

	DETOUR_DECL_MEMBER(void, CTFDroppedWeapon_InitPickedUpWeapon, CTFPlayer *player, CTFWeaponBase *weapon)
	{
        auto drop = reinterpret_cast<CTFDroppedWeapon *>(this);
        auto data = drop->GetEntityModule<DroppedWeaponModule>("droppedweapon");
        if (data != nullptr) {
            auto spawner = data->m_hWeaponSpawner;
            if (spawner != nullptr) {
                variant_t variant;
                spawner->FireCustomOutput<"onpickup">(player, spawner, variant);
            }
            if (data->ammo != -1) {
                player->SetAmmoCount(data->ammo, weapon->GetPrimaryAmmoType());
            }
            if (data->clip != -1) {
                weapon->m_iClip1 = data->clip;
            }
            CWeaponMedigun *medigun = rtti_cast<CWeaponMedigun*>(weapon);
            if (medigun != nullptr && data->charge != FLT_MIN) {
                medigun->SetCharge(data->charge);
            }
            if (data->energy != FLT_MIN) {
                weapon->m_flEnergy = data->energy;
            }
        }
        else {
            DETOUR_MEMBER_CALL(CTFDroppedWeapon_InitPickedUpWeapon)(player, weapon);
        }
		
	}
    
    CBaseEntity *filter_entity = nullptr;
    float filter_total_multiplier = 1.0f;
    DETOUR_DECL_MEMBER(bool, CBaseEntity_PassesDamageFilter, CTakeDamageInfo &info)
	{
        filter_entity = reinterpret_cast<CBaseEntity *>(this);
        filter_total_multiplier = 1.0f;
        auto ret = DETOUR_MEMBER_CALL(CBaseFilter_PassesDamageFilter)(info);
        if (filter_total_multiplier != 1.0f) {
            if (filter_total_multiplier > 0)
                info.SetDamage(info.GetDamage() * filter_total_multiplier);
            else {
                filter_entity->TakeHealth(info.GetDamage() * -filter_total_multiplier, DMG_GENERIC);
                info.SetDamage(0);
            }
        }
        filter_entity = nullptr;
        return ret;
    }
    
    DETOUR_DECL_MEMBER(bool, CBaseFilter_PassesDamageFilter, CTakeDamageInfo &info)
	{
        auto ret = DETOUR_MEMBER_CALL(CBaseFilter_PassesDamageFilter)(info);
        auto filter = reinterpret_cast<CBaseEntity *>(this);

        float multiplier = filter->GetCustomVariableFloat<"multiplier">();
        if (multiplier != 0.0f) {
            if (ret && rc_CBaseEntity_TakeDamage == 1) {
                filter_total_multiplier *= multiplier;
            }
            return true;
        }
        
        return ret;
    }

    DETOUR_DECL_MEMBER(bool, CBaseFilter_PassesDamageFilterImpl, CTakeDamageInfo &info)
	{
        auto filter = reinterpret_cast<CBaseFilter *>(this);
        const char *classname = filter->GetClassname();
        if (classname[0] == '$') {
            if (classname == filter_itemname_class && info.GetWeapon() != nullptr && info.GetWeapon()->MyCombatWeaponPointer() != nullptr) {
                return FStrEq(filter->GetCustomVariable<"item">(), info.GetWeapon()->MyCombatWeaponPointer()->GetItem()->GetItemDefinition()->GetName());
            }
            if (classname == filter_specialdamagetype_class && info.GetWeapon() != nullptr && info.GetWeapon()->MyCombatWeaponPointer() != nullptr) {
				float iDmgType = 0;
				CALL_ATTRIB_HOOK_FLOAT_ON_OTHER(info.GetWeapon(), iDmgType, special_damage_type);
                return iDmgType == filter->GetCustomVariableFloat<"type">();
            }
        }
        return DETOUR_MEMBER_CALL(CBaseFilter_PassesDamageFilterImpl)(info);
    }

    DETOUR_DECL_MEMBER(void, CEventQueue_AddEvent_CBaseEntity, CBaseEntity *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID)
	{
        if (fireDelay == -1.0f) {
            if (target != nullptr) {
                target->AcceptInput(targetInput, pActivator, pCaller, Value, outputID);
            }
            return;
        }
        DETOUR_MEMBER_CALL(CEventQueue_AddEvent_CBaseEntity)(target, targetInput, Value, fireDelay, pActivator, pCaller, outputID);
    }


    DETOUR_DECL_MEMBER(void, CEventQueue_AddEvent, const char *target, const char *targetInput, variant_t Value, float fireDelay, CBaseEntity *pActivator, CBaseEntity *pCaller, int outputID)
	{
        if (fireDelay == -1.0f) {
            bool found = false;
            for (CBaseEntity *targetEnt = nullptr; (targetEnt = servertools->FindEntityByName(targetEnt, target, pCaller, pActivator, pCaller)) != nullptr ;) {
                found = true;
                targetEnt->AcceptInput(targetInput, pActivator, pCaller, Value, outputID);
            }
            if (!found) {
                for (CBaseEntity *targetEnt = nullptr; (targetEnt = servertools->FindEntityByClassname(targetEnt, target)) != nullptr ;) {
                    targetEnt->AcceptInput(targetInput, pActivator, pCaller, Value, outputID);
                }
            }
            return;
        }
        DETOUR_MEMBER_CALL(CEventQueue_AddEvent)(target, targetInput, Value, fireDelay, pActivator, pCaller, outputID);
    }

	DETOUR_DECL_STATIC(CBaseEntity *, CreateEntityByName, const char *className, int iForceEdictIndex)
	{
		auto ret = DETOUR_STATIC_CALL(CreateEntityByName)(className, iForceEdictIndex);
        if (ret != nullptr && !entity_listeners.empty()) {
            auto classNameOur = MAKE_STRING(ret->GetClassname());
            for (auto it = entity_listeners.begin(); it != entity_listeners.end();) {
                auto &pair = *it;
                if (pair.first == classNameOur)
                {
                    if (pair.second == nullptr) {
                        it = entity_listeners.erase(it);
                        continue;
                    }
                    variant_t variant;
                    pair.second->FireCustomOutput<"onentityspawned">(ret, pair.second, variant);
                }
                it++;
            }
        }
        
        return ret;
	}

    DETOUR_DECL_MEMBER(void, CEventAction_CEventAction, const char *name)
	{
        //TIME_SCOPE2(cevent)
        char *newname = nullptr;
        const char *findthis = FindCaseSensitive(name, "$$=");
        if (findthis != nullptr) {
            char c;
            newname = alloca(strlen(name)+1);
            strcpy(newname, name);
            int scope = 0;
            char *change = newname + (findthis - name);
            bool instring = false;
            while((c = (*change++)) != '\0') {

                if (c == '\'') {
                    if (instring && *(change-2) != '\\') {
                        instring = false;
                    }
                    else if (!instring) {
                        instring = true;
                    }
                }
                if (c == '(') {
                    scope++;
                }
                else if (c == ',' && (scope > 0 || instring)) {
                    *(change-1) = '\2';
                }
                else if (c == ')') {
                    scope--;
                }
            }
            name = newname;
        }
        //Msg("Event action post %s\n", name);
        DETOUR_MEMBER_CALL(CEventAction_CEventAction)(name);
    }

    class CMod : public IMod, IModCallbackListener
	{
	public:
		CMod() : IMod("Etc:Mapentity_Additions")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_InputIgnitePlayer, "CTFPlayer::InputIgnitePlayer");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_AcceptInput, "CBaseEntity::AcceptInput");
			MOD_ADD_DETOUR_MEMBER(CTFGameRules_CleanUpMap, "CTFGameRules::CleanUpMap");
			MOD_ADD_DETOUR_MEMBER(CTFMedigunShield_RemoveShield, "CTFMedigunShield::RemoveShield");
			MOD_ADD_DETOUR_MEMBER(CTriggerHurt_HurtEntity, "CTriggerHurt::HurtEntity");
			MOD_ADD_DETOUR_MEMBER(CGlobalEntityList_FindEntityByName, "CGlobalEntityList::FindEntityByName");
			MOD_ADD_DETOUR_MEMBER(CGlobalEntityList_FindEntityByClassname, "CGlobalEntityList::FindEntityByClassname");
            MOD_ADD_DETOUR_MEMBER_PRIORITY(CBaseEntity_TakeDamage, "CBaseEntity::TakeDamage", HIGHEST);
            MOD_ADD_DETOUR_MEMBER(CBaseObject_InitializeMapPlacedObject, "CBaseObject::InitializeMapPlacedObject");
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_CommitSuicide, "CBasePlayer::CommitSuicide");
			MOD_ADD_DETOUR_STATIC(CTFDroppedWeapon_Create, "CTFDroppedWeapon::Create");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_UpdateOnRemove, "CBaseEntity::UpdateOnRemove");
            MOD_ADD_DETOUR_STATIC(ParseKeyvalue, "ParseKeyvalue");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_KeyValue, "CBaseEntity::KeyValue");
            MOD_ADD_DETOUR_MEMBER(CBaseFilter_PassesFilterImpl, "CBaseFilter::PassesFilterImpl");
            MOD_ADD_DETOUR_MEMBER(CBaseFilter_PassesDamageFilter, "CBaseFilter::PassesDamageFilter");
            MOD_ADD_DETOUR_MEMBER(CBaseFilter_PassesDamageFilterImpl, "CBaseFilter::PassesDamageFilterImpl");
            MOD_ADD_DETOUR_MEMBER(CFuncRotating_InputStop, "CFuncRotating::InputStop");
            MOD_ADD_DETOUR_MEMBER(CBaseTrigger_Activate, "CBaseTrigger::Activate");
            MOD_ADD_DETOUR_MEMBER(CPointTeleport_Activate, "CPointTeleport::Activate");
            MOD_ADD_DETOUR_MEMBER(CTFDroppedWeapon_InitPickedUpWeapon, "CTFDroppedWeapon::InitPickedUpWeapon");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_PassesDamageFilter, "CBaseEntity::PassesDamageFilter");

            // Execute -1 delay events immediately
            MOD_ADD_DETOUR_MEMBER(CEventQueue_AddEvent_CBaseEntity, "CEventQueue::AddEvent [CBaseEntity]");
            MOD_ADD_DETOUR_MEMBER(CEventQueue_AddEvent, "CEventQueue::AddEvent");
            

            // Fix camera despawn bug
            MOD_ADD_DETOUR_MEMBER(CTriggerCamera_D0, "~CTriggerCamera [D0]");
            MOD_ADD_DETOUR_MEMBER(CTriggerCamera_D2, "~CTriggerCamera [D2]");
            
            MOD_ADD_DETOUR_MEMBER(CEventAction_CEventAction, "CEventAction::CEventAction [C2]");
    
    
		//	MOD_ADD_DETOUR_MEMBER(CTFMedigunShield_UpdateShieldPosition, "CTFMedigunShield::UpdateShieldPosition");
		//	MOD_ADD_DETOUR_MEMBER(CTFMedigunShield_ShieldThink, "CTFMedigunShield::ShieldThink");
		//	MOD_ADD_DETOUR_MEMBER(CBaseGrenade_SetDamage, "CBaseGrenade::SetDamage");
		}

        virtual bool OnLoad() override
		{
            ActivateLoadedInput();
            if (servertools->GetEntityFactoryDictionary()->FindFactory("$filter_keyvalue") == nullptr) {
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_keyvalue");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_variable");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_datamap");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_sendprop");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_proximity");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_bbox");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_itemname");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("filter_base"), "$filter_specialdamagetype");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("trigger_multiple"), "$trigger_detector");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("point_teleport"), "$weapon_spawner");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("math_counter"), "$math_vector");
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("math_counter"), "$entity_spawn_detector");
            }

			return true;
		}
        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void LevelInitPreEntity() override
        { 
            sendproxies = gamedll->GetStandardSendProxies();
            send_prop_cache.clear();
            datamap_cache.clear();
            entity_listeners.clear();
        }
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_mapentity_additions", "0", FCVAR_NOTIFY,
		"Mod: tell maps that sigsegv extension is loaded",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
