#ifndef _INCLUDE_SIGSEGV_STUB_POPULATORS_H_
#define _INCLUDE_SIGSEGV_STUB_POPULATORS_H_


#include "link/link.h"
#include "stub/tfbot.h"


class CWave;


class CPopulationManager : public CPointEntity
{
public:
	bool LoadMvMMission(KeyValues *kv) { return ft_LoadMvMMission(this, kv); }
	CWave *GetCurrentWave()            { return ft_GetCurrentWave(this); }
	void ResetMap()                    {        ft_ResetMap      (this); }
	void PauseSpawning()               {        ft_PauseSpawning (this); }
	void UnpauseSpawning()             {        ft_UnpauseSpawning (this); }
	void AllocateBots()	               {        ft_AllocateBots (this); }
	static int CollectMvMBots(CUtlVector<CTFPlayer *> *mvm_bots) { return ft_CollectMvMBots(mvm_bots); }
	void RemovePlayerAndItemUpgradesFromHistory( CTFPlayer *pPlayer ) { return ft_RemovePlayerAndItemUpgradesFromHistory(this, pPlayer); }
	
	using SteamIDMap = CUtlMap<uint64_t, int>;
	DECL_EXTRACT(SteamIDMap, m_RespecPoints);
	DECL_EXTRACT(bool,       m_bAllocatedBots);
	DECL_EXTRACT(KeyValues *,m_pTemplates);
	
private:
	static MemberFuncThunk<CPopulationManager *, bool, KeyValues *> ft_LoadMvMMission;
	static MemberFuncThunk<CPopulationManager *, CWave *>           ft_GetCurrentWave;
	static MemberFuncThunk<CPopulationManager *, void>              ft_ResetMap;
	static MemberFuncThunk<CPopulationManager *, void>              ft_PauseSpawning;
	static MemberFuncThunk<CPopulationManager *, void>              ft_UnpauseSpawning;
	static MemberFuncThunk<CPopulationManager *, void>              ft_AllocateBots;
	static MemberFuncThunk<CPopulationManager *, void, CTFPlayer *>             ft_RemovePlayerAndItemUpgradesFromHistory;
	
	static StaticFuncThunk<int, CUtlVector<CTFPlayer *> *> ft_CollectMvMBots;
};
extern GlobalThunk<CPopulationManager *> g_pPopulationManager;


class IPopulationSpawner;


class IPopulator
{
public:
	void **vtable;
	IPopulationSpawner *m_Spawner;
	CPopulationManager *m_PopMgr;
};


class CRandomPlacementPopulator : public IPopulator {};
class CPeriodicSpawnPopulator   : public IPopulator {};
class CWaveSpawnPopulator       : public IPopulator {
	
};

class CWave : public IPopulator
{
public:
	void AddClassType(string_t icon, int count, unsigned int flags) { ft_AddClassType      (this, icon, count, flags); }
	void ForceFinish()                                              { ft_ForceFinish       (this); }
	void ActiveWaveUpdate()                                         { ft_ActiveWaveUpdate  (this); }
	void WaveCompleteUpdate()                                       { ft_WaveCompleteUpdate(this); }
	bool IsDoneWithNonSupportWaves()                                { return ft_IsDoneWithNonSupportWaves(this); }
	//CUtlVector<CWaveSpawnPopulator *> GetWaveSpawns()	            { return reinterpret_cast<T>((uintptr_t)this + 0x18 - 0x0c); }
	
	CUtlVector<CWaveSpawnPopulator *> m_WaveSpawns;
	
private:
	static MemberFuncThunk<CWave *, void, string_t, int, unsigned int> ft_AddClassType;
	static MemberFuncThunk<CWave *, void>                              ft_ForceFinish;
	static MemberFuncThunk<CWave *, void>                              ft_ActiveWaveUpdate;
	static MemberFuncThunk<CWave *, void>                              ft_WaveCompleteUpdate;
	static MemberFuncThunk<CWave *, bool>                              ft_IsDoneWithNonSupportWaves;
};

class CMissionPopulator : public IPopulator
{
public:
	bool UpdateMission(int mtype) { return ft_UpdateMission(this, mtype); }
	
	int m_Objective;
	
private:
	static MemberFuncThunk<CMissionPopulator *, bool, int> ft_UpdateMission;
};


class IPopulationSpawner
{
public:
	string_t GetClassIcon(int index)                         { return vt_GetClassIcon(this, index); }
	bool IsMiniBoss(int index)                               { return vt_IsMiniBoss  (this, index); }
	bool HasAttribute(CTFBot::AttributeType attr, int index) { return vt_HasAttribute(this, attr, index); }
	bool Parse(KeyValues *kv)                                { return vt_Parse(this, kv); }

	void **vtable;
	IPopulator *m_Populator;
	
private:
	static MemberVFuncThunk<IPopulationSpawner *, string_t, int                   > vt_GetClassIcon;
	static MemberVFuncThunk<IPopulationSpawner *, bool, int                       > vt_IsMiniBoss;
	static MemberVFuncThunk<IPopulationSpawner *, bool, CTFBot::AttributeType, int> vt_HasAttribute;
	static MemberVFuncThunk<IPopulationSpawner *, bool, KeyValues *               > vt_Parse;
};

class CMobSpawner : public IPopulationSpawner
{
public:
	int m_iCount;
	IPopulationSpawner *m_SubSpawner;
};

class CSentryGunSpawner : public IPopulationSpawner {};
class CTankSpawner      : public IPopulationSpawner {};

class CTFBotSpawner : public IPopulationSpawner
{
public:
	int m_iClass;
	string_t m_strClassIcon;
	int m_iHealth;
	float m_flScale;
	float m_flAutoJumpMin;
	float m_flAutoJumpMax;
	CUtlString m_strName;
	CUtlStringList m_TeleportWhere;
	CTFBot::EventChangeAttributes_t m_DefaultAttrs;
	CUtlVector<CTFBot::EventChangeAttributes_t> m_ECAttrs;
	
};

class CSquadSpawner : public IPopulationSpawner
{
public:
	CUtlVector<IPopulationSpawner *> m_SubSpawners;

	float m_formationSize;
	char m_bShouldPreserveSquad;
};

class CRandomChoiceSpawner : public IPopulationSpawner
{
public:
	CUtlVector<IPopulationSpawner *> m_SubSpawners;
	CUtlVector<int> m_Indexes;
};

struct EventInfo
{
	CFmtStrN<256> target; // +0x000
	CFmtStrN<256> action; // +0x10c
};

extern StaticFuncThunk<bool, const Vector&> ft_IsSpaceToSpawnHere;
extern StaticFuncThunk<bool, CTFBot::EventChangeAttributes_t &, KeyValues *> ft_ParseDynamicAttributes;


inline bool IsSpaceToSpawnHere(const Vector& pos)
{
	return ft_IsSpaceToSpawnHere(pos);
}

inline bool ParseDynamicAttributes(CTFBot::EventChangeAttributes_t &event, KeyValues *kv)
{
	return ft_ParseDynamicAttributes(event, kv);
}
#endif
