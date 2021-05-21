#include "mod.h"
#include "stub/tf_shareddefs.h"
#include "stub/tfweaponbase.h"
#include "stub/tfplayer.h"
#include "util/color.h"


namespace Mod::Visualize::Ammo_Counts
{
	void ScreenText(int group, int line, float duration, const char *str, const Color32& color)
	{
		float x = 0.15f;
		float y = 0.35f + (group * 0.15f) + (line * 0.03f);
		
		NDebugOverlay::ScreenText(x, y, str, C32_EXPAND(color), duration);
	}
	
	
	constexpr Color32 cWhite  (0xff, 0xff, 0xff, 0xff);
	constexpr Color32 cBlack  (0x00, 0x00, 0x00, 0xff);
	constexpr Color32 cRed    (0xff, 0x00, 0x00, 0xff);
	constexpr Color32 cOrange (0xff, 0x80, 0x00, 0xff);
	constexpr Color32 cYellow (0xff, 0xff, 0x00, 0xff);
	constexpr Color32 cGreen  (0x00, 0xff, 0x00, 0xff);
	constexpr Color32 cCyan   (0x00, 0xff, 0xff, 0xff);
	constexpr Color32 cBlue   (0x00, 0x00, 0xff, 0xff);
	constexpr Color32 cViolet (0x80, 0x00, 0xff, 0xff);
	constexpr Color32 cMagenta(0xff, 0x00, 0xff, 0xff);
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Ammo_Counts") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 != 0) return;
			
			float duration = 0.032f;
			
			CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(1));
			if (player == nullptr) {
				Warning("UTIL_PlayerByIndex(1) returned nullptr\n");
				return;
			}
			
			auto pri = dynamic_cast<CTFWeaponBase *>(player->Weapon_GetSlot(0));
			if (pri != nullptr) {
				int iClip    = pri->m_iClip1;
				int iClipMax = pri->GetMaxClip1();
				
				int iReserve    = player->GetAmmoCount(TF_AMMO_PRIMARY);
				int iReserveMax = player->GetMaxAmmo  (TF_AMMO_PRIMARY);
				
				ScreenText(0, 0, duration, "PRIMARY AMMO", cWhite);
				
				ScreenText(0, 1, duration, CFmtStr("CLIP:      /  "           ), cWhite);
				ScreenText(0, 1, duration, CFmtStr("         %2d  ",  iClip   ), Color32(cRed, cYellow, cGreen, iClip, 0.0f, (float)iClipMax * 0.5f, (float)iClipMax));
				ScreenText(0, 1, duration, CFmtStr("            %2d", iClipMax), Color32(cRed, cYellow, cGreen, iClip, 0.0f, (float)iClipMax * 0.5f, (float)iClipMax));
				
				ScreenText(0, 2, duration, CFmtStr("RESERVE:   /  "              ), cWhite);
				ScreenText(0, 2, duration, CFmtStr("         %2d  ",  iReserve   ), Color32(cRed, cYellow, cGreen, iReserve, 0.0f, (float)iReserveMax * 0.5f, (float)iReserveMax));
				ScreenText(0, 2, duration, CFmtStr("            %2d", iReserveMax), Color32(cRed, cYellow, cGreen, iReserve, 0.0f, (float)iReserveMax * 0.5f, (float)iReserveMax));
			} else {
				ScreenText(0, 0, duration, "PRIMARY AMMO", cWhite);
				
				ScreenText(0, 1, duration, "ERROR!",       cRed);
			}
			
			auto sec = dynamic_cast<CTFWeaponBase *>(player->Weapon_GetSlot(1));
			if (sec != nullptr) {
				int iClip    = sec->m_iClip1;
				int iClipMax = sec->GetMaxClip1();
				
				int iReserve    = player->GetAmmoCount(TF_AMMO_SECONDARY);
				int iReserveMax = player->GetMaxAmmo  (TF_AMMO_SECONDARY);
				
				ScreenText(1, 0, duration, "SECONDARY AMMO", cWhite);
				
				ScreenText(1, 1, duration, CFmtStr("CLIP:      /  "           ), cWhite);
				ScreenText(1, 1, duration, CFmtStr("         %2d  ",  iClip   ), Color32(cRed, cYellow, cGreen, iClip, 0.0f, (float)iClipMax * 0.5f, (float)iClipMax));
				ScreenText(1, 1, duration, CFmtStr("            %2d", iClipMax), Color32(cRed, cYellow, cGreen, iClip, 0.0f, (float)iClipMax * 0.5f, (float)iClipMax));
				
				ScreenText(1, 2, duration, CFmtStr("RESERVE:   /  "              ), cWhite);
				ScreenText(1, 2, duration, CFmtStr("         %2d  ",  iReserve   ), Color32(cRed, cYellow, cGreen, iReserve, 0.0f, (float)iReserveMax * 0.5f, (float)iReserveMax));
				ScreenText(1, 2, duration, CFmtStr("            %2d", iReserveMax), Color32(cRed, cYellow, cGreen, iReserve, 0.0f, (float)iReserveMax * 0.5f, (float)iReserveMax));
			} else {
				ScreenText(1, 0, duration, "SECONDARY AMMO", cWhite);
				
				ScreenText(1, 1, duration, "ERROR!",         cRed);
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_ammo_counts", "0", FCVAR_NOTIFY,
		"Visualization: draw ammo count numbers for all weapons",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
