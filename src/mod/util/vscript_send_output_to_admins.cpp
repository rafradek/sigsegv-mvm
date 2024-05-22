#include "mod.h"
#include "vscript/ivscript.h"
#include "mem/protect.h"
#include "link/link.h"
#include "util/admin.h"
#include "stub/baseplayer.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "stub/misc.h"

namespace Mod::Util::VScript_Send_Output_To_Admins
{

	float lastNotifyTime = 0;
	int errorCounter = 0;
	GlobalThunk<IScriptVM *> g_pScriptVM("g_pScriptVM"); 

	void Output(const char *text) {
		ForEachPlayer([text](CBasePlayer *player){
			if (PlayerIsSMAdmin(player)) {
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
			if (PlayerIsSMAdmin(player)) {
				if (errorsNumDisplay > 0) {
					PrintToChat(errorsNumDisplay > 1 ? CFmtStr("%d VScript errors had occured. Check console for details", errorsNumDisplay) : "A VScript error had occured. Check console for details", player);
				}
				ClientMsg(player, "%s", text);
			}
		});
		lastErrorTick = gpGlobals->tickcount;
		return true;
	};

    class CMod : public IMod, IModCallbackListener, IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Util:VScript_Send_Output_To_Admins")
		{

		}

		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
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