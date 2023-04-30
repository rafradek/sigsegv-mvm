#ifndef _INCLUDE_SIGSEGV_MOD_MVM_EXTENDED_UPGRADES_H_
#define _INCLUDE_SIGSEGV_MOD_MVM_EXTENDED_UPGRADES_H_

namespace Mod::MvM::Extended_Upgrades
{

	void Parse_ExtendedUpgrades(KeyValues *kv, bool v2);
	void ClearUpgrades();
	int GetExtendedUpgradesStartIndex();
}

#endif
