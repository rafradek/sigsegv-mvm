inline const CBaseEntity *MyEntityFromEntityHandle( const IHandleEntity *pConstHandleEntity )
{
    return (const CBaseEntity *) pConstHandleEntity;
}

inline CBaseEntity *MyEntityFromEntityHandle( IHandleEntity *pHandleEntity )
{
    return (CBaseEntity *) pHandleEntity;
}