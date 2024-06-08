#include "mod.h"
#include "stub/tfplayer.h"
#include "util/backtrace.h"


namespace Mod::Debug::Heal_Debuff
{
	ConVar cvar_trace("sig_debug_heal_debuff_trace", "0", FCVAR_NOTIFY, "Debug: Print backtrace when InCond(TF_COND_HEALING_DEBUFF) is called");
	
	DETOUR_DECL_MEMBER(bool, CTFPlayerShared_InCond, ETFCond cond)
	{
		if (cvar_trace.GetBool() && cond == TF_COND_HEALING_DEBUFF) {
			BACKTRACE();
		}
		
		return DETOUR_MEMBER_CALL(cond);
	}
	
	
	ConVar cvar_critheal("sig_debug_heal_debuff_critheal", "0", FCVAR_NONE, "Debug: Enable crit heals?");
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Heal_Debuff")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_InCond, "CTFPlayerShared::InCond");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(1));
			if (player == nullptr) return;
			
			if (cvar_critheal.GetBool()) {
				player->m_flMvMLastDamageTime = gpGlobals->curtime - 30.0f;
			} else {
				player->m_flMvMLastDamageTime = gpGlobals->curtime;
			}
			
			player->RemoveCustomAttribute("dmg taken increased");
			player->AddCustomAttribute("dmg taken increased", 0.0f, 3600.0f);
			
			static int hp_last = 0;
			int hp_now = player->GetHealth();
			
			// keep a history of the last N heal amounts
			static std::deque<int> heal_hist;
			heal_hist.push_back(hp_now - hp_last);
			if (heal_hist.size() > 66) heal_hist.pop_front();
			
			hp_last = hp_now;
			
			// ensure we stay non-overhealed
			if (player->GetHealth() > (player->GetMaxHealth() - 25)) {
				hp_last -= (player->GetHealth() - 1);
				player->SetHealth(1);
			}
			
			static long frame = 0;
			if (++frame >= 6) {
				frame = 0;
				
				int heal_total = 0;
				for (auto heal : heal_hist) {
					heal_total += heal;
				}
				heal_total *= 66;
				float heal_avg = (float)heal_total / (float)heal_hist.size();
				
				NDebugOverlay::Clear();
				
				NDebugOverlay::ScreenText(0.04f, 0.05f, "InCond(TF_COND_HEALING_DEBUFF):", 0xff, 0xff, 0xff, 0xff, 3600.0f);
				NDebugOverlay::ScreenText(0.04f, 0.10f, "             Average Heal Rate:", 0xff, 0xff, 0xff, 0xff, 3600.0f);
				NDebugOverlay::ScreenText(0.04f, 0.15f, "                    Crit Heals:", 0xff, 0xff, 0xff, 0xff, 3600.0f);
				
				Color c; const char *str;
				if (player->m_Shared->InCond(TF_COND_HEALING_DEBUFF)) {
					c = Color(0xff, 0x00, 0x00, 0xff);
					str = "YES";
				} else {
					c = Color(0x00, 0xff, 0x00, 0xff);
					str = "NO";
				}
				NDebugOverlay::ScreenText(0.42f, 0.05f, str, c[0], c[1], c[2], c[3], 3600.0f);
				
				NDebugOverlay::ScreenText(0.42f, 0.10f, CFmtStr("%.1f HP/sec", heal_avg), 0xff, 0xff, 0xff, 0xff, 3600.0f);
				
				if (cvar_critheal.GetBool()) {
					c = Color(0x00, 0xff, 0x00, 0xff);
					str = "ENABLED";
				} else {
					c = Color(0xff, 0x00, 0x00, 0xff);
					str = "DISABLED";
				}
				NDebugOverlay::ScreenText(0.42f, 0.15f, str, c[0], c[1], c[2], c[3], 3600.0f);
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_heal_debuff", "0", FCVAR_NOTIFY,
		"Debug: figure out why TF_COND_HEALING_DEBUFF is so screwed up",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
