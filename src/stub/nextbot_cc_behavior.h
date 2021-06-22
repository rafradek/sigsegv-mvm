#ifndef _INCLUDE_SIGSEGV_STUB_NEXTBOT_CC_BEHAVIOR_H_
#define _INCLUDE_SIGSEGV_STUB_NEXTBOT_CC_BEHAVIOR_H_


#include "re/nextbot.h"
#include "re/path.h"
#include "stub/nextbot_cc.h"


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
SIZE_CHECK(CEyeballBossIdle, 0x4854); // 0x4851

class CEyeballBossDead : public Action<CEyeballBoss>
{
public:
	CountdownTimer m_ctDelay; // +0x0034
};
SIZE_CHECK(CEyeballBossDead, 0x0040); // 0x0040


#endif
