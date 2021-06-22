/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Player/NextBotPlayerLocomotion.h
 */


class PlayerLocomotion : public ILocomotion
{
public:
	PlayerLocomotion(INextBot *nextbot);
	virtual ~PlayerLocomotion();
	
	virtual void Reset() override;
	virtual void Update() override;
	
	virtual void Approach(const Vector& dst, float f1) override;
	virtual void DriveTo(const Vector& dst) override;
	virtual bool ClimbUpToLedge(const Vector& v1, const Vector& v2, const CBaseEntity *ent) override;
	virtual void JumpAcrossGap(const Vector& v1, const Vector& v2) override;
	virtual void Jump() override;
	virtual bool IsClimbingOrJumping() const override;
	virtual bool IsClimbingUpToLedge() const override;
	virtual bool IsJumpingAcrossGap() const override;
	virtual void Run() override;
	virtual void Walk() override;
	virtual void Stop() override;
	virtual bool IsRunning() const override;
	virtual void SetDesiredSpeed(float speed) override;
	virtual float GetDesiredSpeed() const override;
	virtual bool IsOnGround() const override;
	virtual CBaseEntity *GetGround() const override;
	virtual Vector& GetGroundNormal() const override;
	virtual void ClimbLadder(const CNavLadder *ladder, const CNavArea *area) override;
	virtual void DescendLadder(const CNavLadder *ladder, const CNavArea *area) override;
	virtual bool IsUsingLadder() const override;
	virtual bool IsAscendingOrDescendingLadder() const override;
	virtual bool IsAbleToAutoCenterOnLadder() const override;
	virtual void FaceTowards(const Vector& vec) override;
	virtual void SetDesiredLean(const QAngle& ang) override;
	virtual QAngle& GetDesiredLean() const override;
	virtual Vector& GetFeet() const override;
	virtual float GetStepHeight() const override;
	virtual float GetMaxJumpHeight() const override;
	virtual float GetDeathDropHeight() const override;
	virtual float GetRunSpeed() const override;
	virtual float GetWalkSpeed() const override;
	virtual float GetMaxAcceleration() const override;
	virtual float GetMaxDeceleration() const override;
	virtual Vector& GetVelocity() const override;
	virtual void AdjustPosture(const Vector& vec) override;
	
	virtual void SetMinimumSpeedLimit(float speed);
	virtual void SetMaximumSpeedLimit(float speed);
	
private:
	enum LadderState : int
	{
		NONE             = 0,
		ASCEND_APPROACH  = 1,
		DESCEND_APPROACH = 2,
		ASCEND           = 3,
		DESCEND          = 4,
		ASCEND_DISMOUNT  = 5,
		DESCEND_DISMOUNT = 6,
	};
	
	bool IsClimbPossible(INextBot *nextbot, const CBaseEntity *ent);
	
	bool TraverseLadder();
	LadderState ApproachAscendingLadder();
	LadderState ApproachDescendingLadder();
	LadderState AscendLadder();
	LadderState DescendLadder();
	LadderState DismountLadderTop();
	LadderState DismountLadderBottom();
	
	CBasePlayer *m_Player;          // +0x58
	bool m_bJumping;                // +0x5c
	CountdownTimer m_ctJump;        // +0x60
	bool m_bClimbing;               // +0x6c
	bool m_bGapJumping;             // +0x6d
	Vector m_vecClimbJump;          // +0x70
	// 7c byte, something about being airborne after jumping/climbing
	float m_flDesiredSpeed;         // +0x80
	float m_flMinSpeedLimit;        // +0x84
	float m_flMaxSpeedLimit;        // +0x88
	LadderState m_iLadderState;     // +0x8c
	const CNavLadder *m_NavLadder;  // +0x90
	const CNavArea *m_NavArea;      // +0x94
	CountdownTimer m_ctPlayerLoco2; // +0x98
};
