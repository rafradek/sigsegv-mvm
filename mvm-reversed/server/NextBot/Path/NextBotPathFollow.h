/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Path/NextBotPathFollow.h
 */


class PathFollower : public Path
{
public:
	PathFollower();
	virtual ~PathFollower();
	
	virtual const Segment *GetCurrentGoal() const override;
	
	virtual void Invalidate() override;
	
	virtual void Draw(const Segment *seg) const override;
	
	virtual void OnPathChanged(INextBot *nextbot, ResultType rtype) override;
	
	virtual void Update(INextBot *nextbot);
	virtual void SetMinLookAheadDistance(float dist);
	virtual CBaseEntity *GetHindrance() const;
	virtual bool IsDiscontinuityAhead(INextBot *nextbot, SegmentType stype, float f1) const;
	
private:
	void AdjustSpeed(INextBot *nextbot);
	Vector Avoid(INextBot *nextbot, const Vector& v1, const Vector& v2, const Vector& v3);
	bool CheckProgress(INextBot *nextbot);
	bool Climbing(INextBot *nextbot, const Segment *seg, const Vector& v1, const Vector& v2, float f1);
	CBaseEntity *FindBlocker(INextBot *nextbot);
	bool IsAtGoal(INextBot *nextbot) const;
	bool JumpOverGaps(INextBot *nextbot, const Segment *seg, const Vector& v1, const Vector& v2, float f1);
	bool LadderUpdate(INextBot *nextbot);
	
	#define UNKNOWN_PTR void*
	#define PAD(n, x) char n[x]
	
	// TODO: do any of these need to be protected, i.e. accessed by ChasePath?
	const Segment *m_CurrentGoal;   // +0x4754
	float m_flMinLookAheadDistance; // +0x4758
	PAD(pad_475c, 0x4);
	CountdownTimer m_ctUnknown1; // +0x4760
	CountdownTimer m_ctUnknown2; // +0x476c
	CHandle<CBaseEntity> m_hHindrance; // +0x4778
	// 477c byte 0
	// 4780 Vector
	// 478c Vector
	// 4798 byte
	// 479c Vector
	// 47a8 Vector
	// 47b4 byte
	// 47b8 Vector
	// 47c4 Vector
	// 47d0 float 25.0f
	PAD(pad_477c, 0x58);
};
