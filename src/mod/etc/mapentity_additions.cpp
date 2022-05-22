#include "mod.h"
#include "stub/baseentity.h"
#include "stub/entities.h"
#include "stub/gamerules.h"
#include "stub/populators.h"
#include "stub/tfbot.h"
#include "stub/nextbot_cc.h"
#include "stub/tf_shareddefs.h"
#include "stub/misc.h"
#include "stub/strings.h"
#include "stub/server.h"
#include "stub/objects.h"
#include "stub/extraentitydata.h"
#include "mod/pop/common.h"
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
#include "mod/etc/mapentity_additions.h"
#include "stub/trace.h"

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

    std::vector<ServerClass *> send_prop_cache_classes;
    std::vector<std::pair<std::vector<std::string>, std::vector<PropCacheEntry>>> send_prop_cache;

    std::vector<datamap_t *> datamap_cache_classes;
    std::vector<std::pair<std::vector<std::string>, std::vector<PropCacheEntry>>> datamap_cache;

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

    bool SetCustomVariable(CBaseEntity *entity, std::string &key, variant_t &value, bool create = true, bool find = true, int vecIndex = -1)
    {
        if (value.FieldType() == FIELD_STRING) {
            ParseNumberOrVectorFromString(value.String(), value);
        }

        if (vecIndex != -1) {
            Vector vec;
            entity->GetCustomVariableByText(key.c_str(), value);
            value.Vector3D(vec);
            vec[vecIndex] = value.Float();
            value.SetVector3D(vec);
        }

        
        auto ret = entity->SetCustomVariable(key.c_str(), value, create, find);
        return ret;
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
                        auto prop_array = s_table->GetProp(i + 1);
                        if (prop_array != nullptr && prop_array->GetType() == DPT_Array && index < prop_array->GetNumElements()) {
                            off += prop_array->GetElementStride() * index;
                        }
                    }
                }
                else {
                    if (s_prop->IsInsideArray()) {
                        off -= s_prop->GetOffset();
                        continue;
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

    bool ReadArrayIndexFromString(std::string &name, int &arrayPos, int &vecAxis)
    {
        size_t arrayStr = name.find('$');
        const char *vecChar = nullptr;
        vecAxis = -1;
        arrayPos = 0;
        if (arrayStr != std::string::npos) {
            StringToIntStrict(name.c_str() + arrayStr + 1, arrayPos, 0, &vecChar);
            name.resize(arrayStr);
            if (vecChar != nullptr) {
                switch (*vecChar) {
                    case 'x': case 'X': vecAxis = 0; break;
                    case 'y': case 'Y': vecAxis = 1; break;
                    case 'z': case 'Z': vecAxis = 2; break;
                }
            }
            return true;
        }
        return false;
    }


    void *stringSendProxy = nullptr;
    CStandardSendProxies* sendproxies = nullptr;
    void GetSendPropInfo(SendProp *prop, PropCacheEntry &entry, int offset) {
        if (prop == nullptr) return;
        
        entry.offset = offset;
        if (prop->GetType() == DPT_Array) {
            entry.offset += prop->GetArrayProp()->GetOffset();
            entry.elementStride = prop->GetElementStride();
            entry.arraySize = prop->GetNumElements();
        }
        else if (prop->GetDataTable() != nullptr) {
            entry.arraySize = prop->GetDataTable()->GetNumProps();
            if (entry.arraySize > 1) {
                entry.elementStride = prop->GetDataTable()->GetProp(1)->GetOffset() - prop->GetDataTable()->GetProp(0)->GetOffset();
            }
        }

        if (prop->GetArrayProp() != nullptr) {
            prop = prop->GetArrayProp();
        }

        auto propType = prop->GetType();
        if (propType == DPT_Int) {
            
            if (prop->m_nBits == 21 && (prop->GetFlags() & SPROP_UNSIGNED)) {
                entry.fieldType = FIELD_EHANDLE;
            }
            else {
                auto proxyfn = prop->GetProxyFn();
                if (proxyfn == sendproxies->m_Int8ToInt32 || proxyfn == sendproxies->m_UInt8ToInt32) {
                    entry.fieldType = FIELD_CHARACTER;
                }
                else if (proxyfn == sendproxies->m_Int16ToInt32 || proxyfn == sendproxies->m_UInt16ToInt32) {
                    entry.fieldType = FIELD_SHORT;
                }
                else {
                    entry.fieldType = FIELD_INTEGER;
                }
            }
        }
        else if (propType == DPT_Float) {
            entry.fieldType = FIELD_FLOAT;
        }
        else if (propType == DPT_String) {
            auto proxyfn = prop->GetProxyFn();
            if (proxyfn != stringSendProxy) {
                entry.fieldType = FIELD_STRING;
            }
            else {
                entry.fieldType = FIELD_CHARACTER;
            }
        }
        else if (propType == DPT_Vector || propType == DPT_VectorXY) {
            entry.fieldType = FIELD_VECTOR;
        }
    }

    PropCacheEntry &GetSendPropOffset(ServerClass *serverClass, std::string &name) {
        TIME_SCOPE2(Sendproptime);
        size_t classIndex = 0;
        for (; classIndex < send_prop_cache_classes.size(); classIndex++) {
            if (send_prop_cache_classes[classIndex] == serverClass) {
                break;
            }
        }
        if (classIndex >= send_prop_cache_classes.size()) {
            send_prop_cache_classes.push_back(serverClass);
            send_prop_cache.emplace_back();
        }
        auto &pair = send_prop_cache[classIndex];
        auto &names = pair.first;

        int nameCount = names.size();
        for (size_t i = 0; i < nameCount; i++ ) {
            if (names[i] == name) {
                return pair.second[i];
            }
        }

        int offset = 0;
        SendProp *prop = nullptr;
        FindSendProp(offset,serverClass->m_pTable, name.c_str(), prop);

        PropCacheEntry entry;
        GetSendPropInfo(prop, entry, offset);
        
        names.push_back(name);
        pair.second.push_back(entry);
        return pair.second.back();
    }

    void WriteProp(CBaseEntity *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis)
    {
        if (entry.offset > 0) {
            int offset = entry.offset + arrayPos * entry.elementStride;
            fieldtype_t fieldType = entry.fieldType;
            if (vecAxis != -1) {
                fieldType = FIELD_FLOAT;
                offset += vecAxis * sizeof(float);
            }

            if (fieldType == FIELD_CHARACTER && entry.arraySize > 1) {
                V_strncpy(((char*)entity) + offset, variant.String(), entry.arraySize);
            }
            else {
                variant.Convert(fieldType);
                variant.SetOther(((char*)entity) + offset);
            }
        }
    }

    void ReadProp(CBaseEntity *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis)
    {
        if (entry.offset > 0) {
            int offset = entry.offset + arrayPos * entry.elementStride;
            fieldtype_t fieldType = entry.fieldType;
            if (vecAxis != -1) {
                fieldType = FIELD_FLOAT;
                offset += vecAxis * sizeof(float);
            }

            if (fieldType == FIELD_CHARACTER && entry.arraySize > 1) {
                variant.SetString(AllocPooledString(((char*)entity) + offset));
            }
            else {
                variant.Set(fieldType, ((char*)entity) + offset);
            }
            if (fieldType == FIELD_POSITION_VECTOR) {
                variant.Convert(FIELD_VECTOR);
            }
            else if (fieldType == FIELD_CLASSPTR) {
                variant.Convert(FIELD_EHANDLE);
            }
            else if ((fieldType == FIELD_CHARACTER && entry.arraySize == 1) || (fieldType == FIELD_SHORT)) {
                variant.Convert(FIELD_INTEGER);
            }
        }
    }

    void GetDataMapInfo(typedescription_t &desc, PropCacheEntry &entry) {
        entry.fieldType = desc.fieldType;
        entry.offset = desc.fieldOffset[ TD_OFFSET_NORMAL ];
        
        entry.arraySize = (int)desc.fieldSize;
        entry.elementStride = (desc.fieldSizeInBytes / desc.fieldSize);
    }

    PropCacheEntry &GetDataMapOffset(datamap_t *datamap, std::string &name) {
        
        size_t classIndex = 0;
        for (; classIndex < datamap_cache_classes.size(); classIndex++) {
            if (datamap_cache_classes[classIndex] == datamap) {
                break;
            }
        }
        if (classIndex >= datamap_cache_classes.size()) {
            datamap_cache_classes.push_back(datamap);
            datamap_cache.emplace_back();
        }
        auto &pair = datamap_cache[classIndex];
        auto &names = pair.first;

        int nameCount = names.size();
        for (size_t i = 0; i < nameCount; i++ ) {
            if (names[i] == name) {
                return pair.second[i];
            }
        }

        for (datamap_t *dmap = datamap; dmap != NULL; dmap = dmap->baseMap) {
            // search through all the readable fields in the data description, looking for a match
            for (int i = 0; i < dmap->dataNumFields; i++) {
                if (dmap->dataDesc[i].fieldName != nullptr && (strcmp(dmap->dataDesc[i].fieldName, name.c_str()) == 0 || 
                    ( (dmap->dataDesc[i].flags & (FTYPEDESC_OUTPUT | FTYPEDESC_KEY)) && strcmp(dmap->dataDesc[i].externalName, name.c_str()) == 0))) {
                    PropCacheEntry entry;
                    GetDataMapInfo(dmap->dataDesc[i], entry);

                    names.push_back(name);
                    pair.second.push_back(entry);
                    return pair.second.back();
                }
            }
        }

        names.push_back(name);
        pair.second.push_back({0, FIELD_VOID, 1, 0});
        return pair.second.back();
    }

    void ParseCustomOutput(CBaseEntity *entity, const char *name, const char *value) {
        std::string namestr = name;
        boost::algorithm::to_lower(namestr);
    //  DevMsg("Add custom output %d %s %s\n", entity, namestr.c_str(), value);
        entity->AddCustomOutput(namestr.c_str(), value);
        variant_t variant;
        variant.SetString(AllocPooledString(value));
        SetCustomVariable(entity, namestr, variant);

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

    bool SetEntityVariable(CBaseEntity *entity, GetInputType type, const char *name, variant_t &variable) {

        std::string nameNoArray = name;
        int arrayPos;
        int vecAxis;
        ReadArrayIndexFromString(nameNoArray, arrayPos, vecAxis);
        return SetEntityVariable(entity, type, nameNoArray, variable, arrayPos, vecAxis);
    }

    bool SetEntityVariable(CBaseEntity *entity, GetInputType type, std::string &name, variant_t &variable, int arrayPos, int vecAxis) {
        bool found = false;

        if (type == ANY) {
            found = SetEntityVariable(entity, VARIABLE_NO_CREATE, name, variable, arrayPos, vecAxis) || 
                    SetEntityVariable(entity, DATAMAP_REFRESH, name, variable, arrayPos, vecAxis) ||
                    SetEntityVariable(entity, SENDPROP, name, variable, arrayPos, vecAxis) ||
                    SetEntityVariable(entity, KEYVALUE, name, variable, arrayPos, vecAxis) ||
                    SetEntityVariable(entity, VARIABLE_NO_FIND, name, variable, arrayPos, vecAxis);
        }
        else if (type == VARIABLE || type == VARIABLE_NO_CREATE || type == VARIABLE_NO_FIND) {
            found = SetCustomVariable(entity, name, variable, type != VARIABLE_NO_CREATE, type != VARIABLE_NO_FIND, vecAxis);
        }
        else if (type == KEYVALUE) {
            
            if (vecAxis != -1) {
                Vector vec;
                variant_t variant;
                entity->ReadKeyField(name.c_str(), &variant);
                variable.Vector3D(vec);
                variable.Convert(FIELD_FLOAT);
                vec[vecAxis] = variable.Float();
                variable.SetVector3D(vec);
            }

            found = entity->KeyValue(name.c_str(), variable.String());
        }
        else if (type == DATAMAP || type == DATAMAP_REFRESH) {
            auto &entry = GetDataMapOffset(entity->GetDataDescMap(), name);

            if (entry.offset > 0) {
                WriteProp(entity, entry, variable, arrayPos, vecAxis);

                // Sometimes datamap shares the property with sendprops. In this case, it is better to tell the network state changed
                if (type == DATAMAP_REFRESH) {
                    entity->NetworkStateChanged();
                }
                found = true;
            }
        }
        else if (type == SENDPROP) {
            auto &entry = GetSendPropOffset(entity->GetServerClass(), name);

            if (entry.offset > 0) {
                WriteProp(entity, entry, variable, arrayPos, vecAxis);
                entity->NetworkStateChanged();
                found = true;
            }
        }
        return found;
    }

    bool GetEntityVariable(CBaseEntity *entity, GetInputType type, const char *name, variant_t &variable) {

        std::string nameNoArray = name;
        int arrayPos;
        int vecAxis;
        ReadArrayIndexFromString(nameNoArray, arrayPos, vecAxis);
        return GetEntityVariable(entity, type, nameNoArray, variable, arrayPos, vecAxis);
    }

    bool GetEntityVariable(CBaseEntity *entity, GetInputType type, std::string &name, variant_t &variable, int arrayPos, int vecAxis) {
        bool found = false;

        if (type == ANY) {
            found = GetEntityVariable(entity, VARIABLE, name, variable, arrayPos, vecAxis) ||
                    GetEntityVariable(entity, DATAMAP, name, variable, arrayPos, vecAxis) ||
                    GetEntityVariable(entity, SENDPROP, name, variable, arrayPos, vecAxis);// ||
                    //GetEntityVariable(entity, KEYVALUE, name, variable, arrayPos, vecAxis);
        }
        else if (type == VARIABLE) {
            if (entity->GetCustomVariableByText(name.c_str(), variable)) {
                found = true;
                ConvertToVectorIndex(variable, vecAxis);
            }
        }
        else if (type == KEYVALUE) {
            if (name[0] == '$') {
                std::string varName = name.substr(1);
                return GetEntityVariable(entity, VARIABLE, varName, variable, arrayPos, vecAxis);
            }
            auto &entry = GetDataMapOffset(entity->GetDataDescMap(), name);
            if (entry.offset > 0) {
                ReadProp(entity, entry, variable, arrayPos, vecAxis);
                found = true;
            }
            //found = entity->ReadKeyField(name.c_str(), &variable);
            //ConvertToVectorIndex(variable, vecAxis);
        }
        else if (type == DATAMAP) {
            auto &entry = GetDataMapOffset(entity->GetDataDescMap(), name);
            if (entry.offset > 0) {
                ReadProp(entity, entry, variable, arrayPos, vecAxis);
                found = true;
            }
        }
        else if (type == SENDPROP) {
            auto &entry = GetSendPropOffset(entity->GetServerClass(), name);
            if (entry.offset > 0) {
                ReadProp(entity, entry, variable, arrayPos, vecAxis);
                found = true;
            }
        }
        return found;
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_InputIgnitePlayer, inputdata_t &inputdata)
    {
        CTFPlayer *activator = inputdata.pActivator != nullptr && inputdata.pActivator->IsPlayer() ? ToTFPlayer(inputdata.pActivator) : reinterpret_cast<CTFPlayer *>(this);
        reinterpret_cast<CTFPlayer *>(this)->m_Shared->Burn(activator, nullptr, 10.0f);
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

    inline bool CompareCaseInsensitiveStringView(std::string_view &view1, std::string_view &view2)
    {
        return view1.size() == view2.size() && stricmp(view1.data(), view2.data()) == 0;
    }

    inline bool CompareCaseInsensitiveStringViewBeginsWith(std::string_view &view1, std::string_view &view2)
    {
        return view1.size() >= view2.size() && strnicmp(view1.data(), view2.data(), view2.size()) == 0;
    }

    bool allow_create_dropped_weapon = false;
    CustomInputFunction *GetCustomInput(CBaseEntity *ent, const char *szInputName)
    {
        {
            char inputNameLowerBuf[1024];
            StrLowerCopy(szInputName, inputNameLowerBuf);
            std::string_view inputNameLower(inputNameLowerBuf);

            for (auto &filter : InputFilter::List()) {
                if (filter->Test(ent)) {
                    for (auto &input : filter->inputs) {
                        if ((!input.prefix && CompareCaseInsensitiveStringView(inputNameLower, input.name)) ||
                            (input.prefix && CompareCaseInsensitiveStringViewBeginsWith(inputNameLower, input.name))) {
                            return &input.func;
                        }
                    }
                }
            }
            return nullptr;
        }
        
        return nullptr;
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
        if (szInputName[0] == '$') {
            auto func = GetCustomInput(ent, szInputName + 1);
            if (func != nullptr) {
                (*func)(ent, szInputName + 1, pActivator, pCaller, Value);
                return true;
            }
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

                        CBaseFilter *filter = rtti_cast<CBaseFilter *>(servertools->FindEntityByName(nullptr, filtername.c_str()));
                        if (filter != nullptr) {
                            while (true) {
                                pStartEntity = functor(pStartEntity, realname);
                                if (pStartEntity == nullptr) return nullptr;
                                Msg("Passes filter %d\n", filter->PassesFilter(pStartEntity, pStartEntity));
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

        return functor(pStartEntity, szName+1);
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

        if (szName[0] == '@' && szName[1] == 'h' && szName[2] == '@') { return CHandle<CBaseEntity>::FromIndex(atoi(szName+3)); }

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

    DETOUR_DECL_MEMBER(bool, CTankSpawner_Spawn, const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents)
	{
		auto spawner = reinterpret_cast<CTankSpawner *>(this);
		
		auto result = DETOUR_MEMBER_CALL(CTankSpawner_Spawn)(where, ents);
		
        if (cvar_fast_lookup.GetBool() && result && ents != nullptr && !ents->IsEmpty()) {

            auto tank = rtti_cast<CTFTankBoss *>(ents->Tail().Get());
            if (tank != nullptr) {
                char *lowercase = stackalloc(strlen(STRING(tank->GetEntityName())) + 1);
                StrLowerCopy(STRING(tank->GetEntityName()), lowercase);
                tank->SetName(AllocPooledString(lowercase));
            }
        }
        return result;
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
                            return valuecmp.FieldType() == FIELD_FLOAT ? variable.Float() < valuecmp.Float() : variable.Int() < valuecmp.Int();
                        }
                        else if (compare == less_or_equal) {
                            return valuecmp.FieldType() == FIELD_FLOAT ? variable.Float() <= valuecmp.Float() : variable.Int() <= valuecmp.Int();
                        }
                        else if (compare == greater) {
                            return valuecmp.FieldType() == FIELD_FLOAT ? variable.Float() > valuecmp.Float() : variable.Int() > valuecmp.Int();
                        }
                        else if (compare == greater_or_equal) {
                            return valuecmp.FieldType() == FIELD_FLOAT ? variable.Float() >= valuecmp.Float() : variable.Int() >= valuecmp.Int();
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
        DETOUR_MEMBER_CALL(CTriggerCamera_D0)();
    }

    DETOUR_DECL_MEMBER(void, CTriggerCamera_D2)
	{
        OnCameraRemoved(reinterpret_cast<CTriggerCamera *>(this));
        DETOUR_MEMBER_CALL(CTriggerCamera_D2)();
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

    DETOUR_DECL_STATIC(void, SV_ComputeClientPacks, int clientCount,  void **clients, void *snapshot)
	{
		for (auto &module : FakePropModule::List()) {
            for (auto &entry : module->props) {
                GetEntityVariable(module->entity, SENDPROP, entry.first.c_str(), entry.second.second);
                SetEntityVariable(module->entity, SENDPROP, entry.first.c_str(), entry.second.first);
            }
        }
		DETOUR_STATIC_CALL(SV_ComputeClientPacks)(clientCount, clients, snapshot);
        for (auto &module : FakePropModule::List()) {
            for (auto &entry : module->props) {
                SetEntityVariable(module->entity, SENDPROP, entry.first.c_str(), entry.second.second);
            }
        }
	}

    DETOUR_DECL_MEMBER(bool, CTFGameRules_RoundCleanupShouldIgnore, CBaseEntity *entity)
	{
        if (entity->GetClassname() == PStr<"$script_manager">()) return true;

        return DETOUR_MEMBER_CALL(CTFGameRules_RoundCleanupShouldIgnore)(entity);
    }

    DETOUR_DECL_MEMBER(bool, CTFGameRules_ShouldCreateEntity, const char *classname)
	{
        if (strcmp(classname, "$script_manager") == 0) return false;
        
        return DETOUR_MEMBER_CALL(CTFGameRules_ShouldCreateEntity)(classname);
    }
    

    VHOOK_DECL(void, CMathCounter_Activate)
	{
        auto entity = reinterpret_cast<CBaseEntity *>(this);
        if (strcmp(entity->GetClassname(), "$script_manager") == 0) {
            auto mod = entity->GetOrCreateEntityModule<ScriptModule>("script");

            auto filesVar = entity->GetCustomVariable<"scriptfile">();
            if (filesVar != nullptr) {
                std::string files(filesVar);
                boost::tokenizer<boost::char_separator<char>> tokens(files, boost::char_separator<char>(","));

                for (auto &token : tokens) {
                    mod->DoFile(token.c_str(), true);
                }
            }

            auto script = entity->GetCustomVariable<"script">();
            if (script != nullptr) {
                mod->DoString(script, true);
            }
            
            mod->Activate();
        }
        
        return VHOOK_CALL(CMathCounter_Activate)();
    }

    bool DoCollideTest(CBaseEntity *entity1, CBaseEntity *entity2, bool &result) {
        variant_t val;
        if (entity1->GetCustomVariableVariant<"colfilter">(val)) {
            auto filterEnt = static_cast<CBaseFilter *>(val.Entity().Get());
            if (filterEnt != nullptr) {
                result = filterEnt->PassesFilter(entity1, entity2);
                return true;
            }
        }
        else if (entity2->GetCustomVariableVariant<"colfilter">(val)) {
            auto filterEnt = static_cast<CBaseFilter *>(val.Entity().Get());
            if (filterEnt != nullptr) {
                result = filterEnt->PassesFilter(entity2, entity1);
                return true;
            }
        }
        return false;
    }
    DETOUR_DECL_STATIC(bool, PassServerEntityFilter, IHandleEntity *ent1, IHandleEntity *ent2)
	{
        auto ret = DETOUR_STATIC_CALL(PassServerEntityFilter)(ent1, ent2);
        {
            if (ret) {
                auto entity1 = (CBaseEntity*) ent1;
                auto entity2 = (CBaseEntity*) ent2;
                
                bool result;
                if (entity1 != entity2 && entity1 != nullptr && entity2 != nullptr && DoCollideTest(entity1, entity2, result))
                {
                    return result;
                }
            }
        }
        return ret;
    }

    DETOUR_DECL_MEMBER(int, CCollisionEvent_ShouldCollide, IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1)
	{
        {
            CBaseEntity *entity1 = static_cast<CBaseEntity *>(pGameData0);
            CBaseEntity *entity2 = static_cast<CBaseEntity *>(pGameData1);

            bool result;
            if (entity1 != entity2 && entity1 != nullptr && entity2 != nullptr && DoCollideTest(entity1, entity2, result))
            {
                return result;
            }
        }
        return DETOUR_MEMBER_CALL(CCollisionEvent_ShouldCollide)(pObj0, pObj1, pGameData0, pGameData1);
    }

    DETOUR_DECL_MEMBER(int, CBaseEntity_DispatchUpdateTransmitState)
	{
        auto entity = reinterpret_cast<CBaseEntity *>(this);
        auto mod = entity->GetEntityModule<VisibilityModule>("visibility");
        if (mod != nullptr) {
            if (mod->hideTo.empty()) {
                if (mod->defaultHide) {
                    entity->SetTransmitState(FL_EDICT_DONTSEND);
                    return FL_EDICT_DONTSEND;
                }
            }
            else {
                entity->SetTransmitState(FL_EDICT_FULLCHECK);
                return FL_EDICT_FULLCHECK;
            }
        }
        return DETOUR_MEMBER_CALL(CBaseEntity_DispatchUpdateTransmitState)();
    }

    bool ModVisibilityUpdate(CBaseEntity *entity, const CCheckTransmitInfo *info)
    {
        auto mod = entity->GetEntityModule<VisibilityModule>("visibility");
        if (mod != nullptr) {
            bool hide = mod->defaultHide;
            for (auto player : mod->hideTo) {
                if (player == info->m_pClientEnt) {
                    hide = !hide;
                    break;
                }
            }
            if (hide) {
                return true;
            }
        }
        return false;
    }

    DETOUR_DECL_MEMBER(int, CBaseEntity_ShouldTransmit, const CCheckTransmitInfo *info)
	{
        {
            auto entity = reinterpret_cast<CBaseEntity *>(this);
            if (entity->GetExtraEntityData() != nullptr && ModVisibilityUpdate(entity, info))
                return FL_EDICT_DONTSEND;
        }
        return DETOUR_MEMBER_CALL(CBaseEntity_ShouldTransmit)(info);
    }

    void ClearFakeProp()
    {
        while (!FakePropModule::List().empty() ) {
            FakePropModule::List().back()->entity->RemoveEntityModule("fakeprop");
        }
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
            MOD_ADD_DETOUR_STATIC(SV_ComputeClientPacks, "SV_ComputeClientPacks");
            MOD_ADD_DETOUR_MEMBER(CTankSpawner_Spawn, "CTankSpawner::Spawn");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_RoundCleanupShouldIgnore, "CTFGameRules::RoundCleanupShouldIgnore");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_ShouldCreateEntity, "CTFGameRules::ShouldCreateEntity");
			MOD_ADD_VHOOK(CMathCounter_Activate, TypeName<CMathCounter>(), "CBaseEntity::Activate");
            MOD_ADD_DETOUR_STATIC(PassServerEntityFilter, "PassServerEntityFilter");
            MOD_ADD_DETOUR_MEMBER(CCollisionEvent_ShouldCollide, "CCollisionEvent::ShouldCollide");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_ShouldTransmit, "CBaseEntity::ShouldTransmit");
            MOD_ADD_DETOUR_MEMBER(CBaseEntity_DispatchUpdateTransmitState, "CBaseEntity::DispatchUpdateTransmitState");

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
            stringSendProxy = AddrManager::GetAddr("SendProxy_StringToString");
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
                servertools->GetEntityFactoryDictionary()->InstallFactory(servertools->GetEntityFactoryDictionary()->FindFactory("math_counter"), "$script_manager");
            }

			return true;
		}
        virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }

        virtual void OnEnable() override
		{
            sendproxies = gamedll->GetStandardSendProxies();
        }

        virtual void LevelInitPreEntity() override
        { 
            sendproxies = gamedll->GetStandardSendProxies();
            send_prop_cache.clear();
            send_prop_cache_classes.clear();
            datamap_cache.clear();
            datamap_cache_classes.clear();
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
