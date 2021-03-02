#include "stub/upgrades.h"


GlobalThunk<CMannVsMachineUpgradeManager> g_MannVsMachineUpgrades("g_MannVsMachineUpgrades");


StaticFuncThunk<unsigned short, const CMannVsMachineUpgrades&, CTFPlayer *, CEconItemView *, int, bool> ft_ApplyUpgrade_Default("ApplyUpgrade_Default");
StaticFuncThunk<int, CTFPlayer *, int, int, int&, bool&> ft_GetUpgradeStepData("GetUpgradeStepData");