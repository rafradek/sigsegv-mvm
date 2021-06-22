/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Path/NextBotChasePath.h
 */


class ChasePath : public PathFollower
{
public:
	ChasePath();
	virtual ~ChasePath();
	
	virtual void Invalidate() override;
	
	virtual void Update(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec);
	virtual float GetLeadRadius() const;
	virtual float GetMaxPathLength() const;
	virtual Vector PredictSubjectPosition(INextBot *nextbot, CBaseEntity *ent) const;
	virtual bool IsRepathNeeded(INextBot *nextbot, CBaseEntity *ent) const;
	virtual float GetLifetime() const;
	
	void RefreshPath(INextBot *nextbot, CBaseEntity *ent, const IPathCost& cost_func, Vector *vec);
	
protected:
	#define UNKNOWN_PTR void*
	#define PAD(n, x) char n[x]
	
	CountdownTimer m_ctTimer1; // +0x47d4
	CountdownTimer m_ctTimer2; // +0x47e0
	CountdownTimer m_ctTimer3; // +0x47ec
	CHandle<CBaseEntity> m_hChaseSubject;
	PAD(pad_47fc, 0x4);
	// 47fc dword 0 or possibly 1
};

// TODO: DirectChasePath
