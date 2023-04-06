#ifndef IENTITYHANDLE_MY_H
#define IENTITYHANDLE_MY_H
// Set later in extension
extern void *vtable_staticprop;
inline const CBaseEntity *MyEntityFromEntityHandle( const IHandleEntity *pConstHandleEntity )
{
    return pConstHandleEntity == nullptr || *(uintptr_t *)pConstHandleEntity != (uintptr_t)vtable_staticprop ? (const CBaseEntity *) pConstHandleEntity : nullptr;
}

inline CBaseEntity *MyEntityFromEntityHandle( IHandleEntity *pHandleEntity )
{
    return pHandleEntity == nullptr || *(uintptr_t *)pHandleEntity != (uintptr_t)vtable_staticprop ? (CBaseEntity *) pHandleEntity : nullptr;
}

#endif