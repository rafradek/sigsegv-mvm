#include "mod.h"
#include "stub/baseentity.h"
#include "stub/projectiles.h"
#include "stub/objects.h"
#include "util/backtrace.h"
#include "tier1/CommandBuffer.h"


namespace Mod::Etc::Server_Allow_Wait_Command
{
	DETOUR_DECL_MEMBER(bool, CCommandBuffer_AddText, const char *pText, int nTickDelay )
	{
		reinterpret_cast<CCommandBuffer *>(this)->SetWaitEnabled(true);
		return DETOUR_MEMBER_CALL(pText, nTickDelay);
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Misc")
		{
			MOD_ADD_DETOUR_MEMBER(CCommandBuffer_AddText,    "CCommandBuffer::AddText");
		}
	};
	CMod s_Mod;

    ConVar cvar_enable("sig_etc_allow_wait_command", "0", FCVAR_NOTIFY,
		"Mod: always allow wait command on server",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}