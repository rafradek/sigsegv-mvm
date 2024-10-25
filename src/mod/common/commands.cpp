#include "mod.h"
#include "stub/baseplayer.h"
#include "stub/misc.h"
#include "util/misc.h"
#include "util/admin.h"
#include "util/clientmsg.h"
#include "mod/common/commands.h"
#include <boost/algorithm/string.hpp>
#include "sp_vm_api.h"

std::unordered_map<std::string, ModCommand *> commands;

void ModConsoleCommand(const CCommand &args);
ModCommand::ModCommand(const char *name, ModCommandCallbackFn callback, IMod *mod, const char *helpString, int flags) : name(name), callback(callback), command(name, ModConsoleCommand, helpString, flags), mod(mod)
{
    commands[name] = this;
}

ModCommand::~ModCommand() 
{
    commands.erase(this->name);
}

enum class ModCommandExecuteType
{
    SERVER_CONSOLE,
    CLIENT_CONSOLE,
    CHAT
};

ModCommandExecuteType last_mod_command_execute_type = ModCommandExecuteType::SERVER_CONSOLE;
CBasePlayer *last_mod_command_execute_player = nullptr;

void ModConsoleCommand(const CCommand &args)
{
    std::string commandLower = args[0];
    boost::algorithm::to_lower(commandLower);
    auto it = commands.find(commandLower.c_str());
    if (it != commands.end() && !(it->second->mod != nullptr && !it->second->mod->IsEnabled())) {
        if (it->second->CanPlayerCall(nullptr)) {
            auto func = it->second->callback;
            last_mod_command_execute_type = ModCommandExecuteType::SERVER_CONSOLE;
            last_mod_command_execute_player = nullptr;
            (*func)(nullptr, args);
        }
        else {
            Msg("[%s] This command cannot be executed in server console.\n", args[0]);
        }
        
        return;
    }
}

void ModCommandResponse(const char *fmt, ...) 
{
    va_list arglist;
	va_start(arglist, fmt);
	char buf[1024];
	vsnprintf(buf, sizeof(buf), fmt, arglist);
	va_end(arglist);
	//CFmtStrN<1024> str(fmt, std::forward<ARGS>(args)...);
	
    switch (last_mod_command_execute_type) {
        case ModCommandExecuteType::SERVER_CONSOLE : Msg("%s", buf); break;
        case ModCommandExecuteType::CHAT : PrintToChat(buf, last_mod_command_execute_player); 
        case ModCommandExecuteType::CLIENT_CONSOLE : engine->ClientPrintf(last_mod_command_execute_player->edict(), buf); break;
    }
}

namespace Mod::Common::Commands
{
    DETOUR_DECL_STATIC(void, Host_Say, edict_t *edict, const CCommand& args, bool team )
	{
		CCommandPlayer *player = (CCommandPlayer*) (GetContainingEntity(edict));
		if (player != nullptr) {
			const char *p = args.ArgS();
			int len = strlen(p) - 1;
			if (*p == '"')
			{
				p++;
				len -=2;
			}
            if (*p != '!' && *p != '/')
			{
                DETOUR_STATIC_CALL(edict, args, team);
                return;
			}
            p++;

            std::string fullCommand(p, len);
            CCommand newArgs;
            newArgs.Tokenize(fullCommand.c_str());

			std::string commandLower = newArgs[0];
            boost::algorithm::to_lower(commandLower);

            auto it = commands.find(commandLower.c_str());
            // Add sig_ prefix if not present and did not find a command with original name
            if (it == commands.end() && !commandLower.starts_with("sig_")) {
                it = commands.find("sig_"s+commandLower.c_str());
                fullCommand.insert(0, "sig_");
                newArgs.Tokenize(fullCommand.c_str());
            }
            if (it != commands.end() && !(it->second->mod != nullptr && !it->second->mod->IsEnabled())) {
                if (it->second->CanPlayerCall(player)) {
                    auto func = it->second->callback;
                    last_mod_command_execute_type = ModCommandExecuteType::CHAT;
                    last_mod_command_execute_player = player;
                    (*func)(player, newArgs);
                }
                else {
                    ClientMsg(player, "[%s] You are not authorized to use this command because you are not a SourceMod admin. Sorry.\n", (*it).first.c_str());
                }
                return;
            }
		}
		DETOUR_STATIC_CALL(edict, args, team);
	}

    DETOUR_DECL_MEMBER(bool, CBasePlayer_ClientCommand, const CCommand& args)
	{
		auto player = reinterpret_cast<CCommandPlayer *>(this);
		if (player != nullptr) {
			std::string commandLower = args[0];
            boost::algorithm::to_lower(commandLower);
            auto it = commands.find(commandLower.c_str());
            if (it != commands.end()) {
                if (it->second->CanPlayerCall(player)) {
                    auto func = it->second->callback;
                    last_mod_command_execute_type = ModCommandExecuteType::CLIENT_CONSOLE;
                    last_mod_command_execute_player = player;
                    (*func)(player, args);
                }
                else {
                    ClientMsg(player, "[%s] You are not authorized to use this command because you are not a SourceMod admin. Sorry.\n", (*it).first.c_str());
                }
                return true;
            }
		}
		
		return DETOUR_MEMBER_CALL(args);
	}

    DETOUR_DECL_MEMBER(void, CServerGameClients_ClientPutInServer, edict_t *edict, const char *playername)
	{
        DETOUR_MEMBER_CALL(edict, playername);
        ClientMsg((CBasePlayer *)GetContainingEntity(edict), "rafradek's srcds performance optimizer loaded\n");
    }

    ModCommand sig_help("sig_help", [](CCommandPlayer *player, const CCommand& args){
        for (auto &[name, command] : commands) {
            if ((command->mod == nullptr || command->mod->IsEnabled()) && command->CanPlayerCall(player)) {
                auto text = command->command.GetHelpText();
                if (last_mod_command_execute_type == ModCommandExecuteType::CHAT) {
                    last_mod_command_execute_type = ModCommandExecuteType::CLIENT_CONSOLE;
                }
                ModCommandResponse("%s %s\n", name.c_str(), text);
            }
        }
    });

    // class PluginCommands : public IPlugin
    // {
    //     virtual PluginType GetType() { return SourceMod::PluginType_MapUpdated; }
    //     virtual SourcePawn::IPluginContext *GetBaseContext() { return nullptr; }
    //     virtual sp_context_t *GetContext() { return nullptr; }
    //     virtual void *GetPluginStructure() { return nullptr; }
    //     virtual const sm_plugininfo_t *GetPublicInfo() { return nullptr; }
    //     virtual const char *GetFilename() { return nullptr; }
    //     virtual bool IsDebugging() { return false; }
    //     virtual PluginStatus GetStatus() { return SourceMod::Plugin_Running; }
    //     virtual bool SetPauseState() { return false; }
    //     virtual unsigned int GetSerial() { return 0; }
	// 	virtual IdentityToken_t *GetIdentity() { return nullptr; }
    //     virtual bool SetProperty(const char *prop, void *ptr) {}
	// 	virtual bool GetProperty(const char *prop, void **ptr, bool remove=false) {}
	// 	virtual SourcePawn::IPluginRuntime *GetRuntime() {}
	// 	virtual IPhraseCollection *GetPhrases() {}
	// 	virtual Handle_t GetMyHandle() {}
    // };

    // class PluginCommandsFunction : public IPluginFunction
    // {
    //     virtual int Execute(cell_t* result) {};
    //     virtual int CallFunction(const cell_t* params, unsigned int num_params, cell_t* result) {};
    //     virtual IPluginContext GetParentContext(const cell_t* params, unsigned int num_params, cell_t* result) {};
    // }
    
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Common:Commands")
		{
            MOD_ADD_DETOUR_STATIC(Host_Say, "Host_Say");
            MOD_ADD_DETOUR_MEMBER(CBasePlayer_ClientCommand, "CBasePlayer::ClientCommand");
            MOD_ADD_DETOUR_MEMBER(CServerGameClients_ClientPutInServer, "CServerGameClients::ClientPutInServer");
		}
        virtual bool EnableByDefault() override { return true; }
	};
	CMod s_Mod;
}
ConVar sig_allow_user_debug_commands("sig_allow_user_debug_commands", 0, FCVAR_NONE, "Allow non admin users to run debug (sig_cpu_usage, sig_print_input) commands");