#include "mod.h"
#include "stub/nav.h"


void ForEachArea(const std::function<void(CTFNavArea *)>& functor)
{
	CUtlVector<CTFNavArea *>& vec = TheNavAreas;
	
	FOR_EACH_VEC(vec, i) {
		functor(vec[i]);
	}
}


namespace Mod::Debug::Nav_Mesh
{
	constexpr float INTERVAL = 0.25f;
	
	
	ConVar cvar_mode("sig_debug_nav_mesh_mode", "", FCVAR_NOTIFY,
		"Debug: nav mesh debug mode(s): all, blocked, spawn");
	
	
	void UpdateOverlays()
	{
		const char *mode = cvar_mode.GetString();
		
		if (V_stristr(mode, "all") != nullptr) {
			ForEachArea([](CTFNavArea *area){
				area->DrawFilled(0xc0, 0xc0, 0xc0, 0x00, INTERVAL, true, 1.0f);
			});
		}
		
		if (V_stristr(mode, "blocked") != nullptr) {
			ForEachArea([](CTFNavArea *area){
				if (area->HasTFAttributes(BLOCKED)) {
					area->DrawFilled(0xff, 0xff, 0x00, 0x40, INTERVAL, true, 2.0f);
				}
			});
		}
		
		if (V_stristr(mode, "spawn") != nullptr) {
			ForEachArea([](CTFNavArea *area){
				if (area->HasTFAttributes(RED_SPAWN_ROOM)) {
					if (area->HasTFAttributes(SPAWN_ROOM_EXIT)) {
						area->DrawFilled(0xff, 0x80, 0x80, 0x40, INTERVAL, true, 2.0f);
					} else {
						area->DrawFilled(0xff, 0x00, 0x00, 0x40, INTERVAL, true, 2.0f);
					}
				}
				if (area->HasTFAttributes(BLUE_SPAWN_ROOM)) {
					if (area->HasTFAttributes(SPAWN_ROOM_EXIT)) {
						area->DrawFilled(0x80, 0x80, 0xff, 0x40, INTERVAL, true, 2.0f);
					} else {
						area->DrawFilled(0x00, 0x00, 0xff, 0x40, INTERVAL, true, 2.0f);
					}
				}
			});
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Nav_Mesh") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			if (this->m_ctUpdate.IsElapsed()) {
				this->m_ctUpdate.Start(INTERVAL);
				
				UpdateOverlays();
			}
		}
		
	private:
		CountdownTimer m_ctUpdate;
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_nav_mesh", "0", FCVAR_NOTIFY,
		"Debug: nav mesh stuff",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
