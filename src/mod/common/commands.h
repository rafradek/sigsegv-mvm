#ifndef _INCLUDE_SIGSEGV_MOD_COMMON_COMMANDS_H_
#define _INCLUDE_SIGSEGV_MOD_COMMON_COMMANDS_H_

#ifdef SE_TF2
#include "stub/tfplayer.h"
#define CCommandPlayer CTFPlayer
#else
#include "stub/baseplayer.h"
#define CCommandPlayer CBasePlayer
#endif

#include "util/admin.h"

typedef void (*ModCommandCallbackFn)(CCommandPlayer *, const CCommand&);

void ModCommandResponse(const char *fmt, ...);

class ModCommand
{
public:
    ModCommand(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0);

    ~ModCommand();

    virtual bool CanPlayerCall(CCommandPlayer *player)
    {
        return true;
    }

    const char *name;
    ModCommandCallbackFn callback;
    ConCommand command;
    IMod *mod;
};

class ModCommandAdmin : public ModCommand
{
public:
    ModCommandAdmin(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

    virtual bool CanPlayerCall(CCommandPlayer *player)
    {
        return player == nullptr || PlayerIsSMAdminOrBot(player);
    }
};

class ModCommandClient : public ModCommand
{
public:
    ModCommandClient(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

    virtual bool CanPlayerCall(CCommandPlayer *player)
    {
        return player != nullptr;
    }
};

class ModCommandClientAdmin : public ModCommand
{
public:
    ModCommandClientAdmin(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

    virtual bool CanPlayerCall(CCommandPlayer *player)
    {
        return player != nullptr && PlayerIsSMAdminOrBot(player);
    }
};

extern ConVar sig_allow_user_debug_commands;
class ModCommandDebug : public ModCommand
{
public:
    ModCommandDebug(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

    virtual bool CanPlayerCall(CCommandPlayer *player)
    {
        return player == nullptr || (PlayerIsSMAdminOrBot(player) || sig_allow_user_debug_commands.GetBool());
    }
};

#endif