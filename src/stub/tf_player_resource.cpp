#include "stub/tf_player_resource.h"

IMPL_SENDPROP(int[34],          CPlayerResource, m_iTeam, CPlayerResource);
IMPL_SENDPROP(int[34],          CPlayerResource, m_bValid, CPlayerResource);

GlobalThunk<CPlayerResource *> g_pPlayerResource("g_pPlayerResource");