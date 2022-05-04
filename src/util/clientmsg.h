#ifndef _INCLUDE_SIGSEGV_UTIL_CLIENTMSG_H_
#define _INCLUDE_SIGSEGV_UTIL_CLIENTMSG_H_


#include "stub/baseplayer.h"


// POTENTIAL ALTERNATIVE:
// rather than using engine->ClientPrintf, we could potentially just make use of
// ClientPrint/UTIL_ClientPrintAll/UTIL_ClientPrintFilter, with HUD_PRINTCONSOLE
// so that the destination is the console; probably needs some looking into
// (i.e. does that other method have substantial disadvantages?)


//template<typename... ARGS>
//[[gnu::format(printf, 2, 3)]]
static void ClientMsg(CBasePlayer *player, const char *fmt, .../*ARGS&&... args*/)
{
	va_list arglist;
	va_start(arglist, fmt);
	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, arglist);
	va_end(arglist);
	//CFmtStrN<1024> str(fmt, std::forward<ARGS>(args)...);
	
	engine->ClientPrintf(player->edict(), buf);
}

//template<typename... ARGS>
//[[gnu::format(printf, 1, 2)]]
static void ClientMsgAll(const char *fmt, .../*ARGS&&... args*/)
{
	va_list arglist;
	va_start(arglist, fmt);
	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, arglist);
	va_end(arglist);
	
	for (int i = 1; i <= gpGlobals->maxClients; ++i) {
		CBasePlayer *player = UTIL_PlayerByIndex(i);
		if (player == nullptr)      continue;
		if (player->IsFakeClient()) continue;
		
		engine->ClientPrintf(player->edict(), buf);
	}
}


#endif
