
#include <util/prop_helper.h>
#include <stub/misc.h>
#include <stub/sendprop.h>

std::vector<ServerClass *> send_prop_cache_classes;
std::vector<std::pair<std::vector<std::string>, std::vector<PropCacheEntry>>> send_prop_cache;

std::vector<datamap_t *> datamap_cache_classes;
std::vector<std::pair<std::vector<std::string>, std::vector<PropCacheEntry>>> datamap_cache;

void *stringSendProxy = nullptr;

bool FindSendProp(int& off, SendTable *s_table, const char *name, SendProp *&prop, DatatableProxyVector &usedTables, int index = -1)
{
    static CStandardSendProxies* sendproxies = gamedll->GetStandardSendProxies();
    for (int i = 0; i < s_table->GetNumProps(); ++i) {
        SendProp *s_prop = s_table->GetProp(i);
        
        if (s_prop->GetName() != nullptr && strcmp(s_prop->GetName(), name) == 0) {
            off += s_prop->GetOffset();
            if (index >= 0) {
                if (s_prop->GetDataTable() != nullptr && index < s_prop->GetDataTable()->GetNumProps()) {
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
            if (s_prop->GetDataTable() != nullptr) {
                bool modifying = !CPropMapStack::IsNonPointerModifyingProxy(s_prop->GetDataTableProxyFn(), sendproxies);
                int oldOffset = usedTables.empty() ? 0 : usedTables.back().second;
                if (modifying) {
                    usedTables.push_back({s_prop, off - oldOffset});
                    off = 0;
                }
            }
            prop = s_prop;
            return true;
        }
        
        if (s_prop->GetDataTable() != nullptr) {
            bool modifying = !CPropMapStack::IsNonPointerModifyingProxy(s_prop->GetDataTableProxyFn(), sendproxies);
            int oldOffset = usedTables.empty() ? 0 : usedTables.back().second;
            int oldOffReal = off;
            off += s_prop->GetOffset();
            if (modifying) {
                usedTables.push_back({s_prop, off - oldOffset});
                off = 0;
            }

            if (FindSendProp(off, s_prop->GetDataTable(), name, prop, usedTables, index)) {
                return true;
            }
            off -= s_prop->GetOffset();
            if (modifying) {
                off = oldOffReal;
                usedTables.pop_back();
            }
        }
    }
    
    return false;
}

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
    entry.prop = prop;

    if (prop->GetArrayProp() != nullptr) {
        prop = prop->GetArrayProp();
    }

    if (prop->GetDataTable() != nullptr && prop->GetDataTable()->GetNumProps() > 0) {
        prop = prop->GetDataTable()->GetProp(0);
    }

    auto propType = prop->GetType();
    if (propType == DPT_Int) {
        
        auto proxyfn = prop->GetProxyFn();
        if ((prop->m_nBits == 21 && (prop->GetFlags() & SPROP_UNSIGNED)) || proxyfn == DLLSendProxy_EHandleToInt.GetRef()) {
            entry.fieldType = FIELD_EHANDLE;
        }
        else if ((prop->m_nBits == 32 && (prop->GetFlags() & SPROP_UNSIGNED)) && StringHasPrefixCaseSensitive(prop->GetName(), "m_clr")) {
            entry.fieldType = FIELD_COLOR32;
        }
        else {
            static CStandardSendProxies* sendproxies = gamedll->GetStandardSendProxies();
            if (proxyfn == sendproxies->m_Int8ToInt32 || proxyfn == sendproxies->m_UInt8ToInt32) {
                entry.fieldType = FIELD_CHARACTER;
            }
            else if (proxyfn == sendproxies->m_Int16ToInt32 || proxyfn == sendproxies->m_UInt16ToInt32) {
                entry.fieldType = FIELD_SHORT;
            }
            else if (proxyfn == DLLSendProxy_Color32ToInt.GetRef()) {
                entry.fieldType = FIELD_COLOR32;
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
        if (proxyfn != DLLSendProxy_StringToString) {
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

PropCacheEntry &GetSendPropOffset(ServerClass *serverClass, const std::string &name) {
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

    size_t nameCount = names.size();
    for (size_t i = 0; i < nameCount; i++ ) {
        if (names[i] == name) {
            return pair.second[i];
        }
    }

    int offset = 0;
    SendProp *prop = nullptr;
    bool vectorComponent = false;
    PropCacheEntry entry;
    if (!FindSendProp(offset,serverClass->m_pTable, name.c_str(), prop, entry.usedTables)) {
        // Sometimes each component of vector is saved separately, check for their existence as well
        if (FindSendProp(offset,serverClass->m_pTable, (name + "[0]").c_str(), prop, entry.usedTables)) {
            vectorComponent = true;
        }
    }
    

    GetSendPropInfo(prop, entry, offset);
    if (vectorComponent && entry.fieldType == FIELD_FLOAT) {
        entry.fieldType = FIELD_VECTOR;
    }
    names.push_back(name);
    pair.second.push_back(entry);
    return pair.second.back();
}

CSendProxyRecipients static_recip;
void WriteProp(void *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis)
{
    if (entry.offset > 0) {
        void *base = entity;
        void *newBase = base;
        for (auto &[prop, offset] : entry.usedTables) {
            newBase = (unsigned char*)prop->GetDataTableProxyFn()( prop, base, ((char*)base) + offset, &static_recip, 0);
            if (newBase != nullptr) {
                base = newBase;
            }
        }

        int offset = entry.offset + arrayPos * entry.elementStride;
        fieldtype_t fieldType = entry.fieldType;
        if (vecAxis != -1) {
            fieldType = FIELD_FLOAT;
            offset += vecAxis * sizeof(float);
        }
        char *target = ((char*)base) + offset;

        if (fieldType == FIELD_CHARACTER && entry.arraySize > 1) {
            V_strncpy(target, variant.String(), entry.arraySize);
        }
        else {
            variant.Convert(fieldType);
            variant.SetOther(target);
        }
    }
}

void ReadProp(void *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis)
{
    if (entry.offset > 0) {
        void *base = entity;
        void *newBase = base;
        for (auto &[prop, offset] : entry.usedTables) {
            newBase = (unsigned char*)prop->GetDataTableProxyFn()( prop, base, ((char*)base) + offset, &static_recip, 0);
            if (newBase != nullptr) {
                base = newBase;
            }
        }
        
        int offset = entry.offset + arrayPos * entry.elementStride;
        fieldtype_t fieldType = entry.fieldType;
        if (vecAxis != -1) {
            fieldType = FIELD_FLOAT;
            offset += vecAxis * sizeof(float);
        }

        if (fieldType == FIELD_CHARACTER && entry.arraySize > 1) {
            variant.SetString(AllocPooledString(((char*)base) + offset));
        }
        else {
            variant.Set(fieldType, ((char*)base) + offset);
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
    auto fieldType = desc.fieldType;
    if (fieldType == FIELD_MODELNAME || fieldType == FIELD_SOUNDNAME) fieldType = FIELD_STRING;
    if (fieldType == FIELD_TIME) fieldType = FIELD_FLOAT;
    if (fieldType == FIELD_TICK || fieldType == FIELD_MODELINDEX || fieldType == FIELD_MATERIALINDEX) fieldType = FIELD_INTEGER;

    entry.fieldType = fieldType;
    
    entry.offset = desc.fieldOffset[ TD_OFFSET_NORMAL ];
    
    entry.arraySize = (int)desc.fieldSize;
    entry.elementStride = (desc.fieldSizeInBytes / desc.fieldSize);
}

PropCacheEntry &GetDataMapOffset(datamap_t *datamap, const std::string &name) {
    
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

    size_t nameCount = names.size();
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

void ResetPropDataCache() {
    send_prop_cache.clear();
    send_prop_cache_classes.clear();
    datamap_cache.clear();
    datamap_cache_classes.clear();
}

int FindSendPropPrecalcIndex(SendTable *table, const std::string &name, int index, SendProp *&prop) {
    prop = nullptr;
    int offset = 0;
    DatatableProxyVector usedTables;

    if (FindSendProp(offset, table, name.c_str(), prop, usedTables, index)) {
        auto precalc = table->m_pPrecalc;
        int indexProp = -1;
        for (int i = 0; i < precalc->m_Props.Count(); i++) {
            if (precalc->m_Props[i] == prop) {
                return i;
            }
        }
    }
    return -1;
}
