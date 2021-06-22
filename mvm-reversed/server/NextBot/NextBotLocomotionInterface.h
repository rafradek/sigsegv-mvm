/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/NextBotLocomotionInterface.h
 */


class ILocomotion : public INextBotComponent
{
public:
	enum class TraverseWhenType : int
	{
		// TODO
	};
	
	ILocomotion(INextBot *nextbot);
	virtual ~ILocomotion();
	
	virtual void OnLeaveGround(CBaseEntity *ent) override;
	virtual void OnLandOnGround(CBaseEntity *ent) override;
	
	virtual void Reset() override;
	virtual void Update() override;
	
	virtual void Approach(const Vector& v1, float f1);
	virtual void DriveTo(const Vector& v1);
	virtual bool ClimbUpToLedge(const Vector& v1, const Vector& v2, const CBaseEntity *ent);
	virtual void JumpAcrossGap(const Vector& v1, const Vector& v2);
	virtual void Jump();
	virtual bool IsClimbingOrJumping() const;
	virtual bool IsClimbingUpToLedge() const;
	virtual bool IsJumpingAcrossGap() const;
	virtual bool IsScrambling() const;
	virtual void Run();
	virtual void Walk();
	virtual void Stop();
	virtual bool IsRunning() const;
	virtual void SetDesiredSpeed(float speed);
	virtual float GetDesiredSpeed() const;
	virtual void SetSpeedLimit(float limit);
	virtual float GetSpeedLimit() const;
	virtual bool IsOnGround() const;
	virtual CBaseEntity *GetGround() const;
	virtual Vector& GetGroundNormal() const;
	virtual float GetGroundSpeed() const;
	virtual Vector& GetGroundMotionVector() const;
	virtual void ClimbLadder(const CNavLadder *ladder, const CNavArea *area);
	virtual void DescendLadder(const CNavLadder *ladder, const CNavArea *area);
	virtual bool IsUsingLadder() const;
	virtual bool IsAscendingOrDescendingLadder() const;
	virtual bool IsAbleToAutoCenterOnLadder() const;
	virtual void FaceTowards(const Vector& v1);
	virtual void SetDesiredLean(const QAngle& q1);
	virtual QAngle& GetDesiredLean() const;
	virtual bool IsAbleToJumpAcrossGaps() const;
	virtual bool IsAbleToClimb() const;
	virtual Vector& GetFeet() const;
	virtual float GetStepHeight() const;
	virtual float GetMaxJumpHeight() const;
	virtual float GetDeathDropHeight() const;
	virtual float GetRunSpeed() const;
	virtual float GetWalkSpeed() const;
	virtual float GetMaxAcceleration() const;
	virtual float GetMaxDeceleration() const;
	virtual Vector& GetVelocity() const;
	virtual float GetSpeed() const;
	virtual Vector& GetMotionVector() const;
	virtual bool IsAreaTraversable(const CNavArea *area) const;
	virtual float GetTraversableSlopeLimit() const;
	virtual bool IsPotentiallyTraversable(const Vector& v1, const Vector& v2, TraverseWhenType ttype, float *f1) const;
	virtual bool HasPotentialGap(const Vector& v1, const Vector& v2, float *f1) const;
	virtual bool IsGap(const Vector& v1, const Vector& v2) const;
	virtual bool IsEntityTraversable(CBaseEntity *ent, TraverseWhenType ttype) const;
	virtual bool IsStuck() const;
	virtual float GetStuckDuration() const;
	virtual void ClearStuckStatus(char const *s1);
	virtual bool IsAttemptingToMove() const;
	virtual bool ShouldCollideWith(const CBaseEntity *ent) const;
	virtual void AdjustPosture(const Vector& v1);
	virtual void StuckMonitor();
	
	void TraceHull(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs, unsigned int mask, ITraceFilter *filter, CGameTrace *trace);
	
protected:
	Vector m_vecMotion;         // +0x14
	Vector m_vecGroundMotion;   // +0x20
	float m_flSpeed;            // +0x2c
	float m_flGroundSpeed;      // +0x30
	bool m_bIsStuck;            // +0x34
	IntervalTimer m_itStuck;    // +0x38
	CountdownTimer m_ctUnknown; // +0x3c
	Vector m_vecStuck;          // +0x48
	IntervalTimer m_itApproach; // +0x54
};
