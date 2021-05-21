#include "mod.h"
#include "stub/entities.h"
#include "stub/tfplayer.h"
#include "stub/populators.h"
#include "util/iterate.h"


namespace Mod::Debug::Credit_Magnet
{
	// show all members for each currency pack
	// show pull info for each currency pack (lines connecting to players pulling on it)
	// draw vector arrows for impulse being applied
	
	
	/* correct as of 20160819a */
	constexpr ptrdiff_t OFF_CCURRENCYPACK = 0x4f8;
	struct CCurrencyPackPartial
	{
		int m_nAmount;
		CWave *m_pWave;
		int m_iBlink;
		CountdownTimer m_ctUnknown;
		bool m_bTouched;
		bool m_bPulled;
		bool m_bDistributed;
	};
	
	
	struct pulledcurrencypacks_t
	{
		CHandle<CCurrencyPack> hPack;
		float flTime;
	};
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Credit_Magnet") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 == 0) return;
			
			for (int i = 0; i < ICurrencyPackAutoList::AutoList().Count(); ++i) {
				auto pack = rtti_cast<CCurrencyPack *>(ICurrencyPackAutoList::AutoList()[i]);
				if (pack == nullptr) continue;
				
				auto partial = reinterpret_cast<CCurrencyPackPartial *>((uintptr_t)pack + OFF_CCURRENCYPACK);
				
				NDebugOverlay::EntityText(ENTINDEX(pack), 0, CFmtStrN<256>("$%d", partial->m_nAmount),
					0.030f, 0xff, 0xff, 0xff, 0xff);
				// TODO: wave
				NDebugOverlay::EntityText(ENTINDEX(pack), 1, CFmtStrN<256>("Blink %d", partial->m_iBlink),
					0.030f, 0xff, 0xff, 0xff, 0xff);
				// TODO: countdown timer
				NDebugOverlay::EntityText(ENTINDEX(pack), 2, CFmtStrN<256>("Touched: %s", (partial->m_bTouched ? "YES" : "NO")),
					0.030f, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityText(ENTINDEX(pack), 3, CFmtStrN<256>("Pulled: %s", (partial->m_bPulled ? "YES" : "NO")),
					0.030f, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityText(ENTINDEX(pack), 4, CFmtStrN<256>("Distributed: %s", (partial->m_bDistributed ? "YES" : "NO")),
					0.030f, 0xff, 0xff, 0xff, 0xff);
			}
			
		//	ForEachTFPlayer([](CTFPlayer *player){
		//		
		//	});
			
			// 
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_credit_magnet", "0", FCVAR_NOTIFY,
		"Debug: investigate how credit pack magnet pull works",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
