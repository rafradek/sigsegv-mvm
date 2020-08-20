#include "stub/tf_objective_resource.h"


IMPL_SENDPROP(unsigned int,     CTFObjectiveResource, m_nMannVsMachineMaxWaveCount,      CTFObjectiveResource);
IMPL_SENDPROP(unsigned int,     CTFObjectiveResource, m_nMannVsMachineWaveCount,         CTFObjectiveResource);
IMPL_SENDPROP(unsigned int,     CTFObjectiveResource, m_nMannVsMachineWaveEnemyCount,    CTFObjectiveResource);
IMPL_SENDPROP(unsigned int,     CTFObjectiveResource, m_nMvMWorldMoney,                  CTFObjectiveResource);
IMPL_SENDPROP(float,            CTFObjectiveResource, m_flMannVsMachineNextWaveTime,     CTFObjectiveResource);
IMPL_SENDPROP(bool,             CTFObjectiveResource, m_bMannVsMachineBetweenWaves,      CTFObjectiveResource);
IMPL_SENDPROP(int[12],          CTFObjectiveResource, m_nMannVsMachineWaveClassCounts,   CTFObjectiveResource);
IMPL_SENDPROP(int[12],          CTFObjectiveResource, m_nMannVsMachineWaveClassCounts2,  CTFObjectiveResource);
IMPL_SENDPROP(string_t[12],     CTFObjectiveResource, m_iszMannVsMachineWaveClassNames,  CTFObjectiveResource);
IMPL_SENDPROP(string_t[12],     CTFObjectiveResource, m_iszMannVsMachineWaveClassNames2, CTFObjectiveResource);
IMPL_SENDPROP(unsigned int[12], CTFObjectiveResource, m_nMannVsMachineWaveClassFlags,    CTFObjectiveResource);
IMPL_SENDPROP(unsigned int[12], CTFObjectiveResource, m_nMannVsMachineWaveClassFlags2,   CTFObjectiveResource);
IMPL_SENDPROP(bool[12],         CTFObjectiveResource, m_bMannVsMachineWaveClassActive,   CTFObjectiveResource);
IMPL_SENDPROP(bool[12],         CTFObjectiveResource, m_bMannVsMachineWaveClassActive2,  CTFObjectiveResource);
IMPL_SENDPROP(unsigned int,     CTFObjectiveResource, m_nFlagCarrierUpgradeLevel,        CTFObjectiveResource);
IMPL_SENDPROP(float,            CTFObjectiveResource, m_flMvMBaseBombUpgradeTime,        CTFObjectiveResource);
IMPL_SENDPROP(float,            CTFObjectiveResource, m_flMvMNextBombUpgradeTime,        CTFObjectiveResource);
IMPL_SENDPROP(int,              CTFObjectiveResource, m_iChallengeIndex,                 CTFObjectiveResource);
IMPL_SENDPROP(string_t,         CTFObjectiveResource, m_iszMvMPopfileName,               CTFObjectiveResource);
IMPL_SENDPROP(unsigned int,     CTFObjectiveResource, m_nMvMEventPopfileType,            CTFObjectiveResource);

GlobalThunk<CBaseTeamObjectiveResource *> g_pObjectiveResource("g_pObjectiveResource");

MemberFuncThunk<CTFObjectiveResource *, void, string_t, unsigned int > CTFObjectiveResource::ft_DecrementMannVsMachineWaveClassCount ("CTFObjectiveResource::DecrementMannVsMachineWaveClassCount");