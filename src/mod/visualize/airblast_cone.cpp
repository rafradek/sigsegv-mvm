#include "mod.h"
#include "stub/tfplayer.h"
#include "stub/tfweaponbase.h"


#if 0 // obsolete as of Inferno Update

namespace Mod::Visualize::Airblast_Cone
{
	ConVar cvar_clear("sig_visualize_airblast_cone_clear", "1", FCVAR_NOTIFY,
		"Visualization: set 1 to clear overlays each time");
	ConVar cvar_duration("sig_visualize_airblast_cone_duration", "10.0", FCVAR_NOTIFY,
		"Visualization: cone draw duration");
	ConVar cvar_depth("sig_visualize_airblast_cone_depth", "180.0", FCVAR_NOTIFY,
		"Visualization: cone draw depth");
	
	ConVar cvar_color_r("sig_visualize_airblast_cone_color_r", "255", FCVAR_NOTIFY,
		"Visualization: cone color (red)");
	ConVar cvar_color_g("sig_visualize_airblast_cone_color_g", "255", FCVAR_NOTIFY,
		"Visualization: cone color (green)");
	ConVar cvar_color_b("sig_visualize_airblast_cone_color_b", "255", FCVAR_NOTIFY,
		"Visualization: cone color (blue)");
	ConVar cvar_color_a("sig_visualize_airblast_cone_color_a", "255", FCVAR_NOTIFY,
		"Visualization: cone color (alpha)");
	
	
	ConVar cvar_circle("sig_visualize_airblast_cone_circle", "1", FCVAR_NOTIFY,
		"Visualization: use DrawCone_Circles");
	ConVar cvar_circle_delta("sig_visualize_airblast_cone_circle_delta", "15.0", FCVAR_NOTIFY,
		"Visualization: distance between circles for DrawCone_Circles");
	void DrawCone_Circles(const Vector& origin, const QAngle& angle, float dot, float depth)
	{
		if (!cvar_circle.GetBool()) return;
		
		int r = cvar_color_r.GetInt();
		int g = cvar_color_g.GetInt();
		int b = cvar_color_b.GetInt();
		int a = cvar_color_a.GetInt();
		float duration = cvar_duration.GetFloat();
		
		float delta = cvar_circle_delta.GetFloat();
		
		Vector fwd;
		AngleVectors(angle, &fwd);
		
		for (float dist = 1.0f; dist <= depth; dist += delta) {
			Vector center = origin + (fwd * dist);
			float radius = dist * tan(acos(dot));
			
			NDebugOverlay::Circle(center, angle, radius, r, g, b, a, false, duration);
		}
	}
	
	ConVar cvar_line("sig_visualize_airblast_cone_line", "0", FCVAR_NOTIFY,
		"Visualization: use DrawCone_Lines");
	ConVar cvar_line_count("sig_visualize_airblast_cone_line_count", "60", FCVAR_NOTIFY,
		"Visualization: number of lines for DrawCone_Lines");
	void DrawCone_Lines(const Vector& origin, const QAngle& angle, float dot, float depth)
	{
		if (!cvar_line.GetBool()) return;
		
		Vector fwd, rt, up;
		AngleVectors(angle, &fwd, &rt, &up);
		
		Vector center = origin + (fwd * depth);
		
		int num_lines = cvar_line_count.GetInt();
		for (int i = 0; i < num_lines; ++i) {
			// TODO
		}
	}
	
	ConVar cvar_3d("sig_visualize_airblast_cone_3d", "0", FCVAR_NOTIFY,
		"Visualization: use DrawCone_3D");
	ConVar cvar_3d_polys("sig_visualize_airblast_cone_3d_polys", "60", FCVAR_NOTIFY,
		"Visualization: number of triangle polygons for DrawCone_3D");
	void DrawCone_3D(const Vector& origin, const QAngle& angle, float dot, float depth)
	{
		if (!cvar_3d.GetBool()) return;
		
		int r = cvar_color_r.GetInt();
		int g = cvar_color_g.GetInt();
		int b = cvar_color_b.GetInt();
		int a = cvar_color_a.GetInt();
		float duration = cvar_duration.GetFloat();
		
		Vector vecFwd;
		AngleVectors(angle, &vecFwd);
		Vector vecEndCenter = origin + (vecFwd * depth);
		
		matrix3x4_t xform;
		AngleMatrix(angle, origin, xform);
		Vector xAxis;
		MatrixGetColumn(xform, 2, xAxis);
		Vector yAxis;
		MatrixGetColumn(xform, 1, yAxis);
		
		float radius = depth * tan(acos(dot));
		
		Vector vecPosition;
		Vector vecLastPosition;
		
		int num_polys = cvar_3d_polys.GetInt();
		
		float flRadStep = (2.0f * M_PI) / (float)num_polys;
		
		for (int i = 1; i <= num_polys + 1; ++i) {
			vecLastPosition = vecPosition;
			
			float flSin, flCos;
			SinCos(i * flRadStep, &flSin, &flCos);
			
			vecPosition = vecEndCenter + (radius * flCos * xAxis) + (radius * flSin * yAxis);
			
			if (i > 1) {
				const Vector& p1 = origin;
				const Vector& p2 = vecLastPosition;
				const Vector& p3 = vecPosition;
				
				/* lines */
				NDebugOverlay::Line(origin,          vecPosition, r, g, b, false, duration);
				NDebugOverlay::Line(vecLastPosition, vecPosition, r, g, b, false, duration);
				
				if (cvar_color_a.GetInt() != 0) {
					/* forward and back faces */
					NDebugOverlay::Triangle(p1, p2, p3, r, g, b, a, false, duration);
					NDebugOverlay::Triangle(p1, p3, p2, r, g, b, a, false, duration);
				}
			}
		}
		
		// TODO: draw circle for end (?)
	}
	
	
	std::map<CHandle<CTFPlayer>, bool> deflected;
	DETOUR_DECL_MEMBER(void, CTFFlameThrower_FireAirBlast, int i1)
	{
		auto flamethrower = reinterpret_cast<CTFFlameThrower *>(this);
		CTFPlayer *pyro = flamethrower->GetTFPlayerOwner();
		
		deflected.clear();
		for (int i = 1; i <= gpGlobals->maxClients; ++i) {
			CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(i));
			if (player == nullptr) continue;
			if (player == pyro)    continue;
			
			deflected[player] = false;
		}
		
		DETOUR_MEMBER_CALL(i1);
		
		if (cvar_clear.GetBool()) {
			NDebugOverlay::Clear();
		}
		
		DrawCone_Circles(pyro->WorldSpaceCenter(), pyro->EyeAngles(), 0.8f, cvar_depth.GetFloat());
		DrawCone_Lines  (pyro->WorldSpaceCenter(), pyro->EyeAngles(), 0.8f, cvar_depth.GetFloat());
		DrawCone_3D     (pyro->WorldSpaceCenter(), pyro->EyeAngles(), 0.8f, cvar_depth.GetFloat());
		
		for (const auto& pair : deflected) {
			CTFPlayer *victim  = pair.first;
			bool was_deflected = pair.second;
			
			int r = (was_deflected ? 0x00 : 0xff);
			int g = (was_deflected ? 0xff : 0x00);
			int b = 0x00;
			int a = 0xff;
			
			NDebugOverlay::Sphere(victim->WorldSpaceCenter(), vec3_angle, 3.0f, r, g, b, a, true, cvar_duration.GetFloat());
		}
	}
	
	DETOUR_DECL_MEMBER(bool, CTFFlameThrower_DeflectPlayer, CTFPlayer *pVictim, CTFPlayer *pPyro, const Vector& vecEyeFwd, const Vector& vecBoxCenter, const Vector& vecBoxSize)
	{
		auto result = DETOUR_MEMBER_CALL(pVictim, pPyro, vecEyeFwd, vecBoxCenter, vecBoxSize);
		
		if (result) {
			auto it = deflected.find(pVictim);
			if (it != deflected.end()) {
				(*it).second = true;
			} else {
				Warning("Airblast cone visualization: can't find player #%d in map\n", ENTINDEX(pVictim));
			}
		}
		
		return result;
	}
	
	
	class CMod : public IMod
	{
	public:
		CMod() : IMod("Visualize:Airblast_Cone")
		{
			MOD_ADD_DETOUR_MEMBER(CTFFlameThrower_FireAirBlast,  "CTFFlameThrower::FireAirBlast");
			MOD_ADD_DETOUR_MEMBER(CTFFlameThrower_DeflectPlayer, "CTFFlameThrower::DeflectPlayer");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_airblast_cone", "0", FCVAR_NOTIFY,
		"Visualization: draw cone used for airblast deflection of players",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
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
	
	CON_COMMAND(sig_visualize_airblast_cone_bot_add, "")
	{
		static const char *classnames[] = {
			"scout",
			"soldier",
			"pyro",
			"demoman",
			"heavy",
			"engineer",
			"medic",
			"sniper",
			"spy",
			nullptr,
		};
		
		for (int i = 0; i < gpGlobals->maxClients - 1; ++i) {
			const char *classname = classnames[i % 9];
			if (classname == nullptr) {
				Warning("Null class name string!\n");
				continue;
			}
			
			ServerCommand("bot -name Bot%02d -team blu -class %s\n", i + 1, classname);
		}
	}
	
	CON_COMMAND(sig_visualize_airblast_cone_bot_teleport, "")
	{
		struct GridPos
		{
			int x;
			int y;
		};
		
		if (args.ArgC() != 9) return;
		
		Vector vecCenter(atof(args[1]), atof(args[2]), atof(args[3]));
		Vector vecAngles(atof(args[4]), atof(args[5]), atof(args[6]));
		
		float spacing_x = atof(args[7]);
		float spacing_y = atof(args[8]);
		
		unsigned int num_bots = gpGlobals->maxClients - 1;
		
		std::vector<GridPos> grid;
		
		for (int d = 0; grid.size() < num_bots; ++d) {
			for (int x = -d; x <= d; ++x) {
				for (int y = -d; y <= d; ++y) {
					grid.push_back({ x, y });
				}
			}
		}
		
		// for debugging
		for (const auto& pos : grid) {
			DevMsg("[ %+1d %+1d ]\n", pos.x, pos.y);
		}
		
		for (unsigned int i = 0; i < num_bots; ++i) {
			Vector vecPos = vecCenter;
			vecPos.x += (spacing_x * grid[i].x);
			vecPos.y += (spacing_y * grid[i].y);
			
			ServerCommand("bot_teleport Bot%02d %f %f %f %f %f %f\n", i + 1,
				vecPos.x,    vecPos.y,    vecPos.z,
				vecAngles.x, vecAngles.y, vecAngles.z);
		}
	}
}

#endif
