#include "stub/point.h"


IMPL_REL_BEFORE_ALIGN(CUniformRandomStream,     CTFPointManager, m_Random,            m_nRandomSeed, alignof(CUniformRandomStream));
IMPL_SENDPROP        (int,                      CTFPointManager, m_nRandomSeed,       CTFPointManager);
IMPL_SENDPROP        (int[30],                  CTFPointManager, m_nSpawnTime,        CTFPointManager);
IMPL_SENDPROP        (unsigned int,             CTFPointManager, m_unNextPointIndex,  CTFPointManager);
IMPL_REL_AFTER       (float,                    CTFPointManager, m_flLastUpdateTime,  m_unNextPointIndex);
IMPL_REL_AFTER       (CUtlVector<tf_point_t *>, CTFPointManager, m_Points,            m_flLastUpdateTime);


IMPL_REL_BEFORE_ALIGN(CTFFlameManager::MapType049c, CTFFlameManager, m_Map__049c,                           m_nDamageBits, alignof(CTFFlameManager::MapType049c));
IMPL_REL_BEFORE      (int,                          CTFFlameManager, m_nDamageBits,                         m_flDamage);
IMPL_REL_BEFORE      (float,                        CTFFlameManager, m_flDamage,                            m_flUnknown__04c0);
IMPL_REL_BEFORE      (float,                        CTFFlameManager, m_flUnknown__04c0,                     m_bBackCrit);
IMPL_REL_BEFORE_ALIGN(bool,                         CTFFlameManager, m_bBackCrit,                           m_hWeapon,     alignof(CHandle<CTFFlameThrower>));
IMPL_SENDPROP        (CHandle<CTFFlameThrower>,     CTFFlameManager, m_hWeapon,                             CTFFlameManager);
IMPL_SENDPROP        (CHandle<CTFPlayer>,           CTFFlameManager, m_hAttacker,                           CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flSpreadDegree,                      CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flRedirectedFlameSizeMult,           CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameStartSizeMult,                CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameEndSizeMult,                  CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameIgnorePlayerVelocity,         CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameReflectionAdditionalLifeTime, CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameReflectionDamageReduction,    CTFFlameManager);
IMPL_SENDPROP        (int,                          CTFFlameManager, m_iMaxFlameReflectionCount,            CTFFlameManager);
IMPL_SENDPROP        (int,                          CTFFlameManager, m_nShouldReflect,                      CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameSpeed,                        CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameLifeTime,                     CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flRandomLifeTimeOffset,              CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameGravity,                      CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameDrag,                         CTFFlameManager);
IMPL_SENDPROP        (float,                        CTFFlameManager, m_flFlameUp,                           CTFFlameManager);
IMPL_SENDPROP        (bool,                         CTFFlameManager, m_bIsFiring,                           CTFFlameManager);


GlobalThunk<CUtlVector<ITFFlameManager *>> ITFFlameManager::m_ITFFlameManagerAutoList("ITFFlameManager::m_ITFFlameManagerAutoList");
