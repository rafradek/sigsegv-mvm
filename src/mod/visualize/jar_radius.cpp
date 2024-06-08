#include "mod.h"
#include "util/scope.h"
#include "stub/projectiles.h"
#include "stub/tf_shareddefs.h"
#include "stub/misc.h"
#include "stub/tfplayer.h"


namespace Mod::Visualize::Jar_Radius
{
	[[gnu::format(printf, 1, 2)]]
	void ServerCommand(const char *format, ...)
	{
		char buf[4096];
		
		va_list va;
		va_start(va, format);
		vsnprintf(buf, sizeof(buf), format, va);
		va_end(va);
		
		engine->ServerCommand(buf);
		engine->ServerExecute();
	}
	
	
	void AddPuppetBot(const char *team, const char *playerclass, const char *name)
	{
		ServerCommand("bot -team %s -class %s -name %s\n", team, playerclass, name);
	}
	
	void TeleportBot(const char *name, const Vector& origin, const QAngle& angles)
	{
		ServerCommand("bot_teleport %s %f %f %f %f %f %f\n", name,
			origin.x, origin.y, origin.z,
			angles.x, angles.y, angles.x);
		
	//	constexpr int damage = -1000000;
	//	ServerCommand("bot_hurt -name %s -damage %d\n", name, damage);
	}
	
	
	constexpr int num_bots = 30;
	
	
	CON_COMMAND(sig_visualize_jar_radius_addbots, "")
	{
		ServerCommand("bot_kick all\n");
		
		for (int i = 0; i < num_bots; ++i) {
			AddPuppetBot("blu", "pyro", CFmtStrN<64>("Bot%d", i + 1));
		}
	}
	
	CON_COMMAND(sig_visualize_jar_radius_movebots, "")
	{
		constexpr float x =     0.0f;
		          float y =     0.0f;
		constexpr float z =   -50.0f;
		
		constexpr float ax = 0.0f;
		constexpr float ay = 0.0f;
		constexpr float az = 0.0f;
		
		for (int i = 0; i < num_bots; ++i) {
			constexpr float spacing = 100.0f;
			
			float xi = RemapValClamped(i / 5, 0, 5, -2.5f * spacing, 2.5f * spacing);
			float yi = RemapValClamped(i % 5, 0, 4, -2.0f * spacing, 2.0f * spacing);
			
			Vector origin(xi, yi, z);
			QAngle angles(ax, ay, az);
			
			TeleportBot(CFmtStrN<64>("Bot%d", i + 1), origin, angles);
		}
	}
	
	
	ConVar cvar_duration_sphere("sig_visualize_jar_radius_duration_sphere", "3.00", FCVAR_NOTIFY,
		"Visualization: duration for the sphere overlay");
	ConVar cvar_duration_player("sig_visualize_jar_radius_duration_player", "10.00", FCVAR_NOTIFY,
		"Visualization: duration for the player damage overlay");
	ConVar cvar_clear("sig_visualize_jar_radius_clear", "1", FCVAR_NOTIFY,
		"Visualization: clear overlays each time?");
	
	
	bool is_jarate = false;
	bool is_milk   = false;
	DETOUR_DECL_MEMBER(void, CTFProjectile_Jar_Explode, CGameTrace *trace, int i1)
	{
		auto jar = reinterpret_cast<CTFProjectile_Jar *>(this);
		
		switch (jar->GetWeaponID()) {
		case TF_WEAPON_GRENADE_JAR:
			is_jarate = true;
			DevMsg("weapon ID: GRENADE_JAR\n");
			break;
		case TF_WEAPON_GRENADE_JAR_MILK:
			is_milk = true;
			DevMsg("weapon ID: GRENADE_JAR_MILK\n");
			break;
		default:
			DevMsg("weapon ID: %02x\n", jar->GetWeaponID());
			break;
		}
		
		DETOUR_MEMBER_CALL(trace, i1);
		
		is_jarate = false;
		is_milk   = false;
	}
	
	DETOUR_DECL_STATIC(int, UTIL_EntitiesInSphere, const Vector& center, float radius, CFlaggedEntitiesEnum *pEnum)
	{
		DevMsg("radius: %.1f\n", radius);
		
		if (cvar_clear.GetBool()) {
			NDebugOverlay::Clear();
		}
		
		DevMsg("sphere\n");
		if (is_jarate) {
			DevMsg("sphere: is_jarate\n");
			NDebugOverlay::Sphere(center, vec3_angle, radius, 0xff, 0xff, 0x00, 0x20, false, cvar_duration_sphere.GetFloat());
		}
		if (is_milk) {
			DevMsg("sphere: is_milk\n");
			NDebugOverlay::Sphere(center, vec3_angle, radius, 0xff, 0xff, 0xff, 0x20, false, cvar_duration_sphere.GetFloat());
		}
		
		return DETOUR_STATIC_CALL(center, radius, pEnum);
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFPlayerShared_AddCond, ETFCond cond, float duration, CTFPlayer *provider)
	{
		DevMsg("addcond\n");
		
		if (is_jarate && cond == TF_COND_URINE) {
			DevMsg("addcond URINE\n");
			
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			CTFPlayer *player = shared->GetOuter();
			
			NDebugOverlay::EntityBounds(player, 0xff, 0xff, 0x00, 0x20, cvar_duration_player.GetFloat());
			
		//	NDebugOverlay::EntityText(ENTINDEX(player), 0, player->GetPlayerName(), cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(player), 1, "JARATED", cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(player), 2, CFmtStrN<64>("%.1f sec", duration), cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
		}
		
		if (is_milk && cond == TF_COND_MAD_MILK) {
			DevMsg("addcond MAD_MILK\n");
			
			auto shared = reinterpret_cast<CTFPlayerShared *>(this);
			CTFPlayer *player = shared->GetOuter();
			
			NDebugOverlay::EntityBounds(player, 0xff, 0xff, 0xff, 0x20, cvar_duration_player.GetFloat());
			
		//	NDebugOverlay::EntityText(ENTINDEX(player), 0, player->GetPlayerName(), cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(player), 1, "MILKED", cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityText(ENTINDEX(player), 2, CFmtStrN<64>("%.1f sec", duration), cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
		}
		
		DETOUR_MEMBER_CALL(cond, duration, provider);
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Jar_Radius")
		{
			MOD_ADD_DETOUR_MEMBER(CTFProjectile_Jar_Explode, "CTFProjectile_Jar::Explode");
			MOD_ADD_DETOUR_STATIC(UTIL_EntitiesInSphere,     "UTIL_EntitiesInSphere");
			
			MOD_ADD_DETOUR_MEMBER(CTFPlayerShared_AddCond, "CTFPlayerShared::AddCond");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_jar_radius", "0", FCVAR_NOTIFY,
		"Visualization: jar radius",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
