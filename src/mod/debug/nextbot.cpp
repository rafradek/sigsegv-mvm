#include "mod.h"
#include "re/nextbot.h"
#include "stub/tfbot.h"


namespace Mod::Debug::NextBot
{
	constexpr float INTERVAL = 0.25f;
	
	
	void UpdateOverlays()
	{
		static Color white (0xff, 0xff, 0xff, 0xff);
		static Color green (0x80, 0xff, 0x80, 0xff);
		static Color yellow(0xff, 0xff, 0x80, 0xff);
		static Color red  (0xff, 0x80, 0x00, 0xff);
		
		for (int i = 1; i < gpGlobals->maxClients; ++i) {
			CTFBot *bot = ToTFBot(UTIL_PlayerByIndex(i));
			if (bot == nullptr) continue;
			
			auto nextbot = rtti_cast<INextBot *>(bot);
			if (nextbot == nullptr) continue;
			
			{
				Color& color = white;
				const char *text = "";
				
				switch (bot->GetIntentionInterface()->ShouldHurry(nextbot)) {
				case QueryResponse::NO:
					color = red;
					text  = "ShouldHurry: NO";
					break;
				case QueryResponse::YES:
					color = green;
					text  = "ShouldHurry: YES";
					break;
				case QueryResponse::DONTCARE:
					color = white;
					text = "ShouldHurry: DONTCARE";
					break;
				}
				
				NDebugOverlay::EntityText(i, 2, text, INTERVAL, color.r(), color.g(), color.b(), color.a());
			}
			
			{
				Color& color = white;
				const char *text = "";
				
				switch (bot->GetIntentionInterface()->ShouldRetreat(nextbot)) {
				case QueryResponse::NO:
					color = red;
					text  = "ShouldRetreat: NO";
					break;
				case QueryResponse::YES:
					color = green;
					text  = "ShouldRetreat: YES";
					break;
				case QueryResponse::DONTCARE:
					color = white;
					text = "ShouldRetreat: DONTCARE";
					break;
				}
				
				NDebugOverlay::EntityText(i, 3, text, INTERVAL, color.r(), color.g(), color.b(), color.a());
			}
			
			{
				CUtlVector<CKnownEntity> knowns;
				bot->GetVisionInterface()->CollectKnownEntities(&knowns);
				
				int n_total    = 0;
				int n_no       = 0;
				int n_yes      = 0;
				int n_dontcare = 0;
				
				for (const auto& known : knowns) {
					switch (bot->GetIntentionInterface()->ShouldAttack(nextbot, &known)) {
					case QueryResponse::NO:
						++n_no;
						break;
					case QueryResponse::YES:
						++n_yes;
						break;
					case QueryResponse::DONTCARE:
						++n_dontcare;
						break;
					}
					
					++n_total;
				}
				
				Color& color = white;
				const char *text = "";
				
				if (n_total == n_no) {
					color = red;
					text  = "ShouldAttack: NO";
				} else if (n_total == n_yes) {
					color = green;
					text  = "ShouldAttack: YES";
				} else if (n_total == n_dontcare) {
					color = white;
					text  = "ShouldAttack: DONTCARE";
				} else {
					color = yellow;
					text  = "ShouldAttack: <VARIES>";
				}
				
				NDebugOverlay::EntityText(i, 4, text, INTERVAL, color.r(), color.g(), color.b(), color.a());
			}
			
			// TODO: GetPrimaryKnownThreat (true and false)
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:NextBot") {}
		
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
	
	
	ConVar cvar_enable("sig_debug_nextbot", "0", FCVAR_NOTIFY,
		"Debug: NextBot",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
