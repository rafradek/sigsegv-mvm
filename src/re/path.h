#ifndef _INCLUDE_SIGSEGV_RE_PATH_H_
#define _INCLUDE_SIGSEGV_RE_PATH_H_


#if defined _MSC_VER
#pragma message("Path classes have not been checked against the VC++ build!")
#endif


#include "util/misc.h"


class CBaseObject;
class CNavLadder;
class CFuncElevator;
class CNavArea;
class INextBot;
class CBaseCombatCharacter;
class CTFBot;
class CZombie;
enum NavDirType : int32_t;



#include "../mvm-reversed/server/NextBot/Path/NextBotPath.h"
#include "../mvm-reversed/server/NextBot/Path/NextBotPathFollow.h"
#include "../mvm-reversed/server/NextBot/Path/NextBotChasePath.h"


#ifdef PLATFORM_64BITS
SIZE_CHECK(Path,         0x5668);
SIZE_CHECK(PathFollower, 0x56F8);
SIZE_CHECK(ChasePath,    0x5730);
SIZE_CHECK(IPathCost,    0x0008);
#else
SIZE_CHECK(Path,         0x4754);
SIZE_CHECK(PathFollower, 0x47d4);
SIZE_CHECK(ChasePath,    0x4800);
SIZE_CHECK(IPathCost,    0x0004);
#endif

/* from game/server/nav_pathfind.h */
enum RouteType : int32_t
{
	DEFAULT_ROUTE,
	FASTEST_ROUTE,
	SAFEST_ROUTE,
	RETREAT_ROUTE,
};


class CTFBotPathCost : public IPathCost
{
public:
	CTFBotPathCost(CTFBot *actor, RouteType rtype);
	~CTFBotPathCost();
	
	virtual float operator()(CNavArea *area1, CNavArea *area2, const CNavLadder *ladder, const CFuncElevator *elevator, float f1) const override;
	
private:
	CTFBot *m_Actor;                          // +0x04
	RouteType m_iRouteType;                   // +0x08
	float m_flStepHeight;                     // +0x0c
	float m_flMaxJumpHeight;                  // +0x10
	float m_flDeathDropHeight;                // +0x14
	CUtlVector<CBaseObject *> m_EnemyObjects; // +0x18
};
#ifdef PLATFORM_64BITS
SIZE_CHECK(CTFBotPathCost, 0x40);
#else
SIZE_CHECK(CTFBotPathCost, 0x2c);
#endif


class CZombiePathCost : public IPathCost
{
public:
	CZombiePathCost( CZombie *actor ) : m_Actor(actor) {};
	~CZombiePathCost();
	
	virtual float operator()(CNavArea *area1, CNavArea *area2, const CNavLadder *ladder, const CFuncElevator *elevator, float f1) const override;
	
private:
	CZombie *m_Actor;                          // +0x04
};

#warning REMOVE ME / MOVE ME ELSEWHERE!
inline CTFBotPathCost::~CTFBotPathCost() {}

#warning REMOVE ME / MOVE ME ELSEWHERE!
inline CZombiePathCost::~CZombiePathCost() {}


#endif
