#include "mod.h"
#include "stub/tfbot.h"
#include "util/clientmsg.h"
#include "util/iterate.h"


namespace Mod::Debug::Hurt_Float
{
	template<typename... ARGS>
	void FancyMsg(float x, float y, const char *fmt, ARGS&&... args)
	{
		CFmtStr str(fmt, std::forward<ARGS>(args)...);
		NDebugOverlay::ScreenText(x, y, str.Get(), 0xff, 0xff, 0xff, 0xff, 3600.0f);
	}
	
	
	double g_damage = 2499.000;
	double g_incr   =    0.100;
	
	
	CON_COMMAND(sig_debug_hurt_float, "")
	{
	//	if (args.ArgC() < 2) {
	//		Warning("Insufficient arguments\n");
	//		return;
	//	}
	//	
	//	float flTarget = atof(args[1]);
		
		ForEachTFBot([=](CTFBot *bot){
			if (!bot->IsAlive()) return;
			if (bot->GetMission() != CTFBot::MISSION_DESTROY_SENTRIES) return;
			
	//		int iHealth = bot->GetHealth();
	//		float flHealth = (float)iHealth;
	//		
	//		float flDamage = flHealth - flTarget;
			
			float flDamage = g_damage;
			g_damage += g_incr;
			
			CTFPlayer *pDamager = ToTFPlayer(UTIL_PlayerByIndex(1));
			CBaseEntity *pWeapon = (pDamager != nullptr ? pDamager : bot)->GetActiveTFWeapon();
			
			CTakeDamageInfo dmginfo(pDamager, pDamager, pWeapon, vec3_origin, vec3_origin, flDamage, DMG_BULLET);
			
			NDebugOverlay::Clear();
			
			FancyMsg(0.10f, 0.10f, " PRE-DAMAGE:  %4d HP  Alive: %-3s", bot->GetHealth(), (bot->IsAlive() ? "YES" : "NO"));
			FancyMsg(0.45f, 0.10f, "Inflicted %.3f damage.", flDamage);
			bot->TakeDamage(dmginfo);
			FancyMsg(0.10f, 0.13f, "POST-DAMAGE:  %4d HP  Alive: %-3s", bot->GetHealth(), (bot->IsAlive() ? "YES" : "NO"));
		});
	}
	
	CON_COMMAND(sig_debug_hurt_float_dmg, "")
	{
		g_damage = atof(args[1]);
	}
	CON_COMMAND(sig_debug_hurt_float_incr, "")
	{
		g_incr = atof(args[1]);
	}
}
