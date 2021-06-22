/* reverse engineering by sigsegv
 * based on TF2 version 20151007a
 * server/tf/player_vs_environment/tf_populator_spawners.h
 */


class IPopulationSpawner
{
public:
	IPopulationSpawner(IPopulator *populator);
	virtual ~IPopulationSpawner();
	
	virtual bool Parse(KeyValues *kv) = 0;
	virtual int Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents) = 0;
	virtual bool IsWhereRequired();
	virtual bool IsVarious();
	virtual int GetClass(int index);
	virtual string_t GetClassIcon(int index);
	virtual int GetHealth(int index);
	virtual bool IsMiniBoss(int index);
	virtual bool HasAttribute(CTFBot::AttributeType attr, int index);
	virtual bool HasEventChangeAttributes(const char *name) const = 0;
	
	static IPopulationSpawner *ParseSpawner(IPopulator *populator, KeyValues *kv);
	
protected:
	IPopulator *m_Populator; // +0x04
};


class CMobSpawner : public IPopulationSpawner
{
public:
	CMobSpawner(IPopulator *populator);
	virtual ~CMobSpawner();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual int Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents) override;
	virtual bool HasEventChangeAttributes(const char *name) const override;
	
private:
	int m_iCount;                     // +0x08
	IPopulationSpawner *m_SubSpawner; // +0x0c
	
	/* none of this stuff is used by any of the code */
	CountdownTimer m_UnusedTimer1;    // +0x10
	CountdownTimer m_UnusedTimer2;    // +0x1c
	int m_UnusedDword1;               // +0x28
	int m_UnusedDword2;               // +0x2c
};


class CSentryGunSpawner : public IPopulationSpawner
{
public:
	CSentryGunSpawner(IPopulator *populator);
	virtual ~CSentryGunSpawner();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual int Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents) override;
	virtual bool HasEventChangeAttributes(const char *name) const override;
	
private:
	int m_iLevel; // +0x08
};


class CTankSpawner : public IPopulationSpawner
{
public:
	CTankSpawner(IPopulator *populator);
	virtual ~CTankSpawner();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual int Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents) override;
	virtual bool IsWhereRequired() override;
	virtual string_t GetClassIcon(int index) override;
	virtual int GetHealth(int index) override;
	virtual bool IsMiniBoss(int index) override;
	virtual bool HasEventChangeAttributes(const char *name) const override;
	
private:
	int m_iHealth;                    // +0x08
	float m_flSpeed;                  // +0x0c
	CUtlString m_strName;             // +0x10
	CUtlString m_strStartNode;        // +0x14
	int m_iSkin;                      // +0x18
	EventInfo *m_OnKilledOutput;      // +0x1c
	EventInfo *m_OnBombDroppedOutput; // +0x20
};


class CTFBotSpawner : public IPopulationSpawner
{
public:
	CTFBotSpawner(IPopulator *populator);
	virtual ~CTFBotSpawner();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual int Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents) override;
	virtual int GetClass(int index) override;
	virtual string_t GetClassIcon(int index) override;
	virtual int GetHealth(int index) override;
	virtual bool IsMiniBoss(int index) override;
	virtual bool HasAttribute(CTFBot::AttributeType attr, int index) override;
	virtual bool HasEventChangeAttributes(const char *name) const override;
	
private:
	bool ParseEventChangeAttributes(KeyValues *kv);
	
	int m_iClass;                                          // +0x08
	string_t m_strClassIcon;                               // +0x0c
	int m_iHealth;                                         // +0x10
	float m_flScale;                                       // +0x14
	float m_flAutoJumpMin;                                 // +0x18
	float m_flAutoJumpMax;                                 // +0x1c
	CUtlString m_strName;                                  // +0x20
	CUtlStringList m_TeleportWhere;                        // +0x24
	CTFBot::EventChangeAttributes_t m_DefaultAttrs;        // +0x38
	CUtlVector<CTFBot::EventChangeAttributes_t> m_ECAttrs; // +0xa4
};


class CSquadSpawner : public IPopulationSpawner
{
public:
	CSquadSpawner(IPopulator *populator);
	virtual ~CSquadSpawner();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual int Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents) override;
	virtual bool IsVarious() override;
	virtual int GetClass(int index) override;
	virtual string_t GetClassIcon(int index) override;
	virtual int GetHealth(int index) override;
	virtual bool IsMiniBoss(int index) override;
	virtual bool HasAttribute(CTFBot::AttributeType attr, int index) override;
	virtual bool HasEventChangeAttributes(const char *name) const override;
	
private:
	CUtlVectorAutoPurge<IPopulationSpawner *> m_SubSpawners; // +0x08
	float m_flFormationSize;                                 // +0x1c
	bool m_bShouldPreserveSquad;                             // +0x20
};


class CRandomChoiceSpawner : public IPopulationSpawner
{
public:
	CRandomChoiceSpawner(IPopulator *populator);
	virtual ~CRandomChoiceSpawner();
	
	virtual bool Parse(KeyValues *kv) override;
	virtual int Spawn(const Vector& where, CUtlVector<CHandle<CBaseEntity>> *ents) override;
	virtual bool IsVarious() override;
	virtual int GetClass(int index) override;
	virtual string_t GetClassIcon(int index) override;
	virtual int GetHealth(int index) override;
	virtual bool IsMiniBoss(int index) override;
	virtual bool HasAttribute(CTFBot::AttributeType attr, int index) override;
	virtual bool HasEventChangeAttributes(const char *name) const override;
	
private:
	void GenerateRandomIndexes(int index);
	
	CUtlVectorAutoPurge<IPopulationSpawner *> m_SubSpawners; // +0x08
	CUtlVector<int> m_Indexes;                               // +0x1c
	int m_iSpawned;                                          // +0x30
};


bool ParseDynamicAttributes(CTFBot::EventChangeAttributes_t& ecattr, KeyValues *kv);
// TODO: IsSpaceToSpawnHere


class CPopulatorInternalSpawnPoint : public CPointEntity
{
public:
	// TODO
	
private:
	// TODO
};
