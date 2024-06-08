#include "mod.h"
#include "mod/pop/kv_conditional.h"


namespace Mod::Pop::KV_Conditional
{

	ConVar cvar_always_enable_conditional("sig_always_enable_conditional", "0", FCVAR_NOTIFY, "");

	bool IsSigsegv()
	{
		for (auto cond : AutoList<IKVCond>::List()) {
			if ((*cond)()) return true;
		}
		
		return cvar_always_enable_conditional.GetBool();
	}
	
	
	DETOUR_DECL_STATIC(bool, EvaluateConditional, const char *str)
	{
		bool result = DETOUR_STATIC_CALL(str);
		
		if (*str == '[') ++str;
		bool bNot = (*str == '!');
		
		if (V_stristr(str, "$SIGSEGV") != nullptr) {
			return IsSigsegv() ^ bNot;
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Pop:KV_Conditional")
		{
			MOD_ADD_DETOUR_STATIC(EvaluateConditional, "EvaluateConditional");
		}
		
		virtual bool EnableByDefault() override { return true; }
	};
	CMod s_Mod;
}
