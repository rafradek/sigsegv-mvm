#include "mod.h"
#include "vscript/ivscript.h"
#include "mem/protect.h"
#include "link/link.h"
#include "util/admin.h"
#include "stub/baseplayer.h"
#include "util/iterate.h"
#include "util/clientmsg.h"
#include "stub/misc.h"

namespace Mod::Util::VScript_Load_Init_Scripts
{

	GlobalThunk<IScriptVM *> g_pScriptVM("g_pScriptVM"); 

    RefCount rc_VScriptServerInit;
    DETOUR_DECL_STATIC(void , VScriptServerInit)
	{
        SCOPED_INCREMENT(rc_VScriptServerInit);
        DETOUR_STATIC_CALL();
    }
    
    DETOUR_DECL_STATIC(void *, VScriptRunScript, const char *name, void *caller, bool flag)
	{
        if (rc_VScriptServerInit && name != nullptr && FStrEq(name, "mapspawn") && filesystem->FileExists("scripts/vscripts/serverinit.nut", "mod")) {
			CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER);
			if (filesystem->ReadFile("scripts/vscripts/serverinit.nut", "mod", buf)) {
				auto script = g_pScriptVM->CompileScript(buf.String(), "serverinit.nut");
				g_pScriptVM->Run(script, false);
			}
			
            //DETOUR_STATIC_CALL("serverinit", caller, flag);

        }
		auto result = DETOUR_STATIC_CALL(name, caller, flag);

		return result;
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:VScript_Load_Init_Scripts")
		{
            MOD_ADD_DETOUR_STATIC(VScriptServerInit, "VScriptServerInit");
            MOD_ADD_DETOUR_STATIC(VScriptRunScript, "VScriptRunScript");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_util_vscript_load_init_scripts", "0", FCVAR_NOTIFY,
		"Utility: Load vscript serverinit.nut before mapspawn",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}