#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/objects.h"
#include "stub/tf_shareddefs.h"
#include "stub/econ.h"
#include "stub/tfbot.h"
#include "util/iterate.h"


namespace Mod::Debug::Building_Status
{
	ConVar cvar_xpos("sig_debug_building_status_xpos", "10", FCVAR_NOTIFY);
	ConVar cvar_ypos("sig_debug_building_status_ypos", "10", FCVAR_NOTIFY);
	
	ConVar cvar_lineheight("sig_debug_building_status_lineheight",  "45", FCVAR_NOTIFY);
	ConVar cvar_spacing   ("sig_debug_building_status_spacing",    "120", FCVAR_NOTIFY);
	
	ConVar cvar_barwidth("sig_debug_building_status_barwidth", "22", FCVAR_NOTIFY);
	ConVar cvar_barchar ("sig_debug_building_status_barchar",   "+", FCVAR_NOTIFY);
	
	
	const Color white (0xff, 0xff, 0xff, 0xff);
	const Color grayA0(0xa0, 0xa0, 0xa0, 0xff);
	const Color gray80(0x80, 0x80, 0x80, 0xff);
	const Color gray40(0x40, 0x40, 0x40, 0xff);
	const Color red   (0xff, 0x00, 0x00, 0xff);
	const Color yellow(0xff, 0xff, 0x00, 0xff);
	const Color green (0x00, 0xff, 0x00, 0xff);
	
	
	void ScreenText(int x, int y, int line, const char *text, const Color& color)
	{
		y += line * cvar_lineheight.GetInt();
		
		float fx = (float)x / 1920.0f;
		float fy = (float)y / 1080.0f;
		
		NDebugOverlay::ScreenText(fx, fy, text, color[0], color[1], color[2], color[3], 1.0f);
	}
	
	
	Color FracColor(float frac)
	{
		static const Color& color_zero = red;
		static const Color& color_half = yellow;
		static const Color& color_full = green;
		
		auto l = [=](int i){
			if (frac < 0.50f) {
				return RemapValClamped(frac, 0.00f, 0.50f, color_zero[i], color_half[i]);
			} else {
				return RemapValClamped(frac, 0.50f, 1.00f, color_half[i], color_full[i]);
			}
		};
		
		return Color(l(0), l(1), l(2), l(3));
	}
	
	
	void DrawHPBarInternal(int x, int y, int line, int lvl, int hp, int hp_max)
	{
		constexpr float width_factor[] = { 0, 1.00f, 1.20f, 1.44f };
		
		int width = (int)((float)cvar_barwidth.GetInt() * width_factor[lvl]);
		const char *c = cvar_barchar.GetString();
		
		float hp_ratio = (float)hp / (float)hp_max;
		
		for (int i = 0; i < width; ++i) {
			float frac = ((float)i + 0.5f) / (float)width;
			
			if (frac >= hp_ratio) {
				ScreenText(x, y, line, CFmtStr("%*s%s", i, "", c), gray40);
			} else {
				ScreenText(x, y, line, CFmtStr("%*s%s", i, "", c), FracColor(frac));
			}
		}
	}
	
	
	void DrawHPBar(int x, int y, int line, CTFPlayer *player, CBaseObject *obj)
	{
		int hp_max_lookup[] = { 0, 150, 180, 216 };
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, hp_max_lookup[1], mult_engy_building_health);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, hp_max_lookup[2], mult_engy_building_health);
		CALL_ATTRIB_HOOK_INT_ON_OTHER(player, hp_max_lookup[3], mult_engy_building_health);
		
		int lvl, hp, hp_max;
		
		if (obj == nullptr) {
			lvl    = 1;
			hp     = 0;
			hp_max = hp_max_lookup[1];
		} else {
			lvl    = obj->m_iUpgradeLevel;
			hp     = obj->GetHealth();
			hp_max = hp_max_lookup[lvl];
		}
		
		DrawHPBarInternal(x, y, line, lvl, hp, hp_max);
	}
	
	
	/* make bots only able to see certain types of buildings based on keywords in their player name */
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		CTFBot *bot = ToTFBot(vision->GetBot()->GetEntity());
		
		auto obj = rtti_cast<CBaseObject *>(ent);
		
		if (bot != nullptr && obj != nullptr) {
			if (V_stristr(bot->GetPlayerName(), "sentry") != nullptr && obj->GetType() == OBJ_SENTRYGUN) {
				return false;
			}
			if (V_stristr(bot->GetPlayerName(), "dispenser") != nullptr && obj->GetType() == OBJ_DISPENSER) {
				return false;
			}
			if (V_stristr(bot->GetPlayerName(), "tele1") != nullptr && obj->GetType() == OBJ_TELEPORTER && obj->GetObjectMode() == 0) {
				return false;
			}
			if (V_stristr(bot->GetPlayerName(), "tele2") != nullptr && obj->GetType() == OBJ_TELEPORTER && obj->GetObjectMode() == 1) {
				return false;
			}
			
			return true;
		}
		
		if (ent->IsPlayer()) {
			return true;
		}
		
		return DETOUR_MEMBER_CALL(CTFBotVision_IsIgnored)(ent);
	}
	
	
	/* make TFBots actually notice teleporters in MvM mode */
	DETOUR_DECL_MEMBER(void, CTFBotVision_CollectPotentiallyVisibleEntities, CUtlVector<CBaseEntity *> *ents)
	{
		DETOUR_MEMBER_CALL(CTFBotVision_CollectPotentiallyVisibleEntities)(ents);
		
		ForEachEntity([&](CBaseEntity *ent){
			auto tele = rtti_cast<CObjectTeleporter *>(ent);
			if (tele == nullptr) return;
			
			ents->AddToTail(tele);
		});
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Building_Status")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored");
			
			MOD_ADD_DETOUR_MEMBER(CTFBotVision_CollectPotentiallyVisibleEntities, "CTFBotVision::CollectPotentiallyVisibleEntities");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			NDebugOverlay::Clear();
			
			CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(1));
			if (player == nullptr) return;
			
			CBaseObject *obj_sentry    = player->GetObjectOfType(OBJ_SENTRYGUN,  0);
			CBaseObject *obj_dispenser = player->GetObjectOfType(OBJ_DISPENSER,  0);
			CBaseObject *obj_tele1     = player->GetObjectOfType(OBJ_TELEPORTER, 0);
			CBaseObject *obj_tele2     = player->GetObjectOfType(OBJ_TELEPORTER, 1);
			
			int x = cvar_xpos.GetInt();
			int y = cvar_ypos.GetInt();
			
			if (obj_sentry != nullptr) {
				ScreenText(x, y, 0, CFmtStr("SENTRY:     %3d/%3d HP", obj_sentry->GetHealth(), obj_sentry->GetMaxHealth()), white);
				DrawHPBar (x, y, 1, player, obj_sentry);
			} else {
				ScreenText(x, y, 0, "SENTRY:           NOPE", grayA0);
				DrawHPBar (x, y, 1, player, nullptr);
			}
			
			y += cvar_spacing.GetInt();
			
			if (obj_dispenser != nullptr) {
				ScreenText(x, y, 0, CFmtStr("DISPENSER:  %3d/%3d HP", obj_dispenser->GetHealth(), obj_dispenser->GetMaxHealth()), white);
				DrawHPBar (x, y, 1, player, obj_dispenser);
			} else {
				ScreenText(x, y, 0, "DISPENSER:        NOPE", grayA0);
				DrawHPBar (x, y, 1, player, nullptr);
			}
			
			y += cvar_spacing.GetInt();
			
			if (obj_tele1 != nullptr) {
				ScreenText(x, y, 0, CFmtStr("TELE ENTR:  %3d/%3d HP", obj_tele1->GetHealth(), obj_tele1->GetMaxHealth()), white);
				DrawHPBar (x, y, 1, player, obj_tele1);
			} else {
				ScreenText(x, y, 0, "TELE ENTR:        NOPE", grayA0);
				DrawHPBar (x, y, 1, player, nullptr);
			}
			
			y += cvar_spacing.GetInt();
			
			if (obj_tele2 != nullptr) {
				ScreenText(x, y, 0, CFmtStr("TELE EXIT:  %3d/%3d HP", obj_tele2->GetHealth(), obj_tele2->GetMaxHealth()), white);
				DrawHPBar (x, y, 1, player, obj_tele2);
			} else {
				ScreenText(x, y, 0, "TELE EXIT:        NOPE", grayA0);
				DrawHPBar (x, y, 1, player, nullptr);
			}
			
			/* ensure that the bots instantly become aware of buildings, without
			 * needing to be facing in the right direction etc */
			ForEachTFBot([=](CTFBot *bot){
				if (bot == nullptr)  return;
				if (!bot->IsAlive()) return;
				
				const char *bot_name = bot->GetPlayerName();
				
				if (obj_sentry != nullptr && V_stristr(bot_name, "sentry") != nullptr) {
					bot->GetVisionInterface()->AddKnownEntity(obj_sentry);
				}
				
				if (obj_dispenser != nullptr && V_stristr(bot_name, "dispenser") != nullptr) {
					bot->GetVisionInterface()->AddKnownEntity(obj_dispenser);
				}
				
				if (obj_tele1 != nullptr && V_stristr(bot_name, "tele1") != nullptr) {
					bot->GetVisionInterface()->AddKnownEntity(obj_tele1);
				}
				
				if (obj_tele2 != nullptr && V_stristr(bot_name, "tele2") != nullptr) {
					bot->GetVisionInterface()->AddKnownEntity(obj_tele2);
				}
			});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_building_status", "0", FCVAR_NOTIFY,
		"Debug: draw large building status \"HUD\" to help elucidate MvM Building Health upgrade exploit",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
