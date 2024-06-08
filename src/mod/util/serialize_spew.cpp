#include "mod.h"
#include "util/scope.h"


#if 0


namespace Mod::Util::Serialize_Spew
{
	FILE *f = nullptr;
	
	
	// block SpewOutputFunc(x) calls from:
	// - CShaderSystem::PrepForShaderDraw
	// - CShaderSystem::DoneWithShaderDraw
	
	// or, block based on whether x is MySpewOutputFunc
	
	
	DETOUR_DECL_STATIC(void, SpewOutputFunc, SpewOutputFunc_t func)
	{
		if (f != nullptr) {
			// report the following info to fprintf(f, ...):
			// thread ID
			// func ptr addr
			// LibMgr::WhichLibAtAddr(func ptr addr), if any
		}
		
		DETOUR_STATIC_CALL(func);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Util:Serialize_Spew")
		{
			#error need to setup SpewOutputFunc detour (can we use address-of operator on the name?)
		}
		
		virtual bool OnLoad() override
		{
			f = fopen("spewlog2.txt", "w");
			return (f != nullptr);
		}
		
		virtual void OnUnload() override
		{
			fclose(f);
			f = nullptr;
		}
	};
	CMod s_Mod;
	
	
	// method 1: carefully set up mutexes etc so that we don't attempt to add
	// new spew from the main thread while CShaderSystem::PrintBufferedSpew is
	// still in progress
	
	// method 2: just disable the damn buffered spew thing altogether and rely
	// on the the Sys_SpewFunc suppression system based on the thread-local bool
	// g_bInSpew
	
	// I like method 2 better
	
	
	ConVar cvar_enable("sig_util_serialize_spew", "0", FCVAR_NOTIFY,
		"Utility: fix spew ordering problems caused by CShaderSystem's spew buffering mechanism",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

#endif
