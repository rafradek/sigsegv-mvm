#include "stub/tf_player_resource.h"

IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CPlayerResource, m_iTeam, CPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CPlayerResource, m_bValid, CPlayerResource);
IMPL_SENDPROP(uint32[MAX_PLAYERS + 1],          CPlayerResource, m_iAccountID, CPlayerResource);

IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iDamage, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iTotalScore, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iDamageBoss, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iHealing, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iCurrencyCollected, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iDamageAssist, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iHealingAssist, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iBonusPoints, CTFPlayerResource);
IMPL_SENDPROP(int[MAX_PLAYERS + 1],          CTFPlayerResource,          m_iDamageBlocked, CTFPlayerResource);

GlobalThunk<CPlayerResource *> g_pPlayerResource("g_pPlayerResource");