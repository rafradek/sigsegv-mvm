#ifndef _INCLUDE_SIGSEGV_STUB_UPGRADES_H_
#define _INCLUDE_SIGSEGV_STUB_UPGRADES_H_


#include "util/misc.h"
#include "link/link.h"
#include "stub/econ.h"


struct CMannVsMachineUpgrades
{
	char m_szAttribute[0x40]; // +0x000
	char m_szIcon[MAX_PATH];  // +0x040
	float m_flIncrement;      // +0x144
	float m_flCap;            // +0x148
	int m_nCost;              // +0x14c
	int m_iUIGroup;           // +0x150
	int m_iQuality;           // +0x154
	int m_iTier;              // +0x158
};
SIZE_CHECK(CMannVsMachineUpgrades, 0x15c);


class CMannVsMachineUpgradeManager;
extern GlobalThunk<CMannVsMachineUpgradeManager> g_MannVsMachineUpgrades;

class CMannVsMachineUpgradeManager : public CAutoGameSystem
{
public:
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPostEntity();

	static CUtlVector<CMannVsMachineUpgrades>& Upgrades()
	{
		CMannVsMachineUpgradeManager& instance = g_MannVsMachineUpgrades;
		return instance.m_Upgrades;
	}
	
private:
	CUtlVector<CMannVsMachineUpgrades> m_Upgrades;
	CUtlMap<const char *, int> m_UpgradeMap;
};
#ifdef PLATFORM_64BITS
SIZE_CHECK(CMannVsMachineUpgradeManager, 0x60);
#else
SIZE_CHECK(CMannVsMachineUpgradeManager, 0x3c);
#endif

extern StaticFuncThunk<attrib_definition_index_t, const CMannVsMachineUpgrades&, CTFPlayer *, CEconItemView *, int, bool> ft_ApplyUpgrade_Default;
extern StaticFuncThunk<int, CTFPlayer *, int, int, int&, bool&> ft_GetUpgradeStepData;

inline int GetUpgradeStepData( CTFPlayer *pPlayer, int nWeaponSlot, int nUpgradeIndex, int &nCurrentStep, bool &bOverCap )
{
	return ft_GetUpgradeStepData(pPlayer, nWeaponSlot, nUpgradeIndex, nCurrentStep, bOverCap);
}

inline attrib_definition_index_t ApplyUpgrade_Default( const CMannVsMachineUpgrades& upgrade, CTFPlayer *pTFPlayer, CEconItemView *pEconItemView, int nCost, bool bDowngrade )
{
	return ft_ApplyUpgrade_Default(upgrade, pTFPlayer, pEconItemView, nCost, bDowngrade);
}

#endif
