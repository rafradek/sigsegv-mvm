#include "mod.h"
#include "vscript/ivscript.h"
#include "mem/protect.h"
#include "link/link.h"
#include "util/admin.h"
#include "stub/baseplayer.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "stub/misc.h"
#include <mod/common/commands.h>

namespace Mod::Util::VScript_Send_Output_To_Admins
{

	float lastNotifyTime = 0;
	int errorCounter = 0;
	GlobalThunk<IScriptVM *> g_pScriptVM("g_pScriptVM"); 

	void Output(const char *text) {
		ForEachPlayer([text](CBasePlayer *player){
			if (player->IsRealPlayer() && PlayerIsSMAdmin(player)) {
				ClientMsg(player, "%s", text);
			}
		});
	};

	int lastErrorTick = 0; 
	bool Error(ScriptErrorLevel_t eLevel, const char *text) {
		int errorsNumDisplay = 0;
		// Error is called per line, count multiple lines as one error
		if (lastErrorTick != gpGlobals->tickcount) {
			errorCounter++;
			if (lastNotifyTime + 5.0f < gpGlobals->curtime || gpGlobals->curtime < lastNotifyTime) {
				errorsNumDisplay = errorCounter;
				errorCounter = 0;
				lastNotifyTime = gpGlobals->curtime;
			}
		}
		ForEachPlayer([text, errorsNumDisplay](CBasePlayer *player){
			if (player->IsRealPlayer() && PlayerIsSMAdmin(player)) {
				if (errorsNumDisplay > 0) {
					PrintToChat(errorsNumDisplay > 1 ? CFmtStr("%d VScript errors had occured. Check console for details", errorsNumDisplay) : "A VScript error had occured. Check console for details", player);
				}
				ClientMsg(player, "%s", text);
			}
		});
		lastErrorTick = gpGlobals->tickcount;
		return true;
	};

	SpewOutputFunc_t s_SpewOutputBackup = nullptr;

	SpewRetval_t PrintToAdmins(SpewType_t type, const char *pMsg)
	{
		if (StringStartsWith(pMsg, "SCRIPT PERF WARNING")) {
			ForEachPlayer([&](CBasePlayer *player){
				if (player->IsRealPlayer() && PlayerIsSMAdmin(player)) {
					ClientMsg(player, "%s", pMsg);
				}
			});
		}
		return s_SpewOutputBackup(type, pMsg);
	}

    bool profile[MAX_PLAYERS + 1] = {};

	
	RefCount rc_test;
	double script_exec_time = 0;
	double script_exec_time_tick = 0;
    DETOUR_DECL_MEMBER(int, SQVM_Call, void *param1, intptr_t param2, intptr_t param3, void *param4, size_t param5)
	{
		SCOPED_INCREMENT(rc_test);
        CFastTimer timer;
		if (rc_test == 1) {
			s_SpewOutputBackup = GetSpewOutputFunc();
			SpewOutputFunc(&PrintToAdmins);
       		timer.Start();
		}
        auto result = DETOUR_MEMBER_CALL(param1, param2, param3, param4, param5);

		if (rc_test == 1) {
			timer.End();
			script_exec_time += timer.GetDuration().GetSeconds();
			script_exec_time_tick += timer.GetDuration().GetSeconds();
			SpewOutputFunc(s_SpewOutputBackup);
		}
		return result;
    }
    
    ModCommandDebug sig_vscript_prof_start("sig_vscript_prof_start", [](CCommandPlayer *player, const CCommand& args){
        profile[ENTINDEX(player)] = 1;
    });

    ModCommandDebug sig_vscript_prof_end("sig_vscript_prof_end", [](CCommandPlayer *player, const CCommand& args){
        profile[ENTINDEX(player)] = 0;
    });

    class CMod : public IMod, IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:VScript_Send_Output_To_Admins")
		{
			MOD_ADD_DETOUR_MEMBER(SQVM_Call, "SQVM::Call");
		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static double script_exec_time_tick_max = 0.0;
			static int ticks_per_second = (int)(1 / gpGlobals->interval_per_tick);
            if (script_exec_time_tick > script_exec_time_tick_max ) {
                script_exec_time_tick_max = script_exec_time_tick;
            }
            script_exec_time_tick = 0.0;
            if (gpGlobals->tickcount % ticks_per_second == 0) {
                char output[256];
                snprintf(output, sizeof(output), "VScript execution time: [avg: %.9fs (%d%%)| max: %.9fs (%d%%)]\n", script_exec_time * gpGlobals->interval_per_tick, (int)(script_exec_time * 100), script_exec_time_tick_max, (int)((script_exec_time_tick_max / gpGlobals->interval_per_tick) * 100));
                for (int i = 1; i < MAX_PLAYERS + 1; i++) {
                    if (profile[i]) {
                        auto player = UTIL_PlayerByIndex(i);
                        if (player != nullptr) {
                            ClientMsg(player, "%s", output);
                        }
                    }
                }
                if (profile[0]) {
                    Msg("%s", output);
                }
                script_exec_time = 0;
                script_exec_time_tick_max = 0;
            }

			if (errorCounter == 0) return;

			if (gpGlobals->curtime >= lastNotifyTime && lastNotifyTime + 5.0f > gpGlobals->curtime) return;

			ForEachPlayer([](CBasePlayer *player){
				if (PlayerIsSMAdmin(player)) {
					PrintToChat(errorCounter > 1 ? CFmtStr("%d VScript errors had occured. Check console for details", errorCounter) : "A VScript error had occured. Check console for details", player);
				}
			});
			lastNotifyTime = gpGlobals->curtime;
			errorCounter = 0;
		}

		virtual void LevelInitPreEntity() override {
			if (g_pScriptVM != nullptr) {
				g_pScriptVM->SetOutputCallback(&Output);
				g_pScriptVM->SetErrorCallback(&Error);
			}
		}

		virtual void OnEnable() override {
			if (g_pScriptVM != nullptr) {
				g_pScriptVM->SetOutputCallback(&Output);
				g_pScriptVM->SetErrorCallback(&Error);
			}
		}

		virtual void OnDisable() override {
			if (g_pScriptVM != nullptr) {
				g_pScriptVM->SetOutputCallback(nullptr);
				g_pScriptVM->SetErrorCallback(nullptr);
			}
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_util_vscript_send_output_to_admins", "0", FCVAR_NOTIFY,
		"Utility: Send output and errors from vscript to admins",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}