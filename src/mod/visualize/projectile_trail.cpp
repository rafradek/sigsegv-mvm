#include "mod.h"
#include "stub/projectiles.h"
#include "stub/tf_shareddefs.h"
#include "stub/baseplayer.h"
#include "util/misc.h"


namespace Mod::Visualize::Projectile_Trail
{
	ConVar cvar_interval("sig_visualize_projectile_trail_interval", "1", FCVAR_NOTIFY,
		"Visualization: frame interval");
	
	
	Color ParseColorConVar24Bit(ConVar *convar)
	{
#if 0
		int raw = std::stoi(convar->GetString(), nullptr, 0);
		
		return Color(
			(raw >> 16) & 0xff,
			(raw >>  8) & 0xff,
			(raw >>  0) & 0xff,
			0xff);
#endif
		
		int raw = std::stoi(convar->GetString(), nullptr, 0);
		
		Color c(
			(raw >> 16) & 0xff,
			(raw >>  8) & 0xff,
			(raw >>  0) & 0xff);
		
		return c;
	}
	Color ParseColorConVar32Bit(ConVar *convar)
	{
		int raw = std::stoi(convar->GetString(), nullptr, 0);
		
		return Color(
			(raw >> 24) & 0xff,
			(raw >> 16) & 0xff,
			(raw >>  8) & 0xff,
			(raw >>  0) & 0xff);
	}
	
	
	ConVar cvar_duration("sig_visualize_projectile_trail_duration", "3.0", FCVAR_NOTIFY,
		"Visualization: trail overlay duration");
	
	ConVar cvar_color_mode("sig_visualize_projectile_trail_color_mode", "rainbow", FCVAR_NOTIFY,
		"Visualization: trail overlay color mode (fixed, team, bot, type, rainbow, random)");
	
	ConVar cvar_color_fixed("sig_visualize_projectile_trail_color_fixed", "0x00ff00", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'fixed'");
	
	ConVar cvar_color_team_default("sig_visualize_projectile_trail_color_team_default", "0xffffff", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'team': default");
	ConVar cvar_color_team_red("sig_visualize_projectile_trail_color_team_red", "0xff2020", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'team': TF_TEAM_RED");
	ConVar cvar_color_team_blu("sig_visualize_projectile_trail_color_team_blu", "0x2020ff", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'team': TF_TEAM_BLUE");
	
	ConVar cvar_color_bot_yes("sig_visualize_projectile_trail_color_bot_yes", "0xffff20", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'bot': bots");
	ConVar cvar_color_bot_no("sig_visualize_projectile_trail_color_bot_no", "0x20ff20", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'bot': humans");
	
	ConVar cvar_color_type_default("sig_visualize_projectile_trail_color_type_default", "0xffffff", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': default");
	ConVar cvar_color_type_stock("sig_visualize_projectile_trail_color_type_stock", "0xff9020", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': stock");
	ConVar cvar_color_type_lochnload("sig_visualize_projectile_trail_color_type_lochnload", "0x20ff20", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': lochnload");
	ConVar cvar_color_type_ironbomber("sig_visualize_projectile_trail_color_type_ironbomber", "0xffff20", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': ironbomber");
	ConVar cvar_color_type_loosecannon("sig_visualize_projectile_trail_color_type_loosecannon", "0xff2020", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': loosecannon");
	ConVar cvar_color_type_oldlochnload("sig_visualize_projectile_trail_color_type_oldlochnload", "0x20ffff", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': oldlochnload");
	ConVar cvar_color_type_normal("sig_visualize_projectile_trail_color_type_normal", "0xffff20", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': normal");
	ConVar cvar_color_type_festive("sig_visualize_projectile_trail_color_type_festive", "0xff2020", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': festive");
	ConVar cvar_color_type_breadmonster("sig_visualize_projectile_trail_color_type_breadmonster", "0x20ff20", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': breadmonster");
	ConVar cvar_color_type_sandman("sig_visualize_projectile_trail_color_type_sandman", "0xffff20", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': sandman");
	ConVar cvar_color_type_wrapassassin("sig_visualize_projectile_trail_color_type_wrapassassin", "0xff2020", FCVAR_NOTIFY,
		"Visualization: trail overlay color for mode 'type': wrapassassin");
	
	
	Color GetTrailColor_Fixed(CBaseProjectile *proj)
	{
		return ParseColorConVar24Bit(&cvar_color_fixed);
	}
	
	Color GetTrailColor_Team(CBaseProjectile *proj)
	{
		switch (proj->GetTeamNumber()) {
		default:           return ParseColorConVar24Bit(&cvar_color_team_default);
		case TF_TEAM_RED:  return ParseColorConVar24Bit(&cvar_color_team_red);
		case TF_TEAM_BLUE: return ParseColorConVar24Bit(&cvar_color_team_blu);
		}
	}
	
	Color GetTrailColor_Bot(CBaseProjectile *proj)
	{
		bool is_bot = false;
		
		CBaseEntity *launcher = proj->GetOriginalLauncher();
		if (launcher != nullptr) {
			CBasePlayer *owner = ToBasePlayer(launcher->GetOwnerEntity());
			if (owner != nullptr) {
				is_bot = owner->IsBot();
			}
		}
		
		if (is_bot) {
			return ParseColorConVar24Bit(&cvar_color_bot_yes);
		} else {
			return ParseColorConVar24Bit(&cvar_color_bot_no);
		}
	}
	
	Color GetTrailColor_Type(CBaseProjectile *proj)
	{
		/* this is a bit of a hack for now */
		
		CBasePlayer *owner = nullptr;
		const char *name = nullptr;
		
		CBaseEntity *launcher = proj->GetOriginalLauncher();
		if (launcher != nullptr) {
			owner = ToBasePlayer(launcher->GetOwnerEntity());
			name  = owner->GetPlayerName();
		}
		
		if (name != nullptr && strcmp(name, "GL_NoSpin")               == 0) return Color(0x90, 0x20, 0xff, 0xff);
		if (name != nullptr && strcmp(name, "GL_NoSpin_Yaw180")        == 0) return Color(0x20, 0x90, 0xff, 0xff);
		if (name != nullptr && strcmp(name, "GL_NoSpin_Yaw45")         == 0) return Color(0x20, 0xff, 0xff, 0xff);
		if (name != nullptr && strcmp(name, "GL_NoSpin_Pitch45")       == 0) return Color(0x20, 0xff, 0x20, 0xff);
		if (name != nullptr && strcmp(name, "GL_NoSpin_Pitch90")       == 0) return Color(0xff, 0xff, 0x20, 0xff);
		if (name != nullptr && strcmp(name, "GL_NoSpin_Yaw90")         == 0) return Color(0xff, 0x90, 0x20, 0xff);
		if (name != nullptr && strcmp(name, "GL_Spin")                 == 0) return Color(0xff, 0x20, 0x20, 0xff);
		if (name != nullptr && strcmp(name, "GL_NoSpin_Yaw45_Pitch45") == 0) return Color(0xff, 0x20, 0xff, 0xff);
		
	//	if (name != nullptr && strstr(name, "Sandman") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_sandman);
	//	} else if (name != nullptr && strstr(name, "WrapAssassin") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_wrapassassin);
	//	}
		
	//	if (name != nullptr && strstr(name, "Normal") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_normal);
	//	} else if (name != nullptr && strstr(name, "Festive") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_festive);
	//	} else if (name != nullptr && strstr(name, "BreadMonster") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_breadmonster);
	//	}
		
	//	if (name != nullptr && strstr(name, "Old") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_oldlochnload);
	//	} else if (name != nullptr && strstr(name, "New") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_lochnload);
	//	}
		
	//	if (name != nullptr && strstr(name, "GrenadeLauncher") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_stock);
	//	} else if (name != nullptr && strstr(name, "LochNLoad") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_lochnload);
	//	} else if (name != nullptr && strstr(name, "IronBomber") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_ironbomber);
	//	} else if (name != nullptr && strstr(name, "LooseCannon") != nullptr) {
	//		return ParseColorConVar24Bit(&cvar_color_type_loosecannon);
	//	}
		
		return ParseColorConVar24Bit(&cvar_color_type_default);
	}
	
	Color GetTrailColor_Rainbow(CBaseProjectile *proj)
	{
		// FIXME: this structure will grow over time unnecessarily
		static std::map<CHandle<CBaseProjectile>, Color> colors;
		
		auto it = colors.find(proj);
		if (it == colors.end()) {
			static Color s_Rainbow[] = {
				{ 0xff, 0x00, 0x00, 0xff }, // red
				{ 0xff, 0x80, 0x00, 0xff }, // orange
				{ 0xff, 0xff, 0x00, 0xff }, // yellow
				{ 0x00, 0xff, 0x00, 0xff }, // green
				{ 0x00, 0xff, 0xff, 0xff }, // cyan
				{ 0x00, 0x00, 0xff, 0xff }, // blue
				{ 0xff, 0x00, 0xff, 0xff }, // magenta
			};
			static size_t idx = 0;
			
			colors.insert(std::make_pair(proj, s_Rainbow[idx++ % countof(s_Rainbow)]));
		}
		
		return colors.at(proj);
	}
	
	Color GetTrailColor_Random(CBaseProjectile *proj)
	{
		
		
		
		// TODO
		return Color();
	}
	
	
	Color GetTrailColor(CBaseProjectile *proj)
	{
		const char *mode = cvar_color_mode.GetString();
		
		ConVar *convar = nullptr;
		if (stricmp(mode, "team") == 0) {
			return GetTrailColor_Team(proj);
		} else if (stricmp(mode, "bot") == 0) {
			return GetTrailColor_Bot(proj);
		} else if (stricmp(mode, "type") == 0) {
			return GetTrailColor_Type(proj);
		} else if (stricmp(mode, "rainbow") == 0) {
			return GetTrailColor_Rainbow(proj);
		} else if (stricmp(mode, "random") == 0) {
			return GetTrailColor_Random(proj);
		} else {
			return GetTrailColor_Fixed(proj);
		}
	}
	
	
	void TrailOverlay(CBaseProjectile *proj)
	{
		// FIXME: this structure will grow over time unnecessarily
		static std::map<CHandle<CBaseProjectile>, Vector> lastpos;
		
		const Vector& curr = proj->GetAbsOrigin();
		
		auto it = lastpos.find(proj);
		if (it != lastpos.end()) {
			const Vector& prev = (*it).second;
			
			Color c = GetTrailColor(proj);
			
			NDebugOverlay::LineAlpha(prev, curr, c.r(), c.g(), c.b(), 0xff, false, cvar_duration.GetFloat());
		}
		lastpos[proj] = curr;
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Projectile_Trail") {}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % cvar_interval.GetInt() != 0) return;
			
			for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
				auto proj = rtti_cast<CBaseProjectile *>(IBaseProjectileAutoList::AutoList()[i]);
				assert(proj != nullptr);
				
				TrailOverlay(proj);
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_projectile_trail", "0", FCVAR_NOTIFY,
		"Visualization: projectile trail overlay",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
