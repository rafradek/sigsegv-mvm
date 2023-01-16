#ifndef _INCLUDE_SIGSEGV_MOD_COMMON_COMMANDS_H_
#define _INCLUDE_SIGSEGV_MOD_COMMON_COMMANDS_H_

typedef void (*ModCommandCallbackFn)(CTFPlayer *, const CCommand&);

#include "util/admin.h"

void ModCommandResponse(const char *fmt, ...);

class ModCommand
{
public:
    ModCommand(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0);

    ~ModCommand();

    virtual bool CanPlayerCall(CTFPlayer *player)
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

    virtual bool CanPlayerCall(CTFPlayer *player)
    {
        return player == nullptr || PlayerIsSMAdminOrBot(player);
    }
};

class ModCommandClient : public ModCommand
{
public:
    ModCommandClient(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

    virtual bool CanPlayerCall(CTFPlayer *player)
    {
        return player != nullptr;
    }
};

class ModCommandClientAdmin : public ModCommand
{
public:
    ModCommandClientAdmin(const char *name, ModCommandCallbackFn callback, IMod *mod = nullptr, const char *helpString = "", int flags = 0) : ModCommand(name, callback, mod, helpString, flags) {}

    virtual bool CanPlayerCall(CTFPlayer *player)
    {
        return player != nullptr && PlayerIsSMAdminOrBot(player);
    }
};

#endif