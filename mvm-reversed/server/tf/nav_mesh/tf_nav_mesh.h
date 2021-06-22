/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/nav_mesh/tf_nav_mesh.h
 */


class CTFNavMesh : public CNavMesh
{
public:
	CTFNavMesh();
	virtual ~CTFNavMesh();
	
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual CNavArea *CreateArea() const override;
	
	virtual void Update() override;
	
	virtual bool IsAuthoritative() const override;
	
	virtual unsigned int GetSubVersionNumber() const override;
	virtual void SaveCustomData(CUtlBuffer& fileBuffer) const override;
	virtual void LoadCustomData(CUtlBuffer& fileBuffer, unsigned int subVersion) const override;
	
	virtual void OnServerActivate() override;
	virtual void OnRoundRestart() override;
	
	virtual unsigned int GetGenerationTraceMask() const override;
	
	virtual void PostCustomAnalysis() override;
	
	virtual void BeginCustomAnalysis(bool bIncremental) override;
	virtual void EndCustomAnalysis() override;
	
	void CollectAmbushAreas(CUtlVector<CTFNavArea *> *areas, CTFNavArea *area, int i1, float f1, float f2) const;
	void CollectAreaWithinBombTravelRange(CUtlVector<CTFNavArea *> *areas, float f1, float f2) const;
	void CollectBuiltObjects(CUtlVector<CBaseObject *> *objects, int i1);
	void CollectSpawnRoomThresholdAreas(CUtlVector<CTFNavArea *> *areas, int i1) const;
	
	bool IsSentryGunHere(CTFNavArea *area) const;
	void OnBlockedAreasChanged();
	
private:
	void CollectAndMarkSpawnRoomExits(CTFNavArea *area, CUtlVector<CTFNavArea *> *areas);
	void CollectControlPointAreas();
	void ComputeBlockedAreas();
	void ComputeBombTargetDistance();
	void ComputeIncursionDistances();
	void ComputeIncursionDistances(CTFNavArea *area, int i1);
	void ComputeInvasionAreas();
	void ComputeLegalBombDropAreas();
	void DecorateMesh();
	void OnObjectChanged();
	void RecomputeInternalData();
	void RemoveAllMeshDecoration();
	void ResetMeshAttributes(bool b1);
	void UpdateDebugDisplay() const;
	
	// 5cc CountdownTimer
	// 5d8 int
	// 5dc dword, perhaps IGameEvent*
	// 5e0 CUtlVector<CTFNavArea *>
	// 5f4 CUtlVector<CTFNavArea *>
	// 608 CUtlVector<CTFNavArea *> [8]
	// 6a8 CTFNavArea* [8]
	// 6c8 CUtlVector<CTFNavArea *>
	// 6dc CUtlVector<CTFNavArea *>
	// 6f0 CUtlVector<CTFNavArea *>
	// 704 CUtlVector<CTFNavArea *>
	// 718 CountdownTimer
	// 724 
};
