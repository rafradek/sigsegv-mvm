/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/NextBot/Path/NextBotPath.h
 */


class IPathCost
{
public:
	virtual float operator()(CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length) const = 0;
};


class IPathOpenGoalSelector
{
public:
	virtual CNavArea *operator()(CNavArea *area, CNavArea *fromArea) const;
};


class Path
{
public:
	enum class SegmentType : int
	{
		// TODO
		TYPE0 = 0, // invalid probably
		TYPE1 = 1,
		TYPE2 = 2,
		TYPE3 = 3,
		TYPE4 = 4,
		TYPE5 = 5,
	};
	
	enum class SeekType : int
	{
		FROM_START  = 0,
		FROM_CURSOR = 1,
		MAX,
	};
	
	enum class MoveCursorType : int
	{
		ABS = 0,
		REL = 1,
		MAX,
	};
	
	enum class ResultType : int
	{
		/* this is only ever zero, as far as I can tell */
		ZERO = 0,
	};
	
	#define UNKNOWN_PTR void*
	#define PAD(n, x) char n[x]
	
	struct Segment
	{
		const CNavArea *area;
		int how;
		Vector pos;
		const CNavLadder *ladder;
		SegmentType type;
		Vector forward;
		float length;
		float distanceFromStart;
		float curvature;
		Vector m_portalCenter;
		float m_portalHalfWidth;
	};
	
	struct CursorData
	{
		Vector m_vecSegStart;     // +0x00 (Segment::m_vecStart)
		Vector m_vecSegDirection; // +0x0c (Segment::m_vecDirection)
		// 18 float: Segment+30
		PAD(pad1, 0x4);
		Segment *m_pSegment;      // +0x1c
	};
	
	Path();
	virtual ~Path();
	
	virtual float GetLength() const;
	
	virtual const Vector& GetPosition(float dist, const Segment *seg) const;
	virtual const Vector& GetClosestPosition(const Vector& near, const Segment *seg, float dist) const;
	virtual const Vector& GetStartPosition() const;
	virtual const Vector& GetEndPosition() const;
	
	virtual CBaseEntity *GetSubject() const;
	virtual const Segment *GetCurrentGoal() const;
	virtual float GetAge() const;
	
	virtual void MoveCursorToClosestPosition(const Vector& near, SeekType stype, float dist) const;
	virtual void MoveCursorToStart();
	virtual void MoveCursorToEnd();
	virtual void MoveCursor(float dist, MoveCursorType mctype);
	
	virtual float GetCursorPosition() const;
	virtual const CursorData *GetCursorData() const;
	
	virtual bool IsValid() const;
	virtual void Invalidate();
	
	virtual void Draw(const Segment *seg) const;
	virtual void DrawInterpolated(float from, float to);
	
	virtual const Segment *FirstSegment() const;
	virtual const Segment *NextSegment(const Segment *seg) const;
	virtual const Segment *PriorSegment(const Segment *seg) const;
	virtual const Segment *LastSegment() const;
	
	virtual void OnPathChanged(INextBot *nextbot, ResultType rtype);
	
	virtual void Copy(INextBot *nextbot, const Path& that);
	
	virtual bool ComputeWithOpenGoal(INextBot *nextbot, const IPathCost& cost_func, const IPathOpenGoalSelector& sel, float f1);
	virtual void ComputeAreaCrossing(INextBot *nextbot, const CNavArea *area, const Vector& from, const CNavArea *to, NavDirType dir, Vector *out) const;
	
	bool ComputePathDetails(INextBot *nextbot, const Vector& vec);
	
	int FindNextOccludedNode(INextBot *nextbot, int index);
	void InsertSegment(Segment seg, int index);
	
	template<class PathCost> bool Compute(INextBot *nextbot, const Vector& vec, PathCost& cost_func, float maxPathLength, bool b1);
	template<class PathCost> bool Compute(INextBot *nextbot, CBaseCombatCharacter *who, PathCost& cost_func, float maxPathLength, bool b1);
	
protected:
	void AssemblePrecomputedPath(INextBot *nextbot, const Vector& v1, CNavArea *area);
	bool BuildTrivialPath(INextBot *nextbot, const Vector& dest);
	void CollectAdjacentAreas(CNavArea *areas);
	void Optimize(INextBot *nextbot);
	void PostProcess();
	
	Segment m_Segments[256];         // +0x0004
	int m_iSegCount;                 // +0x4404
	Vector m_vecGetPosition;         // +0x4408
	Vector m_vecGetClosestPosition;  // +0x4414
	float m_flCursorPosition;        // +0x4420
	CursorData m_CursorData;         // +0x4424
	bool m_bCursorDataDirty;         // +0x4444
	IntervalTimer m_itAge;           // +0x4448
	CHandle<CBaseEntity> m_hSubject; // +0x444c
	// 4450 
	// ...
	// 4750 dword
	
	#define UNKNOWN_PTR void*
	#define PAD(n, x) char n[x]
	
	PAD(pad1, 0x304);
};
