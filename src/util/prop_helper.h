

#ifndef _INCLUDE_SIGSEGV_UTIL_FROP_HELPER_H_
#define _INCLUDE_SIGSEGV_UTIL_FROP_HELPER_H_

struct PropCacheEntry
{
    int offset = 0;
    fieldtype_t fieldType = FIELD_VOID;
    int arraySize = 1;
    int elementStride = 0;
};

void WriteProp(CBaseEntity *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis);
void ReadProp(CBaseEntity *entity, PropCacheEntry &entry, variant_t &variant, int arrayPos, int vecAxis);

void GetSendPropInfo(SendProp *prop, PropCacheEntry &entry, int offset);
void GetDataMapInfo(typedescription_t &desc, PropCacheEntry &entry);

PropCacheEntry &GetSendPropOffset(ServerClass *serverClass, const std::string &name);
PropCacheEntry &GetDataMapOffset(datamap_t *datamap, const std::string &name);

void ResetPropDataCache();
#endif