#include "mod.h"


namespace Mod::Debug::Console_Scramble_v2
{
	FILE *f = nullptr;
	
	
	DETOUR_DECL_STATIC(SpewRetval_t, Sys_SpewFunc, SpewType_t spewType, const char *pMsg)
	{
		if (f != nullptr) {
			fprintf(f, "%04X   >> Sys_SpewFunc [%12.7f] [Tick %5d] [SpewFunc %08x] \"%s\"\n", ThreadGetCurrentId(), Plat_FloatTime(), gpGlobals->tickcount, (uintptr_t)GetSpewOutputFunc(), pMsg);
			fflush(f);
		}
		
		auto result = DETOUR_STATIC_CALL(spewType, pMsg);
		
		if (f != nullptr) {
			fprintf(f, "%04X   << Sys_SpewFunc [%12.7f] [Tick %5d] [SpewFunc %08x] \"%s\"\n", ThreadGetCurrentId(), Plat_FloatTime(), gpGlobals->tickcount, (uintptr_t)GetSpewOutputFunc(), pMsg);
			fflush(f);
		}
		
		return result;
	}
	
	
	DETOUR_DECL_STATIC(void, ConColorMsg, const Color& clr, const char *pMsgFormat, ...)
	{
	//	auto buf = new char[0x1000];
		char buf[0x1000];
		
		va_list va;
		va_start(va, pMsgFormat);
		V_vsnprintf(buf, 0x1000, pMsgFormat, va);
		va_end(va);
		
		if (f != nullptr) {
			fprintf(f, "%04X >>   ConColorMsg  [%12.7f] [Tick %5d] [SpewFunc %08x] \"%s\"\n", ThreadGetCurrentId(), Plat_FloatTime(), gpGlobals->tickcount, (uintptr_t)GetSpewOutputFunc(), buf);
			fflush(f);
		}
		
		DETOUR_STATIC_CALL(clr, "%s", buf);
		
		if (f != nullptr) {
			fprintf(f, "%04X <<   ConColorMsg  [%12.7f] [Tick %5d] [SpewFunc %08x] \"%s\"\n", ThreadGetCurrentId(), Plat_FloatTime(), gpGlobals->tickcount, (uintptr_t)GetSpewOutputFunc(), buf);
			fflush(f);
		}
		
	//	delete buf;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Debug:Console_Scramble_v2")
		{
		//	this->AddDetour(new CDetour("Sys_SpewFunc", reinterpret_cast<void *>(GetSpewOutputFunc()), GET_STATIC_CALLBACK(Sys_SpewFunc), GET_STATIC_INNERPTR(Sys_SpewFunc)));
		//	this->AddDetour(new CDetour("ConColorMsg", reinterpret_cast<void *>(static_cast<void (*)(const Color&, const char *, ...)>(&ConColorMsg)), GET_STATIC_CALLBACK(ConColorMsg), GET_STATIC_INNERPTR(ConColorMsg)));
		}
		
		virtual bool OnLoad() override
		{
			f = fopen("spewlog.txt", "w");
			return (f != nullptr);
		}
		
		virtual void OnUnload() override
		{
			fclose(f);
			f = nullptr;
		}
	};
	CMod s_Mod;
	
	ConVar cvar_enable("sig_debug_console_scramble_v2", "0", FCVAR_NOTIFY,
		"Debug: determine why mat_queue_mode 2 causes console output to go out of order",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CModA : public IMod
	{
	public:
		CModA() : IMod("Debug:Console_Scramble_v2_PartA")
		{
			this->AddDetour(new CDetour("Sys_SpewFunc", reinterpret_cast<void *>(GetSpewOutputFunc()), GET_STATIC_CALLBACK(Sys_SpewFunc), GET_STATIC_INNERPTR(Sys_SpewFunc)));
		}
	};
	CModA s_ModA;
	
	ConVar cvar_spewfunc("sig_debug_console_scramble_v2_spewfunc", "0", FCVAR_NOTIFY,
		"Debug: enable Sys_SpewFunc detour",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_ModA.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	class CModB : public IMod
	{
	public:
		CModB() : IMod("Debug:Console_Scramble_v2_PartB")
		{
			this->AddDetour(new CDetour("ConColorMsg", reinterpret_cast<void *>(static_cast<void (*)(const Color&, const char *, ...)>(&ConColorMsg)), GET_STATIC_CALLBACK(ConColorMsg), GET_STATIC_INNERPTR(ConColorMsg)));
		}
	};
	CModB s_ModB;
	
	ConVar cvar_concolormsg("sig_debug_console_scramble_v2_concolormsg", "0", FCVAR_NOTIFY,
		"Debug: enable ConColorMsg detour",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_ModB.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
