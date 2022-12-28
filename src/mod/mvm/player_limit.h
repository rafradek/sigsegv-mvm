#ifndef _INCLUDE_SIGSEGV_MOD_MVM_PLAYER_LIMIT_H_
#define _INCLUDE_SIGSEGV_MOD_MVM_PLAYER_LIMIT_H_

namespace Mod::MvM::Player_Limit
{

	int GetSlotCounts(int &red, int &blu, int &spectators, int &robots);
    void ToggleModActive();
    void ResetMaxTotalPlayers();
    void SetVisibleMaxPlayers();
    void RecalculateSlots();
}

#endif