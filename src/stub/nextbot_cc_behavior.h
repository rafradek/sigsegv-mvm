#ifndef _INCLUDE_SIGSEGV_STUB_NEXTBOT_CC_BEHAVIOR_H_
#define _INCLUDE_SIGSEGV_STUB_NEXTBOT_CC_BEHAVIOR_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/nextbot_cc.h"

class CBotNPCBody : public IBody
{
public:
	int m_currentActivity;
	int m_moveXPoseParameter;	
	int m_moveYPoseParameter;
	QAngle m_desiredAimAngles;
};

class CZombieIntention : public IIntention
{
public:
	Behavior< CZombie > *m_behavior;
};

class CBotNPCArcherIntention : public IIntention
{
public:
	Behavior< CBotNPCArcher > *m_behavior;
};


class CEyeballBossIdle : public Action<CEyeballBoss>
{
public:
	CountdownTimer       m_ctLookAround;   // +0x0034
	PathFollower         m_PathFollower;   // +0x0040
	CountdownTimer       m_ctUnused;       // +0x4814
	CHandle<CBaseEntity> m_hLastAttacker;  // +0x4820
	CountdownTimer       m_ctMakeSound;    // +0x4824
	CountdownTimer       m_ctLifetime;     // +0x4830
	CountdownTimer       m_ctTeleport;     // +0x483c
	float                m_flLifetimeLeft; // +0x4848
	int                  m_iHealth;        // +0x484c
	bool                 m_bOtherKilled;   // +0x4850
};
#ifdef PLATFORM_64BITS
SIZE_CHECK(CEyeballBossIdle, 0x57c8); // 0x4851
#else
SIZE_CHECK(CEyeballBossIdle, 0x4854); // 0x4851
#endif

class CEyeballBossDead : public Action<CEyeballBoss>
{
public:
	CountdownTimer m_ctDelay; // +0x0034
};
#ifdef PLATFORM_64BITS
SIZE_CHECK(CEyeballBossDead, 0x0078); // 0x0040
#else
SIZE_CHECK(CEyeballBossDead, 0x0040); // 0x0040
#endif


#endif
