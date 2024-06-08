#include "mod.h"


namespace Mod::Debug::MultiDetour
{
	int Function()
	{
		DevMsg("Function\n");
		return 10;
	}
	
	
	DETOUR_DECL_STATIC(int, D1)
	{
		DevMsg("Detour1 PRE\n");
	//	DevMsg("Trampoline: %08x\n", (uintptr_t)DETOUR_STATIC_CALL);
		auto result = DETOUR_STATIC_CALL();
		DevMsg("Detour1 POST (%d)\n", result);
		return result;
	}
	
	DETOUR_DECL_STATIC(int, D2)
	{
		DevMsg("Detour2 PRE\n");
	//	DevMsg("Trampoline: %08x\n", (uintptr_t)DETOUR_STATIC_CALL);
		auto result = DETOUR_STATIC_CALL();
		DevMsg("Detour2 POST (%d)\n", result);
		return result;
	}
	
	DETOUR_DECL_STATIC(int, D3)
	{
		DevMsg("Detour3 PRE\n");
	//	DevMsg("Trampoline: %08x\n", (uintptr_t)DETOUR_STATIC_CALL);
		auto result = DETOUR_STATIC_CALL();
		DevMsg("Detour3 POST (%d)\n", result);
		return result;
	}
	
	
	// Things we'd like to know:
	// 1. does recursive detouring even work?
	// 2. what happens if one of the detours doesn't call through to the function?
	// 3. does disabling the detours in LIFO order work properly?
	// 4. does disabling the detours in non-LIFO order screw everything up?
	
	
	#define EnableDetour  Enable
	#define DisableDetour Disable
	
	
	CON_COMMAND(sig_debug_multidetour_lifo, "LIFO test")
	{
		DevMsg("\nLIFO test\n");
		
//		CDetour *d1 = CDetourManager::CreateDetour(GET_STATIC_CALLBACK(D1), GET_STATIC_TRAMPOLINE(D1), (void *)&Function);
//		CDetour *d2 = CDetourManager::CreateDetour(GET_STATIC_CALLBACK(D2), GET_STATIC_TRAMPOLINE(D2), (void *)&Function);
//		CDetour *d3 = CDetourManager::CreateDetour(GET_STATIC_CALLBACK(D3), GET_STATIC_TRAMPOLINE(D3), (void *)&Function);
		CDetour *d1 = new CDetour("d1", reinterpret_cast<void *>(&Function), GET_STATIC_CALLBACK(D1), GET_STATIC_INNERPTR(D1));
		CDetour *d2 = new CDetour("d2", reinterpret_cast<void *>(&Function), GET_STATIC_CALLBACK(D2), GET_STATIC_INNERPTR(D2));
		CDetour *d3 = new CDetour("d3", reinterpret_cast<void *>(&Function), GET_STATIC_CALLBACK(D3), GET_STATIC_INNERPTR(D3));
		d1->Load();
		d2->Load();
		d3->Load();
		
		
		DevMsg("\n[Undetoured]\n");
		Function();
		
		
		d1->EnableDetour();
		DevMsg("\n[+Detour1]\n");
		Function();
		
		d2->EnableDetour();
		DevMsg("\n[+Detour2]\n");
		Function();
		
		d3->EnableDetour();
		DevMsg("\n[+Detour3]\n");
		Function();
		
		
		d3->DisableDetour();
		DevMsg("\n[-Detour3]\n");
		Function();
		
		d2->DisableDetour();
		DevMsg("\n[-Detour2]\n");
		Function();
		
		d1->DisableDetour();
		DevMsg("\n[-Detour1]\n");
		Function();
		
		
//		d1->Destroy();
//		d2->Destroy();
//		d3->Destroy();
		delete d1;
		delete d2;
		delete d3;
	}
	
	
	CON_COMMAND(sig_debug_multidetour_fifo, "FIFO test")
	{
		DevMsg("\nFIFO test\n");
		
//		CDetour *d1 = CDetourManager::CreateDetour(GET_STATIC_CALLBACK(D1), GET_STATIC_TRAMPOLINE(D1), (void *)&Function);
//		CDetour *d2 = CDetourManager::CreateDetour(GET_STATIC_CALLBACK(D2), GET_STATIC_TRAMPOLINE(D2), (void *)&Function);
//		CDetour *d3 = CDetourManager::CreateDetour(GET_STATIC_CALLBACK(D3), GET_STATIC_TRAMPOLINE(D3), (void *)&Function);
		CDetour *d1 = new CDetour("d1", reinterpret_cast<void *>(&Function), GET_STATIC_CALLBACK(D1), GET_STATIC_INNERPTR(D1));
		CDetour *d2 = new CDetour("d2", reinterpret_cast<void *>(&Function), GET_STATIC_CALLBACK(D2), GET_STATIC_INNERPTR(D2));
		CDetour *d3 = new CDetour("d3", reinterpret_cast<void *>(&Function), GET_STATIC_CALLBACK(D3), GET_STATIC_INNERPTR(D3));
		d1->Load();
		d2->Load();
		d3->Load();
		
		
		
		DevMsg("\n[Undetoured]\n");
		Function();
		
		
		d1->EnableDetour();
		DevMsg("\n[+Detour1]\n");
		Function();
		
		d2->EnableDetour();
		DevMsg("\n[+Detour2]\n");
		Function();
		
		d3->EnableDetour();
		DevMsg("\n[+Detour3]\n");
		Function();
		
		
		d1->DisableDetour();
		DevMsg("\n[-Detour1]\n");
		Function();
		
		d2->DisableDetour();
		DevMsg("\n[-Detour2]\n");
		Function();
		
		d3->DisableDetour();
		DevMsg("\n[-Detour3]\n");
		Function();
		
		
//		d1->Destroy();
//		d2->Destroy();
//		d3->Destroy();
		delete d1;
		delete d2;
		delete d3;
	}
}
