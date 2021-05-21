#include "mod.h"
#include "stub/baseplayer.h"
#include "stub/projectiles.h"
#include "util/scope.h"
#include "util/iterate.h"


struct CTFRadiusDamageInfo
{
	CTakeDamageInfo *m_DmgInfo;   // +0x00
	Vector m_vecOrigin;           // +0x04: blast origin
	float m_flRadius;             // +0x10: blast radius
	CBaseEntity *m_pEntityIgnore; // +0x14
	float m_unknown_18;           // +0x18, default 0.0f, ?
	float m_unknown_1c;           // +0x1c, default 1.0f, scales damage force or something
	int m_unknown_20;             // +0x20, default 0, ?
	float m_flFalloff;            // +0x24: how much the damage is decreased by at the edge of the blast
};


namespace Mod::Debug::Proj_Energy_Ball
{
#if 0
	
	
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
		
		constexpr int damage = -1000000;
		ServerCommand("bot_hurt -name %s -damage %d\n", name, damage);
	}
	
	
	constexpr float y_max = 360.0f;
	constexpr float dy    =  50.0f;
	
	
	CON_COMMAND(sig_visualize_blast_radius_addbots, "")
	{
		ServerCommand("bot_kick all\n");
		
		float y = 0.0f;
		for (int i = 0; y < y_max; ++i, y += dy) {
			if (i == 0) {
				AddPuppetBot("blu", "heavy", "BotCenter");
			} else {
				AddPuppetBot("blu", "heavy", CFmtStrN<64>("BotLeft%d", i));
				AddPuppetBot("blu", "heavy", CFmtStrN<64>("BotRight%d", i));
			}
		}
	}
	
	CON_COMMAND(sig_visualize_blast_radius_movebots, "")
	{
		constexpr float x = -1024.0f;
		          float y =     0.0f;
		constexpr float z =   -50.0f;
		
		constexpr float ax = 0.0f;
		constexpr float ay = 0.0f;
		constexpr float az = 0.0f;
		
		for (int i = 0; y < y_max; ++i, y += dy) {
			if (i == 0) {
				TeleportBot("BotCenter", Vector(x, y, z), QAngle(ax, ay, az));
			} else {
				TeleportBot(CFmtStrN<64>("BotLeft%d",  i), Vector(x, -y, z), QAngle(ax, ay, az));
				TeleportBot(CFmtStrN<64>("BotRight%d", i), Vector(x,  y, z), QAngle(ax, ay, az));
			}
		}
	}
	
	
	ConVar cvar_duration_sphere("sig_visualize_blast_radius_duration_sphere", "3.00", FCVAR_NOTIFY,
		"Visualization: duration for the sphere overlay");
	ConVar cvar_duration_player("sig_visualize_blast_radius_duration_player", "10.00", FCVAR_NOTIFY,
		"Visualization: duration for the player damage overlay");
	ConVar cvar_clear("sig_visualize_blast_radius_clear", "1", FCVAR_NOTIFY,
		"Visualization: clear overlays each time?");
	
	
	CBasePlayer *GetLocalPlayer()
	{
		static ConVarRef index("sig_util_listenserverhost_index");
		return UTIL_PlayerByIndex(index.GetInt());
	}
	
	
#if 0
	Color RatioToColor(float ratio)
	{
		ratio = Clamp(ratio, 0.0f, 1.0f);
		
		float r;
		float g;
		
		if (ratio < 0.50f) {
			r = 1.00f;
			g = ratio * 2;
		} else {
			r = 1.00f - (ratio * 2);
			g = 1.00f;
		}
		
		r *= 255.0f;
		g *= 255.0f;
		
		return Color(lrint(r), lrint(g), 0x00, 0xff);
	}
#endif
	
	Color BlendColors(const Color& c1, const Color c2, float ratio)
	{
		ratio = Clamp(ratio, 0.00f, 1.00f);
		
		return Color(
			lrint(RemapVal(ratio, 0.00f, 1.00f, c1.r(), c2.r())),
			lrint(RemapVal(ratio, 0.00f, 1.00f, c1.g(), c2.g())),
			lrint(RemapVal(ratio, 0.00f, 1.00f, c1.b(), c2.b())),
			lrint(RemapVal(ratio, 0.00f, 1.00f, c1.a(), c2.a())));
	}
	
	Color RatioToColor(float ratio)
	{
		ratio = Clamp(ratio, 0.00f, 1.00f);
		
		static Color red   (0xff, 0x00, 0x00, 0xff);
		static Color orange(0xff, 0x80, 0x00, 0xff);
		static Color yellow(0xff, 0xff, 0x00, 0xff);
		static Color green (0x00, 0xff, 0x00, 0xff);
		static Color cyan  (0x00, 0xff, 0xff, 0xff);
		
		Color& c_100 = cyan;
		Color& c_75  = green;
		Color& c_50  = yellow;
		Color& c_25  = orange;
		Color& c_0   = red;
		
		if (ratio >= 0.75f) {
			return BlendColors(c_100, c_75, RemapValClamped(ratio, 1.00f, 0.75f, 0.00f, 1.00f));
		} else if (ratio >= 0.50f) {
			return BlendColors(c_75, c_50, RemapValClamped(ratio, 0.75f, 0.50f, 0.00f, 1.00f));
		} else if (ratio >= 0.25f) {
			return BlendColors(c_50, c_25, RemapValClamped(ratio, 0.50f, 0.25f, 0.00f, 1.00f));
		} else {
			return BlendColors(c_25, c_0, RemapValClamped(ratio, 0.25f, 0.00f, 0.00f, 1.00f));
		}
	}
	
	
	CTFRadiusDamageInfo *radiusinfo = nullptr;
	RefCount rc_CTFGameRules_RadiusDamage;
	DETOUR_DECL_MEMBER(void, CTFGameRules_RadiusDamage, CTFRadiusDamageInfo& info)
	{
		radiusinfo = &info;
		
		SCOPED_INCREMENT(rc_CTFGameRules_RadiusDamage);
		DETOUR_MEMBER_CALL(CTFGameRules_RadiusDamage)(info);
	}
	
	ConVar cvar_spheretext("sig_visualize_blast_radius_spheretext", "1", FCVAR_NOTIFY,
		"Visualization: show each 10-percent damage threshold on the blast sphere");
	
	DETOUR_DECL_MEMBER(void, CEntitySphereQuery_ctor, const Vector& center, float radius, int flagMask)
	{
		if (rc_CTFGameRules_RadiusDamage > 0) {
			if (cvar_clear.GetBool()) {
				NDebugOverlay::Clear();
			}
			
			QAngle angles = vec3_angle;
			CBasePlayer *player = GetLocalPlayer();
			if (player != nullptr) {
				angles = player->EyeAngles();
			//	angles.x = 0.0f;
			//	angles.z = 0.0f;
			}
			
			Vector fwd, rt, up;
			AngleVectors(angles, &fwd, &rt, &up);
			
			char buf[1024];
			
			float damage_center = radiusinfo->m_DmgInfo->GetDamage();
			float damage_edge   = damage_center * radiusinfo->m_flFalloff;
			
			float ratio_center = 1.00f;
			float ratio_edge   = (damage_edge / damage_center);
			
			DevMsg("hi %f\n", radiusinfo->m_flFalloff);
			
			for (float f = 1.0f; f >= radiusinfo->m_flFalloff - 0.01f; f -= 0.1f) {
				float dist = RemapValClamped(f, 1.0f, radiusinfo->m_flFalloff, 0.0f, radiusinfo->m_flRadius);
				
				Vector where_r = radiusinfo->m_vecOrigin + (dist * rt);
				Vector where_l = radiusinfo->m_vecOrigin - (dist * rt);
				
				if (cvar_spheretext.GetBool()) {
					/* damage percent */
					snprintf(buf, sizeof(buf), "%.0f%%", f * 100.0f);
					NDebugOverlay::EntityTextAtPosition(where_r, -2, buf, cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityTextAtPosition(where_l, -2, buf, cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					
					/* distance from center */
					snprintf(buf, sizeof(buf), "%.0f", dist);
					NDebugOverlay::EntityTextAtPosition(where_r, 0, buf, cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityTextAtPosition(where_l, 0, buf, cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					
					/* HU unit label */
					NDebugOverlay::EntityTextAtPosition(where_r, 1, "HU", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityTextAtPosition(where_l, 1, "HU", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				}
				
				/* colored sphere shell */
				Color c = RatioToColor(RemapValClamped(f, 0.9f, radiusinfo->m_flFalloff, 1.0f, 0.0f));
				NDebugOverlay::Sphere(radiusinfo->m_vecOrigin, angles, dist, c.r(), c.g(), c.b(), 0x10, false, cvar_duration_sphere.GetFloat());
				
				/* additional fake level for showing the 0% cliff */
				if (f < radiusinfo->m_flFalloff + 0.09f && cvar_spheretext.GetBool()) {
					dist = RemapVal(f - 0.1f, 1.0f, radiusinfo->m_flFalloff, 0.0f, radiusinfo->m_flRadius);
					where_r = radiusinfo->m_vecOrigin + (dist * rt);
					where_l = radiusinfo->m_vecOrigin - (dist * rt);
					
					NDebugOverlay::EntityTextAtPosition(where_r, -2, "0%", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityTextAtPosition(where_l, -2, "0%", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					
					NDebugOverlay::EntityTextAtPosition(where_r, 0, ">>", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityTextAtPosition(where_l, 0, "<<", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					
					NDebugOverlay::EntityTextAtPosition(where_r, 1, ">>", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityTextAtPosition(where_l, 1, "<<", cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				}
			}
			
			//NDebugOverlay::Sphere(radiusinfo->m_vecOrigin, angles, radiusinfo->m_flRadius, r, g, b, a, false, cvar_duration_sphere.GetFloat());
			//NDebugOverlay::Circle(radiusinfo->m_vecOrigin, angles, radiusinfo->m_flRadius, r, g, b, a, false, cvar_duration_sphere.GetFloat());
			
			//snprintf(buf, sizeof(buf), "radius: %.1f HU", radiusinfo->m_flRadius);
			//NDebugOverlay::EntityTextAtPosition(radiusinfo->m_vecOrigin, 1, buf, cvar_duration_sphere.GetFloat(), 0xff, 0xff, 0xff, 0xff);
		}
		
		DETOUR_MEMBER_CALL(CEntitySphereQuery_ctor)(center, radius, flagMask);
	}
	
	RefCount rc_CTFRadiusDamageInfo_ApplyToEntity;
	DETOUR_DECL_MEMBER(int, CTFRadiusDamageInfo_ApplyToEntity, CBaseEntity *ent)
	{
		if (rc_CTFGameRules_RadiusDamage > 0) ++rc_CTFRadiusDamageInfo_ApplyToEntity;
		auto result = DETOUR_MEMBER_CALL(CTFRadiusDamageInfo_ApplyToEntity)(ent);
		if (rc_CTFGameRules_RadiusDamage > 0) --rc_CTFRadiusDamageInfo_ApplyToEntity;
		return result;
	}
	
	ConVar cvar_playertext("sig_visualize_blast_radius_playertext", "1", FCVAR_NOTIFY,
		"Visualization: show information for each player who takes damage");
	ConVar cvar_playertext_offset("sig_visualize_blast_radius_playertext_offset", "10", FCVAR_NOTIFY,
		"Visualization: line offset for player damage taken text overlay");
	
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& info)
	{
		if (rc_CTFRadiusDamageInfo_ApplyToEntity > 0 && radiusinfo != nullptr && cvar_playertext.GetBool()) {
			auto ent = reinterpret_cast<CBaseEntity *>(this);
			if (rtti_cast<CBaseCombatCharacter *>(ent) != nullptr) {
				int line = cvar_playertext_offset.GetInt();
				
				float damage_center = radiusinfo->m_DmgInfo->GetDamage();
				float damage_edge   = damage_center * radiusinfo->m_flFalloff;
				float damage_actual = info.GetDamage();
				
			//	DevMsg("damage_center: %f\n", damage_center);
			//	DevMsg("damage_edge:   %f\n", damage_edge);
			//	DevMsg("damage_actual: %f\n", damage_actual);
				
				float ratio_center = 1.00f;
				float ratio_edge   = (damage_edge   / damage_center);
				float ratio_actual = (damage_actual / damage_center);
				
			//	DevMsg("ratio_center: %f\n", ratio_center);
			//	DevMsg("ratio_edge:   %f\n", ratio_edge);
			//	DevMsg("ratio_actual: %f\n", ratio_actual);
				
				Color c = RatioToColor(RemapValClamped(ratio_actual, ratio_edge, ratio_center, 0.00f, 1.00f));
				NDebugOverlay::EntityBounds(ent, c.r(), c.g(), c.b(), 0x20, cvar_duration_player.GetFloat());
				
				char buf[1024];
				
			//	NDebugOverlay::EntityText(ENTINDEX(ent), line - 3, "^", cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			//	NDebugOverlay::EntityText(ENTINDEX(ent), line - 2, "^", cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				
				CBasePlayer *player = ToBasePlayer(ent);
				if (player != nullptr) {
					NDebugOverlay::EntityText(ENTINDEX(ent), line - 2, player->GetPlayerName(), cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				}
				
				NDebugOverlay::EntityText(ENTINDEX(ent), line + 0, "Dist:", cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				snprintf(buf, sizeof(buf), "%.0f HU", RemapValClamped(ratio_actual, 1.0f, radiusinfo->m_flFalloff, 0.0f, radiusinfo->m_flRadius));
				NDebugOverlay::EntityText(ENTINDEX(ent), line + 1, buf, cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				
				NDebugOverlay::EntityText(ENTINDEX(ent), line + 3, "Splash:", cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				snprintf(buf, sizeof(buf), "%.0f%%", ratio_actual * 100.0f);
				NDebugOverlay::EntityText(ENTINDEX(ent), line + 4, buf, cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
				
			//	NDebugOverlay::EntityText(ENTINDEX(ent), line + 6, "Dmg:", cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			//	snprintf(buf, sizeof(buf), "%.0f", info.GetDamage());
			//	NDebugOverlay::EntityText(ENTINDEX(ent), line + 7, buf, cvar_duration_player.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			}
		}
		
		return DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(info);
	}
#endif
	
	
//	DETOUR_DECL_STATIC(CTFProjectile_EnergyBall *, CTFProjectile_EnergyBall_Create, const Vector& origin, const QAngle& angles, float f1, float f2, CBaseEntity *ent1, CBaseEntity *ent2)
//	{
//		
//	}
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Proj_Energy_Ball")
		{
		//	#error need addrs for most of these
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_Create,          "CTFProjectile_EnergyBall::Create");
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_Spawn,           "CTFProjectile_EnergyBall::Spawn");
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_Explode,         "CTFProjectile_EnergyBall::Explode");
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_InitEnergyBall,  "CTFProjectile_EnergyBall::InitEnergyBall");
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_FadeOut,         "CTFProjectile_EnergyBall::FadeOut");
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_EnergyBall_RemoveThink,     "CTFProjectile_EnergyBall::RemoveThink");
			
		//	#error need addrs for most of these
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_Create,          "CTFProjectile_Rocket::Create");
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_Spawn,           "CTFProjectile_Rocket::Spawn");
		//	MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Explode,                "CTFBaseRocket::Explode");
		//	MOD_ADD_DETOUR_MEMBER(CTFProjectile_Rocket_RocketTouch,     "CTFProjectile_Rocket::RocketTouch");
		//	MOD_ADD_DETOUR_MEMBER(CTFBaseRocket_Destroy,                "CTFBaseRocket::Destroy");
			
			
			
			
		//	MOD_ADD_DETOUR_MEMBER(CTFGameRules_RadiusDamage,         "CTFGameRules::RadiusDamage");
		//	MOD_ADD_DETOUR_MEMBER(CEntitySphereQuery_ctor,           "CEntitySphereQuery::CEntitySphereQuery [C1]");
		//	MOD_ADD_DETOUR_MEMBER(CTFRadiusDamageInfo_ApplyToEntity, "CTFRadiusDamageInfo::ApplyToEntity");
		//	MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage,            "CBaseEntity::TakeDamage");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			ForEachEntityByRTTI<CTFBaseRocket>([](CTFBaseRocket *proj){
				auto rocket      = rtti_cast<CTFProjectile_Rocket     *>(proj);
				auto energy_ball = rtti_cast<CTFProjectile_EnergyBall *>(proj);
				
				if (rocket != nullptr || energy_ball != nullptr) {
					DevMsg("[%08d  PRE] #%04d %s [ %+7.2f %+7.2f %+7.2f ]\n",
						gpGlobals->tickcount, ENTINDEX(proj),
						(rocket != nullptr ? "ROCKET" : "ENERGY"),
						proj->GetAbsOrigin().x, proj->GetAbsOrigin().y, proj->GetAbsOrigin().z);
					
					// rocket pre: light red
					// energy pre: light green
					int r = (rocket != nullptr ? 0xff : 0x80);
					int g = (rocket != nullptr ? 0x80 : 0xff);
					int b = (rocket != nullptr ? 0x80 : 0x80);
					int a = 0xff;
					
					NDebugOverlay::Box(proj->GetAbsOrigin(), Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f), r, g, b, a, 3600.0f);
				}
			});
		}
		
		virtual void FrameUpdatePostEntityThink() override
		{
			ForEachEntityByRTTI<CTFBaseRocket>([](CTFBaseRocket *proj){
				auto rocket      = rtti_cast<CTFProjectile_Rocket     *>(proj);
				auto energy_ball = rtti_cast<CTFProjectile_EnergyBall *>(proj);
				
				if (rocket != nullptr || energy_ball != nullptr) {
					DevMsg("[%08d POST] #%04d %s [ %+7.2f %+7.2f %+7.2f ]\n",
						gpGlobals->tickcount, ENTINDEX(proj),
						(rocket != nullptr ? "ROCKET" : "ENERGY"),
						proj->GetAbsOrigin().x, proj->GetAbsOrigin().y, proj->GetAbsOrigin().z);
					
					// rocket post: red
					// energy post: green
					int r = (rocket != nullptr ? 0xff : 0x00);
					int g = (rocket != nullptr ? 0x00 : 0xff);
					int b = (rocket != nullptr ? 0x00 : 0x00);
					int a = 0xff;
					
					NDebugOverlay::Box(proj->GetAbsOrigin(), Vector(-1.0f, -1.0f, -1.0f), Vector(1.0f, 1.0f, 1.0f), r, g, b, a, 3600.0f);
				}
			});
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_proj_energy_ball", "0", FCVAR_NOTIFY,
		"Debug: diagnose why Cow Mangler projectiles can damage enemies through walls in some cases",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}

/*
[#] CTFProjectile_Rocket
 0: +000000 public
 |  [+] CTFBaseRocket
 |   |  [#] CBaseProjectile
 |   |   0: +000000 public
 |   |   |  [+] CBaseAnimating
 |   |   |   |  [+] CBaseEntity
 |   |   |   |   |  [+] IServerEntity
 |   |   |   |   |   |  [+] IServerUnknown
 |   |   |   |   |   |   |  [-] IHandleEntity
 |   |   1: +000488 public
 |   |   |  [-] IBaseProjectileAutoList
 1: +0004c0 public
 |  [-] IScorer

[+] CTFProjectile_EnergyBall
 |  [+] CTFBaseRocket
 |   |  [#] CBaseProjectile
 |   |   0: +000000 public
 |   |   |  [+] CBaseAnimating
 |   |   |   |  [+] CBaseEntity
 |   |   |   |   |  [+] IServerEntity
 |   |   |   |   |   |  [+] IServerUnknown
 |   |   |   |   |   |   |  [-] IHandleEntity
 |   |   1: +000488 public
 |   |   |  [-] IBaseProjectileAutoList
*/
