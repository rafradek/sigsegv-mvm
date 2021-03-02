#include "mod.h"
#include "util/rtti.h"
#include "stub/nav.h"
#include "mem/protect.h"


namespace Mod::Etc::Nav_UpdateBlocked
{
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Etc:Nav_UpdateBlocked") {}
		
		virtual bool OnLoad() override
		{
			this->vt_CNavArea = RTTI::GetVTable<CNavArea>();
			if (this->vt_CNavArea == nullptr) return false;
			
			this->vt_CTFNavArea = RTTI::GetVTable<CTFNavArea>();
			if (this->vt_CTFNavArea == nullptr) return false;
			
		#if 0
			DevMsg("vtable for CNavArea:\n");
			for (int i = 0; i < 0x20; ++i) {
				DevMsg("+%02x: 0x%08x\n", i * 4, (uintptr_t)vt_CNavArea[i]);
			}
			DevMsg("vtable for CTFNavArea:\n");
			for (int i = 0; i < 0x20; ++i) {
				DevMsg("+%02x: 0x%08x\n", i * 4, (uintptr_t)vt_CTFNavArea[i]);
			}
			
			ptr_CNavArea_UpdateBlocked   = vt_CNavArea  [VTIDX_UpdateBlocked];
			ptr_CTFNavArea_UpdateBlocked = vt_CTFNavArea[VTIDX_UpdateBlocked];
			
			DevMsg("CNavArea::UpdateBlocked:   0x%08x\n", (uintptr_t)ptr_CNavArea_UpdateBlocked);
			DevMsg("CTFNavArea::UpdateBlocked: 0x%08x\n", (uintptr_t)ptr_CTFNavArea_UpdateBlocked);
		#endif
			
			return true;
		}
		
		virtual void OnEnable() override
		{
			MemProtModifier_RO_RW(vt_CTFNavArea, 0x1000);
			vt_CTFNavArea[VTIDX_UpdateBlocked] = ptr_CNavArea_UpdateBlocked;
		}
		virtual void OnDisable() override
		{
			MemProtModifier_RO_RW(vt_CTFNavArea, 0x1000);
			vt_CTFNavArea[VTIDX_UpdateBlocked] = ptr_CTFNavArea_UpdateBlocked;
		}
		
	private:
		/* probably only valid for Linux */
		static constexpr int VTIDX_UpdateBlocked = 0x11;
		
		const void **vt_CNavArea   = nullptr;
		const void **vt_CTFNavArea = nullptr;
		
		const void *ptr_CNavArea_UpdateBlocked   = nullptr;
		const void *ptr_CTFNavArea_UpdateBlocked = nullptr;
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_nav_updateblocked", "0", FCVAR_NOTIFY,
		"Etc: make CTFNavArea::UpdateBlocked chain to CNavArea::UpdateBlocked instead of doing NOTHING",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}


namespace Mod::Etc::Nav_UpdateBlocked_Visualize
{
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Etc:Nav_UpdateBlocked_Visualize") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			static long frame = 0;
			if (frame++ % 7 != 0) return;
			
			NDebugOverlay::Clear();
			
			for (auto area : (CUtlVector<CTFNavArea *>&)TheNavAreas) {
				if (area->IsBlocked(TEAM_ANY)) {
					area->DrawFilled(0xff, 0x00, 0x00, 0x80, 1.00f, true, 3.0f);
					NDebugOverlay::EntityTextAtPosition(area->GetCenter(), 0, "BLOCKED", 0xff, 0xff, 0xff, 0xff, 1.00f);
				} else {
					area->DrawFilled(0xff, 0xff, 0xff, 0x00, 1.00f, true, 3.0f);
				}
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_etc_nav_updateblocked_visualize", "0", FCVAR_NOTIFY,
		"Etc: draw blocked and non-blocked nav areas",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
