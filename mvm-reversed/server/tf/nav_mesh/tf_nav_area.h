/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/nav_mesh/tf_nav_area.h
 */


// TODO: figure out if this belongs here or elsewhere
enum TFNavAttributeType : int
{
	BLOCKED                     = (1 <<  0),
	
	RED_SPAWN_ROOM              = (1 <<  1),
	BLUE_SPAWN_ROOM             = (1 <<  2),
	SPAWN_ROOM_EXIT             = (1 <<  3),
	
	AMMO                        = (1 <<  4),
	HEALTH                      = (1 <<  5),
	
	CONTROL_POINT               = (1 <<  6),
	
	BLUE_SENTRY                 = (1 <<  7),
	RED_SENTRY                  = (1 <<  8),
	
	/* bit  9: unused */
	/* bit 10: unused */
	
	BLUE_SETUP_GATE             = (1 << 11),
	RED_SETUP_GATE              = (1 << 12),
	
	BLOCKED_AFTER_POINT_CAPTURE = (1 << 13),
	BLOCKED_UNTIL_POINT_CAPTURE = (1 << 14),
	
	BLUE_ONE_WAY_DOOR           = (1 << 15),
	RED_ONE_WAY_DOOR            = (1 << 16),
	
	WITH_SECOND_POINT           = (1 << 17),
	WITH_THIRD_POINT            = (1 << 18),
	WITH_FOURTH_POINT           = (1 << 19),
	WITH_FIFTH_POINT            = (1 << 20),
	
	SNIPER_SPOT                 = (1 << 21),
	SENTRY_SPOT                 = (1 << 22),
	
	/* bit 23: unused */
	/* bit 24: unused */
	
	NO_SPAWNING                 = (1 << 25),
	RESCUE_CLOSET               = (1 << 26),
	BOMB_DROP                   = (1 << 27),
	DOOR_NEVER_BLOCKS           = (1 << 28),
	DOOR_ALWAYS_BLOCKS          = (1 << 29),
	UNBLOCKABLE                 = (1 << 30),
	
	/* bit 31: unused */
};


class CTFNavArea : public CNavArea
{
public:
	CTFNavArea();
	virtual ~CTFNavArea();
	
	virtual void OnServerActivate() override;
	virtual void OnRoundRestart() override;
	
	virtual void Save(CUtlBuffer& fileBuffer, unsigned int version) const override;
	virtual NavErrorType Load(CUtlBuffer& fileBuffer, unsigned int version, unsigned int subVersion) override;
	
	virtual void UpdateBlocked(bool force = false, int teamID = TEAM_ANY) override;
	virtual bool IsBlocked(int teamID, bool ignoreNavBlockers = false) const override;
	
	virtual void Draw() const override;
	
	virtual void CustomAnalysis(bool isIncremental = false) override;
	
	virtual bool IsPotentiallyVisibleToTeam(int team) const override;
	
	void AddPotentiallyVisibleActor(CBaseCombatCharacter *actor);
	
	void CollectNextIncursionAreas(int i1, CUtlVector<CTFNavArea *> *areas);
	void CollectPriorIncursionAreas(int i1, CUtlVector<CTFNavArea *> *areas);
	CTFNavArea *GetNextIncursionArea(int i1) const;
	
	void ComputeInvasionAreaVectors();
	bool IsAwayFromInvasionAreas(int i1, float f1) const;
	
	float GetCombatIntensity() const;
	bool IsInCombat() const;
	void OnCombat();
	
	void ResetTFMarker();
	void MakeNewTFMarker();
	bool IsTFMarked() const;
	void TFMark();
	
	bool IsValidForWanderingPopulation() const;
	
	// float GetIncursionDistance(int teamnum)
	// (returns -1.0f if teamnum is out of bounds [0,3])
	
private:
	// 160 
	// 164 
	// 168 float
	// 16c float
	CUtlVector<CTFNavArea *> m_InvasionAreas[4]; // +0x170
	// 1c0 
	TFNavAttributeType m_nAttributes; // +0x1c4
	CUtlVector<CBaseCombatCharacter *> m_PVActors[4]; // +0x1c8
	// 218 
	IntervalTimer m_itCombat; // +0x21c
	// 220 mark
	// 224 
	float m_flBombTargetDistance; // +0x228
	
	// TODO: static m_masterTFMark
};
