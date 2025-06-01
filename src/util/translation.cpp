#include "stub/misc.h"
#include "stub/baseplayer.h"
#include "util/translation.h"

const char *FormatTextForPlayerSM_VA(CBasePlayer *player, int paramCount, const char *fmt, va_list arglist, const char *param0 = nullptr)
{
	static char buf[1024];
	void *params[MAX_TRANSLATE_PARAMS];
	params[0] = (void *)param0;
	for (int i = param0 == nullptr ? 0 : 1; i < paramCount; i++) {
		params[i] = va_arg(arglist, void *);
	}
	int oldClient = translator->SetGlobalTarget(ENTINDEX(player));
	size_t size = 0;
	bool success = phrases->FormatString(buf, sizeof(buf), fmt, params,paramCount, &size, nullptr);
	translator->SetGlobalTarget(oldClient);
	if (!success) return fmt;
	return buf;
}

const char *_FormatTextForPlayerSM(CBasePlayer *player, int paramCount, const char *fmt, ...)
{
    va_list arglist;
	va_start(arglist, fmt);
	auto value = FormatTextForPlayerSM_VA(player, paramCount, fmt, arglist);
	va_end(arglist);
	return value;
}

void _PrintToChatAllSM(int paramCount, const char *fmt,...)
{
	for (int i = 1; i <= gpGlobals->maxClients; i++) {
		auto player = UTIL_PlayerByIndex(i);
		if (player != nullptr && !player->IsBot()) {
			va_list arglist;
			va_start(arglist, fmt);
			PrintToChat(FormatTextForPlayerSM_VA(player,paramCount, fmt, arglist), player);
			va_end(arglist);
		}
	}
}

void _PrintToChatSM(CBasePlayer *player, int paramCount, const char *fmt,...)
{
	va_list arglist;
	va_start(arglist, fmt);
	PrintToChat(FormatTextForPlayerSM_VA(player,paramCount, fmt,arglist), player);
	va_end(arglist);
}

const char *_TranslateText_Varargs(CBasePlayer *player, const char *name, int paramCount, ...)
{
	if (paramCount == 0) {
		return _TranslateText_NoParams(player, name);
	}
	va_list arglist;
	va_start(arglist, paramCount);
	auto value = FormatTextForPlayerSM_VA(player,paramCount + 1, "%t",arglist, name);
	va_end(arglist);
	return value;
}

const char *_TranslateText_NoParams(CBasePlayer *player, const char *name)
{
	Translation buf;
	if (phrasesFile->GetTranslation(name, translator->GetClientLanguage(ENTINDEX(player)), &buf) != Trans_Okay) {
		if (phrasesFile->GetTranslation(name, translator->GetServerLanguage(), &buf) != Trans_Okay) {
			return name;
		}
	}
	return buf.szPhrase; 
}