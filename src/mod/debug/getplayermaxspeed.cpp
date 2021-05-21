#include "mod.h"
#include "stub/tfplayer.h"


namespace Mod::Debug::GetPlayerMaxSpeed
{
	void DrawSpeedOverlay(float duration)
	{
	//	float dy = 0.03f;
		
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(i));
			if (player == nullptr)                               continue;
			if (strcmp(player->GetPlayerName(), "sigsegv") != 0) continue;
			
		//	float x = 0.10f;
		//	float y = 0.10f;
			
			float speed1 = player->TeamFortress_CalculateMaxSpeed();
			float speed2 = player->MaxSpeed();
			float speed3 = player->GetPlayerMaxSpeed();
			
		//	float delta = abs(speed1 - speed2);
		//	bool delta_ok = (delta < 0.1f);
			
			CFmtStrN<256> fmt1("TeamFortress_CalculateMaxSpeed: %6.1f HU/s", speed1);
			CFmtStrN<256> fmt2("                      MaxSpeed: %6.1f HU/s", speed2);
			CFmtStrN<256> fmt3("             GetPlayerMaxSpeed: %6.1f HU/s", speed3);
		//	CFmtStrN<256> fmt3("                    Difference: %6.1f HU/s", delta);
			
		//	int r = (delta_ok ? 0x00 : 0xff);
		//	int g = (delta_ok ? 0xff : 0x00);
		//	int b = 0x00;
			
			NDebugOverlay::ScreenText(0.60f, 0.05f, fmt1, 0xff, 0xff, 0xff, 0xff, duration);
			NDebugOverlay::ScreenText(0.60f, 0.08f, fmt2, 0xff, 0xff, 0xff, 0xff, duration);
			NDebugOverlay::ScreenText(0.60f, 0.11f, fmt3, 0xff, 0xff, 0xff, 0xff, duration);
		//	NDebugOverlay::ScreenText(0.60f, 0.12f, fmt3,    r,    g,    b, 0xff, duration);
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:GetPlayerMaxSpeed") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 == 0) return;
			
			DrawSpeedOverlay(0.030f);
			
		#if 0
			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(i));
				if (player == nullptr) continue;
				
				NDebugOverlay::EntityTextAtPosition(player->GetAbsOrigin() + Vector(0.0f, 0.0f, 80.0f), 0,
					player->GetPlayerName(), duration);
				NDebugOverlay::EntityTextAtPosition(player->GetAbsOrigin() + Vector(0.0f, 0.0f, 80.0f), 1,
					CFmtStrN<64>("TF_CMS: %6.1f", player->TeamFortress_CalculateMaxSpeed()), duration);
				NDebugOverlay::EntityTextAtPosition(player->GetAbsOrigin() + Vector(0.0f, 0.0f, 80.0f), 2,
					CFmtStrN<64>("GPMS:   %6.1f", player->GetPlayerMaxSpeed()), duration);
			}
		#endif
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_getplayermaxspeed", "0", FCVAR_NOTIFY,
		"Debug: show result of CBasePlayer::GetPlayerMaxSpeed() as an overlay",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
