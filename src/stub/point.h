#ifndef _INCLUDE_SIGSEGV_STUB_POINT_H_
#define _INCLUDE_SIGSEGV_STUB_POINT_H_


#include "link/link.h"
#include "stub/baseentity.h"
#include "util/misc.h"


// BASED ON: server_srv.so 20171020a
#ifndef _LINUX
#error
#endif


class CTFPlayer;
class CTFFlameThrower;


struct tf_point_t
{
	virtual ~tf_point_t() = default;
	
	Vector m_vecPosition1;         // +0x0004
	Vector m_vecVelocity1;         // +0x0010
	float m_flTimeCreated;         // +0x001c
	float m_flLifeTime;            // +0x0020
	uint32_t m_UnknownDword__0024; // +0x0024
	uint32_t m_UnknownDword__0028; // +0x0028
	Vector m_vecPosition2;         // +0x002c
};
SIZE_CHECK(tf_point_t, 0x38);

struct flame_point_t : public tf_point_t
{
	Vector m_vecAttackerVelocity; // +0x0038
	Vector m_vecUnknown__0044;    // +0x0044
};
SIZE_CHECK(flame_point_t, 0x50);


class CTFPointManager : public CBaseEntity
{
public:
	// TODO
	
//private:
	DECL_RELATIVE(CUniformRandomStream,     m_Random);           // +0x036c
	DECL_SENDPROP(int,                      m_nRandomSeed);      // +0x0400
	DECL_SENDPROP(int[30],                  m_nSpawnTime);       // +0x0404
	DECL_SENDPROP(unsigned int,             m_unNextPointIndex); // +0x047c
	DECL_RELATIVE(float,                    m_flLastUpdateTime); // +0x0480
	DECL_RELATIVE(CUtlVector<tf_point_t *>, m_Points);           // +0x0484
};

class CTFFlameManager : public CTFPointManager
{
public:
	// TODO
	
//private:
	using MapType049c = CUtlMap<CHandle<CBaseEntity>, float, unsigned short>;
	DECL_RELATIVE(MapType049c,              m_Map__049c);                           // +0x049c
	DECL_RELATIVE(int,                      m_nDamageBits);                         // +0x04b8
	DECL_RELATIVE(float,                    m_flDamage);                            // +0x04bc
	DECL_RELATIVE(float,                    m_flUnknown__04c0);                     // +0x04c0
	DECL_RELATIVE(bool,                     m_bBackCrit);                           // +0x04c4
	DECL_SENDPROP(CHandle<CTFFlameThrower>, m_hWeapon);                             // +0x04c8
	DECL_SENDPROP(CHandle<CTFPlayer>,       m_hAttacker);                           // +0x04cc
	DECL_SENDPROP(float,                    m_flSpreadDegree);                      // +0x04d0
	DECL_SENDPROP(float,                    m_flRedirectedFlameSizeMult);           // +0x04d4
	DECL_SENDPROP(float,                    m_flFlameStartSizeMult);                // +0x04d8
	DECL_SENDPROP(float,                    m_flFlameEndSizeMult);                  // +0x04dc
	DECL_SENDPROP(float,                    m_flFlameIgnorePlayerVelocity);         // +0x04e0
	DECL_SENDPROP(float,                    m_flFlameReflectionAdditionalLifeTime); // +0x04e4
	DECL_SENDPROP(float,                    m_flFlameReflectionDamageReduction);    // +0x04e8
	DECL_SENDPROP(int,                      m_iMaxFlameReflectionCount);            // +0x04ec
	DECL_SENDPROP(int,                      m_nShouldReflect);                      // +0x04f0
	DECL_SENDPROP(float,                    m_flFlameSpeed);                        // +0x04f4
	DECL_SENDPROP(float,                    m_flFlameLifeTime);                     // +0x04f8
	DECL_SENDPROP(float,                    m_flRandomLifeTimeOffset);              // +0x04fc
	DECL_SENDPROP(float,                    m_flFlameGravity);                      // +0x0500
	DECL_SENDPROP(float,                    m_flFlameDrag);                         // +0x0504
	DECL_SENDPROP(float,                    m_flFlameUp);                           // +0x0508
	DECL_SENDPROP(bool,                     m_bIsFiring);                           // +0x050c
};

class CTFGasManager : public CTFPointManager {};


class CFlameManagerHelper/* : public CAutoGameSystemPerFrame*/ {};



class ITFFlameManager
{
public:
	static const CUtlVector<ITFFlameManager *>& AutoList() { return m_ITFFlameManagerAutoList; }
private:
	static GlobalThunk<CUtlVector<ITFFlameManager *>> m_ITFFlameManagerAutoList;
};


#endif
