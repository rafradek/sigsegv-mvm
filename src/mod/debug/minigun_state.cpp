#include "mod.h"
#include "stub/tfweaponbase.h"
#include "util/iterate.h"


namespace Mod::Debug::Minigun_State
{
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Minigun_State") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 == 0) return;
			
			ForEachEntityByRTTI<CTFMinigun>([](CTFMinigun *minigun){
				NDebugOverlay::EntityText(ENTINDEX(minigun), 0, CFmtStr("%d", (int)minigun->m_iWeaponState), 0.030f);
				
		//		DevMsg("Minigun WSC: % 6.1f % 6.1f % 6.1f\n",
		//			minigun->WorldSpaceCenter().x,
		//			minigun->WorldSpaceCenter().y,
		//			minigun->WorldSpaceCenter().z);
			});
		//	ForEachEntityByClassname("player", [](CBaseEntity *player){
		//		DevMsg(" Player WSC: % 6.1f % 6.1f % 6.1f\n",
		//			player->WorldSpaceCenter().x,
		//			player->WorldSpaceCenter().y,
		//			player->WorldSpaceCenter().z);
		//	});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_minigun_state", "0", FCVAR_NOTIFY,
		"Debug: watch the value of CTFMinigun::m_iWeaponState",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
