#ifndef _INCLUDE_SIGSEGV_STUB_TF_OBJECTIVE_RESOURCE_H_
#define _INCLUDE_SIGSEGV_STUB_TF_OBJECTIVE_RESOURCE_H_


#include "stub/baseentity.h"
#include "prop.h"
#include "link/link.h"


enum MvMWaveClassFlags : uint32_t
{
	CLASSFLAG_NORMAL          = (1 << 0), // set for non-support bots and tanks
	CLASSFLAG_SUPPORT         = (1 << 1), // set for "support 1" and "support limited" bots
	CLASSFLAG_MISSION         = (1 << 2), // set for mission support bots and teleporters
	CLASSFLAG_MINIBOSS        = (1 << 3), // set for minibosses and tanks (red background)
	CLASSFLAG_CRITICAL        = (1 << 4), // set for crit bots (blue border)
	CLASSFLAG_SUPPORT_LIMITED = (1 << 5), // set for "support limited" bots
};


class CBaseTeamObjectiveResource : public CBaseEntity
{
public:
	
private:
	
};

class CTFObjectiveResource : public CBaseTeamObjectiveResource
{
public:

	void DecrementMannVsMachineWaveClassCount(string_t iconName, unsigned int flags) {ft_DecrementMannVsMachineWaveClassCount(this, iconName, flags);};

	DECL_SENDPROP(unsigned int,     m_nMannVsMachineMaxWaveCount);
	DECL_SENDPROP(unsigned int,     m_nMannVsMachineWaveCount);
	DECL_SENDPROP(unsigned int,     m_nMannVsMachineWaveEnemyCount);
	DECL_SENDPROP(unsigned int,     m_nMvMWorldMoney);
	DECL_SENDPROP(float,            m_flMannVsMachineNextWaveTime);
	DECL_SENDPROP(bool,             m_bMannVsMachineBetweenWaves);
	DECL_SENDPROP_RW(int[12],          m_nMannVsMachineWaveClassCounts);
	DECL_SENDPROP_RW(int[12],          m_nMannVsMachineWaveClassCounts2);
	DECL_SENDPROP_RW(string_t[12],     m_iszMannVsMachineWaveClassNames);
	DECL_SENDPROP_RW(string_t[12],     m_iszMannVsMachineWaveClassNames2);
	DECL_SENDPROP_RW(unsigned int[12], m_nMannVsMachineWaveClassFlags);
	DECL_SENDPROP_RW(unsigned int[12], m_nMannVsMachineWaveClassFlags2);
	DECL_SENDPROP_RW(bool[12],         m_bMannVsMachineWaveClassActive);
	DECL_SENDPROP_RW(bool[12],         m_bMannVsMachineWaveClassActive2);
	DECL_SENDPROP(unsigned int,     m_nFlagCarrierUpgradeLevel);
	DECL_SENDPROP(float,            m_flMvMBaseBombUpgradeTime);
	DECL_SENDPROP(float,            m_flMvMNextBombUpgradeTime);
	DECL_SENDPROP(int,              m_iChallengeIndex);
	DECL_SENDPROP(string_t,         m_iszMvMPopfileName);
	DECL_SENDPROP(unsigned int,     m_nMvMEventPopfileType);

private:
	static MemberFuncThunk<CTFObjectiveResource *, void, string_t, unsigned int > ft_DecrementMannVsMachineWaveClassCount;

};


extern GlobalThunk<CBaseTeamObjectiveResource *> g_pObjectiveResource;
inline CBaseTeamObjectiveResource *ObjectiveResource()   { return g_pObjectiveResource; }
inline CTFObjectiveResource       *TFObjectiveResource() { return reinterpret_cast<CTFObjectiveResource *>(ObjectiveResource()); }


#endif
