#ifndef _INCLUDE_SIGSEGV_STUB_PLAYER_UTIL_H_
#define _INCLUDE_SIGSEGV_STUB_PLAYER_UTIL_H_

#include "link/link.h"
#include "stub/tfplayer.h"

void HealPlayer(CTFPlayer *patient, CTFPlayer *healer, CEconEntity *item, float amount, bool isOnHit, int damageType);
#endif