#ifndef _INCLUDE_SIGSEGV_STUB_NEXTBOT_ETC_H_
#define _INCLUDE_SIGSEGV_STUB_NEXTBOT_ETC_H_


#include "re/nextbot.h"
#include "re/path.h"

class NextBotGroundLocomotion : public ILocomotion
{
public:
	static NextBotGroundLocomotion *New(INextBot *bot);

	virtual void SetVelocity(const Vector &vel);
	float GetUpdateInterval() { return m_flTickInterval; }
	Vector GetApproachPos() { return m_accumApproachVectors / m_accumApproachWeights; }

	void SetGroundNormal(const Vector &vec) { m_groundNormal = vec; }

	NextBotCombatCharacter *m_nextBot;

	Vector m_priorPos;										// last update's position
	Vector m_lastValidPos;									// last valid position (not interpenetrating)
	
	Vector m_acceleration;
	Vector m_velocity;
	
	float m_desiredSpeed;									// speed bot wants to be moving
	float m_actualSpeed;									// actual speed bot is moving

	float m_maxRunSpeed;

	float m_forwardLean;
	float m_sideLean;
	QAngle m_desiredLean;
	
	bool m_isJumping;										// if true, we have jumped and have not yet hit the ground
	bool m_isJumpingAcrossGap;								// if true, we have jumped across a gap and have not yet hit the ground
	EHANDLE m_ground;										// have to manage this ourselves, since MOVETYPE_CUSTOM always NULLs out GetGroundEntity()
	Vector m_groundNormal;									// surface normal of the ground we are in contact with
	bool m_isClimbingUpToLedge;									// true if we are jumping up to an adjacent ledge
	Vector m_ledgeJumpGoalPos;
	bool m_isUsingFullFeetTrace;							// true if we're in the air and tracing the lowest StepHeight in ResolveCollision

	const CNavLadder *m_ladder;								// ladder we are currently climbing/descending
	const CNavArea *m_ladderDismountGoal;					// the area we enter when finished with our ladder move
	bool m_isGoingUpLadder;									// if false, we're going down

	CountdownTimer m_inhibitObstacleAvoidanceTimer;			// when active, turn off path following feelers

	CountdownTimer m_wiggleTimer;							// for wiggling
	int m_wiggleDirection;

	mutable Vector m_eyePos;								// for use with GetEyes(), etc.

	Vector m_moveVector;									// the direction of our motion in XY plane
	float m_moveYaw;										// global yaw of movement direction

	Vector m_accumApproachVectors;							// weighted sum of Approach() calls since last update
	float m_accumApproachWeights;
	bool m_bRecomputePostureOnCollision;

	CountdownTimer m_ignorePhysicsPropTimer;				// if active, don't collide with physics props (because we got stuck in one)
	EHANDLE m_ignorePhysicsProp;							// which prop to ignore

	int8_t pad[256];
};

#endif
