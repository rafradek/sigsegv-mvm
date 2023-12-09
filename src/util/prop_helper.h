

#ifndef _INCLUDE_SIGSEGV_UTIL_FROP_HELPER_H_
#define _INCLUDE_SIGSEGV_UTIL_FROP_HELPER_H_

struct DatatableProxyOffset
{
    SendProp *prop;
    int base;
    int offset;
};

using DatatableProxyVector = std::vector<DatatableProxyOffset>;
struct PropCacheEntry
{
    int offset = 0;
    fieldtype_t fieldType = FIELD_VOID;
    int arraySize = 1;
    int elementStride = 0;
    SendProp *prop = nullptr;
    DatatableProxyVector usedTables;
};

// Remember to call entity->NetworkStateChanged();
void WriteProp(void *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis);
void ReadProp(void *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis);

bool FindSendProp(int& off, SendTable *s_table, const char *name, SendProp *&prop, DatatableProxyVector &usedTables, int index = -1);

void GetSendPropInfo(SendProp *prop, PropCacheEntry &entry, int offset);
void GetDataMapInfo(typedescription_t &desc, PropCacheEntry &entry);

PropCacheEntry &GetSendPropOffset(ServerClass *serverClass, const std::string &name);
PropCacheEntry &GetDataMapOffset(datamap_t *datamap, const std::string &name);

void ResetPropDataCache();

int FindSendPropPrecalcIndex(SendTable *sclass, const std::string &name, int index, SendProp *&prop);
#endif