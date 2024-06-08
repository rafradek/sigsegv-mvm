#include "mod.h"
#include "stub/tfentities.h"
#include "stub/tfweaponbase.h"
#include "stub/tfplayer.h"
#include "stub/projectiles.h"
#include "util/scope.h"
#include "util/backtrace.h"
#include "util/iterate.h"


#define TRACE_ENABLE 0
#define TRACE_TERSE  0
#include "util/trace.h"


/* from CBaseEntity */
enum thinkmethods_t : int32_t
{
	THINK_FIRE_ALL_FUNCTIONS,
	THINK_FIRE_BASE_ONLY,
	THINK_FIRE_ALL_BUT_BASE,
};


// FINDINGS:
// - the problem is specifically that flame entities exist for too long (thereby going farther)
// - high latency (ping) in itself is not the problem
// - short-term changes in ping, especially jitter, is *part*, but not all, of the problem
// - the ultimate culprit is sv_clockcorrection_msecs (server-side clock correction)
//   - the default value of this convar is 60
//   - setting it to 0 completely solves the flame distance variance problem
//     (but obviously might have other deleterious effects on who-knows-what)
// 
// Laggy, jittery clients plus server-side clock correction leads to
// - flames lasting longer and therefore traveling farther before being removed
// - greatly increased overall variance in flame duration/distance


// STUFF THAT I HAVE RULED OUT:
// - random number generation (for tf_flamethrower_vecrand or for tf_flamethrower_flametime)
// - anything related to tf_flamethrower_float or tf_flamethrower_drag
// - client side prediction
// - insufficient CPU priority on dedicated server (RT prio had no effect)
// - skipping ticks or thinks
// - client side interp
// - lag compensation
// - client-side clock drift correction
// - server gpGlobals->curtime not agreeing with reality (Plat_FloatTime etc)


// MORE SPECIFIC FINDINGS:
// - flames are missing several of their first thinks when mojo is in effect
// - when missing thinks occur:
//   CBaseEntity::PhysicsRunSpecificThink is not calling CBaseEntity::PhysicsDispatchThink
//   - this, in turn, is due to GetNextThinkTick(nContextIndex) being <= 0 or > gpGlobals->tickcount


// EVEN MORE SPECIFIC FINDINGS:
// - CTFFlameEntity::Create (and therefore CTFFlameEntity::Spawn as well) are called
//   from a tree originating in CBasePlayer::PhysicsSimulate, CBasePlayer::PlayerRunCommand, etc
// - so the gpGlobals->curtime those functions see is WRONG
// - and they use this gpGlobals->curtime to set the first think time, as well as the end of its time-to-live!
// - but then, the CTFFlameEntity itself sees the actual gpGlobals->curtime
//   and its thinks are based on the actual gpGlobals->curtime

// gpGlobals->curtime specifically gets whacked in CPlayerMove::RunCommand:
//     gpGlobals->curtime = player->m_nTickBase * TICK_INTERVAL;


/*
  2  ffc23180  f16de0c8  DispatchSpawn(CBaseEntity*)+0x1f8
  3  ffc231c0  f12b4b81  CBaseEntity::Create(char const*, Vector const&, QAngle const&, CBaseEntity*)+0x31
  4  ffc231e0  f1134c8b  CTFFlameEntity::Create(Vector const&, QAngle const&, CBaseEntity*, float, int, float, bool, bool)+0x3b
  5  ffc232b0  dd11851e  ???+0x0
  6  ffc23300  f11367e1  CTFFlameThrower::PrimaryAttack() [clone .part.170]+0x4b1
  7  ffc234c0  f1136e12  CTFFlameThrower::ItemPostFrame()+0x232
  8  ffc23540  f0f1891f  CBasePlayer::ItemPostFrame()+0x2cf
  9  ffc235c0  f142bd26  CBasePlayer::PostThink()+0x426
 10  ffc236c0  f1686c74  CTFPlayer::PostThink()+0x14
 11  ffc23750  f14394c0  CPlayerMove::RunPostThink(CBasePlayer*)+0xc0
 12  ffc237c0  f1439e86  CPlayerMove::RunCommand(CBasePlayer*, CUserCmd*, IMoveHelper*)+0x8e6
 13  ffc23880  f141a12c  CBasePlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)+0x9c
 14  ffc238a0  f1662825  CTFPlayer::PlayerRunCommand(CUserCmd*, IMoveHelper*)+0x125
 15  ffc23910  f14333a7  CBasePlayer::PhysicsSimulate()+0x607
 16  ffc239e0  f140344b  Physics_SimulateEntity(CBaseEntity*)+0x45b
 17  ffc23a70  dd117c0d  ???+0x0
 18  ffc23ab0  f140390a  Physics_RunThinkFunctions(bool)+0x22a
 19  ffc23b80  dd117708  ???+0x0
 20  ffc23ba0  f1336ed1  CServerGameDLL::GameFrame(bool)+0x161
 21  ffc23c20  eaf17334  ???+0x0
 22  ffc23c70  dd112fbb  ???+0x0
 23  ffc23c90  f5f1acb7  SV_Think(bool)+0x1b7
 24  ffc23d10  f5f1beb9  SV_Frame(bool)+0x179
 25  ffc23d80  f5e8e5a7  _Host_RunFrame_Server(bool)+0x177
 26  ffc23e10  f5e8f2b9  _Host_RunFrame(float)+0x449
*/


namespace Mod::Debug::Flamethrower_Mojo
{
	Color RainbowGenerator()
	{
		static long i = 0;
		
		switch ((i++) % 8) {
		case 0:  return Color(0xff, 0x00, 0x00, 0xff); // red
		case 1:  return Color(0xff, 0x80, 0x00, 0xff); // orange
		case 2:  return Color(0xff, 0xff, 0x00, 0xff); // yellow
		case 3:  return Color(0x00, 0xff, 0x00, 0xff); // green
		case 4:  return Color(0x00, 0xff, 0xff, 0xff); // cyan
		case 5:  return Color(0x00, 0x00, 0xff, 0xff); // blue
		case 6:  return Color(0x80, 0x00, 0xff, 0xff); // violet
		case 7:  return Color(0xff, 0x00, 0xff, 0xff); // magenta
		default: return Color(0x00, 0x00, 0x00, 0x00); // black
		}
	}
	
	Color MakeColorLighter(Color c)
	{
		c[0] += (0xff - c[0]) / 2;
		c[1] += (0xff - c[1]) / 2;
		c[2] += (0xff - c[2]) / 2;
		
		return c;
	}
	
	
	struct FlameInfo
	{
		FlameInfo(CTFFlameEntity *flame)
		{
			init_origin   = flame->GetAbsOrigin();
			init_curtime  = gpGlobals->curtime;
			init_realtime = Plat_FloatTime();
			init_tick     = gpGlobals->tickcount;
			
			col    = RainbowGenerator();
			col_lt = MakeColorLighter(col);
			
			num_thinks    = 0;
			missed_thinks = 0;
			
			hit_an_entity = false;
			
			spawned_this_tick = true;
			thought_this_tick = false;
			removed_this_tick = false;
		}
		
		FlameInfo(const FlameInfo&) = delete;
		
		
		Vector init_origin;
		float  init_curtime;
		float  init_realtime;
		int    init_tick;
		
		Color col;
		Color col_lt;
		
		int num_thinks;
		int missed_thinks;
		
		bool hit_an_entity;
		
		bool spawned_this_tick;
		bool thought_this_tick;
		bool removed_this_tick;
	};
	std::map<CHandle<CBaseEntity>, FlameInfo> flames;
	std::vector<decltype(flames)::iterator> dead_flames;
	
	
	RefCount rc_CTFFlameEntity_Create;
	RefCount rc_CTFFlameEntity_FlameThink;
	RefCount rc_CTFFlameEntity_OnCollide;
	
	
	std::deque<double> stats_dist;
	
	std::deque<int> stats_thinks_total;
	std::deque<int> stats_thinks_missed;
	
	std::deque<bool> stats_hit;
	
	
	ConVar cvar_fix("sig_debug_flamethrower_mojo_fix", "0", FCVAR_NOTIFY,
		"Debug: apply a fix for flame entities being created/spawned in the wrong timespace");
	
	
	float saved_curtime;
	void BeginTimeFix()
	{
		if (cvar_fix.GetBool()) {
			Warning("Fix is enabled but it doesn't actually quite work correctly!\n");
			saved_curtime = gpGlobals->curtime;
			gpGlobals->curtime = engine->GetServerTime();
		}
	}
	void EndTimeFix()
	{
		if (cvar_fix.GetBool()) {
			gpGlobals->curtime = saved_curtime;
		}
	}
	
	
	DETOUR_DECL_MEMBER(Vector, CTFFlameThrower_GetFlameOriginPos)
	{
		auto result = DETOUR_MEMBER_CALL();
		
	//	DevMsg("[CTFFlameThrower::GetFlameOriginPos]  [%+6.1f %+6.1f %+6.1f]\n",
	//		result.x, result.y, result.z);
		
		return result;
	}
	
	
	DETOUR_DECL_STATIC(CTFFlameEntity *, CTFFlameEntity_Create, const Vector& origin, const QAngle& angles, CBaseEntity *owner, float f1, int i1, float f2, bool b1, bool b2)
	{
		BeginTimeFix();
		
		SCOPED_INCREMENT(rc_CTFFlameEntity_Create);
		
		auto result = DETOUR_STATIC_CALL(origin, angles, owner, f1, i1, f2, b1, b2);
		
		if (result != nullptr) {
		//	DevMsg("[CTFFlameEntity::Create]  [%+6.1f %+6.1f %+6.1f]\n",
		//		result->GetAbsOrigin().x, result->GetAbsOrigin().y, result->GetAbsOrigin().z);
			
			flames.emplace(std::make_pair(result, result));
			
		//	Vector velocity;
		//	result->GetVelocity(&velocity, nullptr);
		//	DevMsg("New flame entity speed: %.0f\n", velocity.Length());
		//	NDebugOverlay::EntityTextAtPosition(result->GetAbsOrigin(), 0, CFmtStrN<64>("%.0f", velocity.Length()), 1.0f, 0xff, 0xff, 0xff, 0xff);
		}
		
	//	BACKTRACE();
		
	//	DevMsg("CTFFlameEntity::Create: curtime = %.3f, tickcount = %d, realtime = %.3f\n", gpGlobals->curtime, gpGlobals->tickcount, gpGlobals->realtime);
		
		EndTimeFix();
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_Spawn)
	{
		DETOUR_MEMBER_CALL();
		
	//	BACKTRACE();
		
	//	DevMsg("CTFFlameEntity::Spawn: curtime = %.3f, tickcount = %d, realtime = %.3f\n", gpGlobals->curtime, gpGlobals->tickcount, gpGlobals->realtime);
	}
	
	
	ConVar cvar_stats_qty("sig_debug_flamethrower_mojo_stats_qty", "22", FCVAR_NOTIFY,
		"Debug: number of recent flame distances to keep track of for statistics");
	
	
	ConVar cvar_dead_flame_duration("sig_debug_flamethrower_mojo_dead_flame_duration", "1.0", FCVAR_NOTIFY,
		"Debug: How long to show boxes for flame entities after they've been removed");
	
	
	// reasons a flame may be removed:
	// - in OnCollide, if (enginetrace->GetPointContents(flame->GetAbsOrigin()) & MASK_WATER) != 0
	// - in FlameThink, if the time-to-live is exceeded
	// - in FlameThink, if forward trace from m_vecPrevPos to current pos with MASK_SOLID hits something (e.g. a wall)
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_RemoveFlame)
	{
		auto flame = reinterpret_cast<CTFFlameEntity *>(this);
		
		auto it = flames.find(flame);
		if (it != flames.end()) {
			FlameInfo& info = (*it).second;
			
			const Vector& pos_start = info.init_origin;
			const Vector& pos_end   = flame->GetAbsOrigin();
			float dist = (pos_end - pos_start).Length();
			
			int delta_tick       = gpGlobals->tickcount - info.init_tick;
			float delta_curtime  = gpGlobals->curtime   - info.init_curtime;
			float delta_realtime = Plat_FloatTime()     - info.init_realtime;
			
		//	NDebugOverlay::EntityTextAtPosition(pos_end, 0, CFmtStrN<64>("%.0f", dist), cvar_dead_flame_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff);
		//	NDebugOverlay::EntityTextAtPosition(pos_end, 0, CFmtStrN<64>("%dt", delta_tick), cvar_dead_flame_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff);
		//	NDebugOverlay::EntityTextAtPosition(pos_end, 1, CFmtStrN<64>("%.3fs", delta_realtime), cvar_dead_flame_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			
			NDebugOverlay::EntityBounds(flame, 0xff, 0xff, 0xff, 0x10, cvar_dead_flame_duration.GetFloat());
		//	NDebugOverlay::Cross3D(flame->WorldSpaceCenter(), 3.0f, 0xff, 0xff, 0xff, false, cvar_dead_flame_duration.GetFloat());
			
			stats_dist.push_back(dist);
			while (stats_dist.size() > (size_t)cvar_stats_qty.GetInt()) {
				stats_dist.pop_front();
			}
			
			stats_thinks_total.push_back(info.num_thinks + info.missed_thinks);
			while (stats_thinks_total.size() > (size_t)cvar_stats_qty.GetInt()) {
				stats_thinks_total.pop_front();
			}
			stats_thinks_missed.push_back(info.missed_thinks);
			while (stats_thinks_missed.size() > (size_t)cvar_stats_qty.GetInt()) {
				stats_thinks_missed.pop_front();
			}
			
			stats_hit.push_back(info.hit_an_entity);
		//	while (stats_hit.size() > (size_t)cvar_stats_qty.GetInt()) {
		//		stats_hit.pop_front();
		//	}
			
			info.removed_this_tick = true;
			dead_flames.push_back(it);
		}
		
		//	DevMsg("[CTFFlameEntity::RemoveFlame]  [%+6.1f %+6.1f %+6.1f]\n",
		//		flame->GetAbsOrigin().x, flame->GetAbsOrigin().y, flame->GetAbsOrigin().z);
		
		DETOUR_MEMBER_CALL();
	}
	
	
	ConVar cvar_backtrace_think("sig_debug_flamethrower_mojo_backtrace_think", "0", FCVAR_NOTIFY,
		"Debug: enable backtrace dumping in CTFFlameEntity::FlameThink");
	
	
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_FlameThink)
	{
		auto flame = reinterpret_cast<CTFFlameEntity *>(this);
		
		TRACE("@%.3f #%d", gpGlobals->curtime, ENTINDEX(flame));
		
		SCOPED_INCREMENT(rc_CTFFlameEntity_FlameThink);
		
		if (cvar_backtrace_think.GetBool()) {
			BACKTRACE();
		}
		
		auto it = flames.find(flame);
		if (it != flames.end()) {
			FlameInfo& info = (*it).second;
			
			info.thought_this_tick = true;
			int num_thinks = info.num_thinks++;
		}
		
		DETOUR_MEMBER_CALL();
	}
	
	
	ConVar cvar_show_traces("sig_debug_flamethrower_mojo_show_traces", "0", FCVAR_NOTIFY,
		"Debug: enable debug overlays for IEngineTrace::TraceRay");
	
	
	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
		DETOUR_MEMBER_CALL(ray, fMask, pTraceFilter, pTrace);
		
		if (cvar_show_traces.GetBool() && rc_CTFFlameEntity_FlameThink > 0 && rc_CTFFlameEntity_OnCollide <= 0 && fMask == MASK_SOLID) {
			auto m_pPassEnt = *reinterpret_cast<IHandleEntity **>((uintptr_t)pTraceFilter + 0x4);
			
			auto flame = rtti_cast<CTFFlameEntity *>(m_pPassEnt);
			if (flame != nullptr) {
				auto it = flames.find(flame);
				if (it != flames.end()) {
					FlameInfo& info = (*it).second;
					
					/* this is the condition that FlameThink uses to check if the flame hit a wall;
					 * if true, it skips collision detection and immediately calls RemoveFlame */
					if (pTrace->startsolid || pTrace->fraction < 1.0f) {
						Vector vecDir = (pTrace->endpos - pTrace->startpos);
						VectorNormalize(vecDir);
						
						Vector better_endpos = pTrace->endpos + (vecDir * flame->CollisionProp()->OBBMaxs().x);
						
						NDebugOverlay::EntityTextAtPosition(better_endpos, -2, "HIT WALL", 0.10f, 0xff, 0xff, 0xff, 0xff);
						NDebugOverlay::Line(pTrace->startpos, better_endpos, 0xff, 0xff, 0xff, true, 0.10f);
						NDebugOverlay::Sphere(better_endpos, vec3_angle, 2.0f, 0xff, 0xff, 0xff, 0x80, true, 0.10f);
					} else {
						NDebugOverlay::Line(pTrace->startpos, pTrace->endpos, info.col.r(), info.col.g(), info.col.b(), true, gpGlobals->interval_per_tick);
					}
				}
			}
		}
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_OnCollide, CBaseEntity *pOther)
	{
		SCOPED_INCREMENT(rc_CTFFlameEntity_OnCollide);
		
		auto flame = reinterpret_cast<CTFFlameEntity *>(this);
		
		auto it = flames.find(flame);
		if (it != flames.end()) {
			FlameInfo& info = (*it).second;
			
			info.hit_an_entity = true;
			
			NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), -2, "HIT", 0.10f, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
		}
		
		DETOUR_MEMBER_CALL(pOther);
	}
	
	
	DETOUR_DECL_MEMBER(void, CBaseProjectile_CollideWithTeammatesThink)
	{
		auto proj = reinterpret_cast<CBaseProjectile *>(this);
		
		Vector vecText        = proj->WorldSpaceCenter() + Vector(-10.0f, 0.0f, 45.0f);
		Vector vecArrowTop    = proj->WorldSpaceCenter() + Vector(  0.0f, 0.0f, 40.0f);
		Vector vecArrowBottom = proj->WorldSpaceCenter() + Vector(  0.0f, 0.0f,  3.0f);
		
		NDebugOverlay::EntityTextAtPosition(vecText, 0, "CollideWithTeammatesThink", 1.00f, 0xff, 0xff, 0xff, 0xff);
		NDebugOverlay::VertArrow(vecArrowTop, vecArrowBottom, 1.0f, 0xff, 0xff, 0xff, 0x00, true, 1.00f);
		NDebugOverlay::EntityBounds(proj, 0xff, 0xff, 0xff, 0x10, 1.00f);
		
		DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_STATIC(int, D_RandomInt, int iMinVal, int iMaxVal)
	{
		auto result = DETOUR_STATIC_CALL(iMinVal, iMaxVal);
		
		if (rc_CTFFlameEntity_Create > 0) {
			DevMsg("RandomInt(%d, %d) = %d\n", iMinVal, iMaxVal, result);
		}
		
		return result;
	}
	
	
	DETOUR_DECL_MEMBER(void, CLagCompensationManager_StartLagCompensation, CBasePlayer *player, CUserCmd *cmd)
	{
		/* do nothing */
	}
	
	DETOUR_DECL_MEMBER(void, CLagCompensationManager_FinishLagCompensation, CBasePlayer *player)
	{
		/* do nothing */
	}
	
	
	////////////////////////////////////////////////////////////////////////////
	// BEGIN ENTITY SIMULATION / THINK TRACING /////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	
	DETOUR_DECL_MEMBER(void, IServerGameDLL_GameFrame, bool simulating)
	{
		TRACE("@%.3f", gpGlobals->curtime);
		DETOUR_MEMBER_CALL(simulating);
	}
	
	DETOUR_DECL_STATIC(void, Physics_RunThinkFunctions, bool simulating)
	{
		TRACE("@%.3f", gpGlobals->curtime);
		DETOUR_STATIC_CALL(simulating);
	}
	
	DETOUR_DECL_STATIC(void, Physics_SimulateEntity, CBaseEntity *pEntity)
	{
		auto flame = rtti_cast<CTFFlameEntity *>(pEntity);
		TRACE_IF(flame != nullptr, "@%.3f #%d", gpGlobals->curtime, ENTINDEX(flame));
		DETOUR_STATIC_CALL(pEntity);
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_PhysicsSimulate)
	{
		auto flame = rtti_cast<CTFFlameEntity *>(reinterpret_cast<CBaseEntity *>(this));
		TRACE_IF(flame != nullptr, "@%.3f #%d", gpGlobals->curtime, ENTINDEX(flame));
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_PhysicsNoclip)
	{
		auto flame = rtti_cast<CTFFlameEntity *>(reinterpret_cast<CBaseEntity *>(this));
		TRACE_IF(flame != nullptr, "@%.3f #%d", gpGlobals->curtime, ENTINDEX(flame));
		DETOUR_MEMBER_CALL();
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_PhysicsRunThink, thinkmethods_t thinkMethod)
	{
		auto flame = rtti_cast<CTFFlameEntity *>(reinterpret_cast<CBaseEntity *>(this));
		TRACE_IF(flame != nullptr, "@%.3f #%d [thinkMethod: %d]", gpGlobals->curtime, ENTINDEX(flame), (int)thinkMethod);
		DETOUR_MEMBER_CALL(thinkMethod);
	}
	
	DETOUR_DECL_MEMBER(bool, CBaseEntity_PhysicsRunSpecificThink, int nContextIndex, BASEPTR thinkFunc)
	{
		auto flame = rtti_cast<CTFFlameEntity *>(reinterpret_cast<CBaseEntity *>(this));
		TRACE_IF(flame != nullptr, "@%.3f #%d [nContextIndex: %d] [thinkFunc: %08x]", gpGlobals->curtime, ENTINDEX(flame), nContextIndex, *reinterpret_cast<uintptr_t *>(&thinkFunc));
		auto result = DETOUR_MEMBER_CALL(nContextIndex, thinkFunc);
		TRACE_EXIT("%s", (result ? "TRUE" : "FALSE"));
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CBaseEntity_PhysicsDispatchThink, BASEPTR thinkFunc)
	{
		auto flame = rtti_cast<CTFFlameEntity *>(reinterpret_cast<CBaseEntity *>(this));
		TRACE_IF(flame != nullptr, "@%.3f #%d [thinkFunc: %08x]", gpGlobals->curtime, ENTINDEX(flame), *reinterpret_cast<uintptr_t *>(&thinkFunc));
		DETOUR_MEMBER_CALL(thinkFunc);
	}
	
	////////////////////////////////////////////////////////////////////////////
	// END ENTITY SIMULATION / THINK TRACING ///////////////////////////////////
	////////////////////////////////////////////////////////////////////////////
	
	
	ConVar cvar_stats_interval("sig_debug_flamethrower_mojo_stats_interval", "0.25", FCVAR_NOTIFY,
		"Debug: how often to refresh the flame distance statistics overlay");
	ConVar cvar_stats_screenpos_x("sig_debug_flamethrower_mojo_stats_screenpos_x", "0.77", FCVAR_NOTIFY,
		"Debug: where to put the statistics overlay");
	ConVar cvar_stats_screenpos_y("sig_debug_flamethrower_mojo_stats_screenpos_y", "0.05", FCVAR_NOTIFY,
		"Debug: where to put the statistics overlay");
	
	void DrawStatTextLine(int line, const char *text, int r = 0xff, int g = 0xff, int b = 0xff, int a = 0xff)
	{
		NDebugOverlay::ScreenText(cvar_stats_screenpos_x.GetFloat(), cvar_stats_screenpos_y.GetFloat() + (line * 0.0175f),
			text, r, g, b, a, cvar_stats_interval.GetFloat());
	}
	
	
	ConVar cvar_net_interval("sig_debug_flamethrower_mojo_net_interval", "0.25", FCVAR_NOTIFY,
		"Debug: how often to refresh the network overlay");
	ConVar cvar_net_screenpos_x("sig_debug_flamethrower_mojo_net_screenpos_x", "0.55", FCVAR_NOTIFY,
		"Debug: where to put the network overlay");
	ConVar cvar_net_screenpos_y("sig_debug_flamethrower_mojo_net_screenpos_y", "0.05", FCVAR_NOTIFY,
		"Debug: where to put the network overlay");
	
	void DrawNetTextLine(int line, const char *text, int r = 0xff, int g = 0xff, int b = 0xff, int a = 0xff)
	{
		NDebugOverlay::ScreenText(cvar_net_screenpos_x.GetFloat(), cvar_net_screenpos_y.GetFloat() + (line * 0.0175f),
			text, r, g, b, a, cvar_net_interval.GetFloat());
	}
	
	
	ConVar cvar_param_interval("sig_debug_flamethrower_mojo_param_interval", "0.25", FCVAR_NOTIFY,
		"Debug: how often to refresh the flamethrower parameter overlay");
	ConVar cvar_param_screenpos_x("sig_debug_flamethrower_mojo_param_screenpos_x", "0.03", FCVAR_NOTIFY,
		"Debug: where to put the flamethrower parameter overlay");
	ConVar cvar_param_screenpos_y("sig_debug_flamethrower_mojo_param_screenpos_y", "0.05", FCVAR_NOTIFY,
		"Debug: where to put the flamethrower parameter overlay");
	
	void DrawParamTextLine(int line, const char *text, int r = 0xff, int g = 0xff, int b = 0xff, int a = 0xff)
	{
		NDebugOverlay::ScreenText(cvar_param_screenpos_x.GetFloat(), cvar_param_screenpos_y.GetFloat() + (line * 0.0175f),
			text, r, g, b, a, cvar_param_interval.GetFloat());
	}
	void DrawParamConVarLine(int line, ConVarRef& ref)
	{
		const char *help_text = "";
		ConVar *pConVar = dynamic_cast<ConVar *>(ref.GetLinkedConVar());
		if (pConVar != nullptr) {
			help_text = pConVar->GetHelpText();
		}
		
		CFmtStrN<256> str("%-33s = %7.2f  (\"%s\")", ref.GetName(), ref.GetFloat(), help_text);
		
		if (strcmp(ref.GetDefault(), ref.GetString()) != 0) {
			DrawParamTextLine(line, str, 0xff, 0xff, 0x00, 0xff);
		} else {
			DrawParamTextLine(line, str);
		}
	}
	
	
	double ComputeStdDev(double mean)
	{
		if (stats_dist.size() < 2) return 0.0;
		
		double accum = 0.0;
		
		std::for_each(stats_dist.begin(), stats_dist.end(), [&](const double dist){
			accum += Square(dist - mean);
		});
		
		return sqrt(accum / (stats_dist.size() - 1));
	}
	
	
	void ComputeQuartiles(double& min, double& q1, double& median, double& q3, double& max)
	{
		if (stats_dist.empty()) {
			min    = 0.0;
			q1     = 0.0;
			median = 0.0;
			q3     = 0.0;
			max    = 0.0;
			return;
		}
		
		auto n_q1 = stats_dist.size() / 4;
		auto n_q2 = stats_dist.size() / 2;
		auto n_q3 = n_q1 + n_q2;
		
		n_q1 = Clamp(n_q1, 0U, stats_dist.size());
		n_q2 = Clamp(n_q2, 0U, stats_dist.size());
		n_q3 = Clamp(n_q3, 0U, stats_dist.size());
		
		std::vector<double> sorted(stats_dist.begin(), stats_dist.end());
		std::sort(sorted.begin(), sorted.end());
		
		q1     = sorted[n_q1];
		median = sorted[n_q2];
		q3     = sorted[n_q3];
		
		min = sorted.front();
		max = sorted.back();
	}
	
	
	ConVar cvar_show_thinks("sig_debug_flamethrower_mojo_show_thinks", "0", FCVAR_NOTIFY,
		"Debug: show think overlays on active flame entities");
	
	
	void PostThink_DrawFlames()
	{
		for (int i = 0; i < ITFFlameEntityAutoList::AutoList().Count(); ++i) {
			auto flame = rtti_cast<CTFFlameEntity *>(ITFFlameEntityAutoList::AutoList()[i]);
			if (flame == nullptr) continue;
			
			auto it = flames.find(flame);
			if (it != flames.end()) {
				FlameInfo& info = (*it).second;
				
				NDebugOverlay::EntityBounds(flame, info.col.r(), info.col.g(), info.col.b(), 0x10, gpGlobals->interval_per_tick);
			//	NDebugOverlay::Cross3D(flame->WorldSpaceCenter(), 3.0f, 0xff, 0xff, 0xff, false, gpGlobals->interval_per_tick);
				
				if (cvar_show_thinks.GetBool()) {
					if (info.thought_this_tick) {
						NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), -1, "THINK", gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
						NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(),  0, CFmtStrN<64>("#%d", info.num_thinks), gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
					} else {
						if (info.spawned_this_tick) {
							NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), -1, "SPAWN", gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
						} else if (info.removed_this_tick) {
							NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), -1, "DEAD", gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
						} else {
							++info.missed_thinks;
							
							NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), -1, "MISSED", gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
							NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(),  0, "THINK!", gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
						}
					}
					
					if (info.missed_thinks != 0) {
						NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), 5, CFmtStrN<64>("%d MISSED", info.missed_thinks), gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
						NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), 6, "THINKS!!", gpGlobals->interval_per_tick, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
					}
				}
			}
		}
		
		ForEachEntityByRTTI<CTFProjectile_HealingBolt>([](CTFProjectile_HealingBolt *bolt){
			NDebugOverlay::EntityBounds(bolt, 0xff, 0xff, 0xff, 0x10, gpGlobals->interval_per_tick);
			
			/* de-bloat the box slightly so it won't interfere with the CollideWithTeammatesThink box */
			Vector vecMins = bolt->CollisionProp()->OBBMins() + Vector(0.01f, 0.01f, 0.01f);
			Vector vecMaxs = bolt->CollisionProp()->OBBMaxs() - Vector(0.01f, 0.01f, 0.01f);
			
			NDebugOverlay::Box(bolt->WorldSpaceCenter(), vecMins, vecMaxs, 0x40, 0x40, 0x40, 0x00, 1.00f);
		});
	}
	
	void PostThink_PyroOverlay()
	{
		static CountdownTimer ctPyroOverlay;
		if (ctPyroOverlay.IsElapsed()) {
			ctPyroOverlay.Start(1.00f);
			
			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				CBasePlayer *player = UTIL_PlayerByIndex(i);
				if (player == nullptr) continue;
				
				NDebugOverlay::EntityBounds(player, 0xff, 0xff, 0xff, 0x00, 1.00f);
				
				if (!player->IsBot()) {
					Vector vecFwd;
					player->EyeVectors(&vecFwd);
					Vector vecStart = player->EyePosition() + Vector(0.0f, 0.0f, 50.0f);
					
				//	NDebugOverlay::Line(vecStart, vecStart + (500.0f * vecFwd), 0x80, 0x80, 0x80, true, 1.00f);
					
					NDebugOverlay::EntityTextAtPosition(vecStart + (-30.0f * vecFwd) + Vector(0.0f, 0.0f, 15.0f), 0, "HAMMER", 1.00f, 0xc0, 0xc0, 0xc0, 0xff);
					NDebugOverlay::EntityTextAtPosition(vecStart + (-30.0f * vecFwd) + Vector(0.0f, 0.0f, 15.0f), 1, " UNITS", 1.00f, 0xc0, 0xc0, 0xc0, 0xff);
				
					for (float x = 0.0f; x <= 500.0f; x += 20.0f) {
						Vector vecHash1 = vecStart + (x * vecFwd) + Vector(0.0f, 0.0f,  10.0f);
						Vector vecHash2 = vecStart + (x * vecFwd) + Vector(0.0f, 0.0f, -50.0f);
						Vector vecHash3 = vecStart + ((x - 3.0f) * vecFwd) + Vector(0.0f, 0.0f,  15.0f);
						
						NDebugOverlay::Line(vecHash1, vecHash2, 0x80, 0x80, 0x80, true, 1.00f);
						NDebugOverlay::EntityTextAtPosition(vecHash3, 0, CFmtStrN<64>("%.0f", x), 1.00f, 0xc0, 0xc0, 0xc0, 0xff);
					}
				}
			}
		}
	}
	
	void PostThink_MedicOverlay()
	{
		static CountdownTimer ctMedicOverlay;
		if (ctMedicOverlay.IsElapsed()) {
			ctMedicOverlay.Start(0.100f);
			
			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(i));
				if (player == nullptr) return;
				
				if (player->IsBot()) {
					Vector vecAbove = player->GetAbsOrigin() + Vector(0.0f, 0.0f, 93.0f);
					
					float hp_ratio = (float)player->GetHealth() / (float)player->GetMaxHealth();
					
					int r = RemapValClamped(hp_ratio, 0.50f, 1.00f, 255.0f,   0.0f);
					int g = RemapValClamped(hp_ratio, 0.00f, 0.50f,   0.0f, 255.0f);
					
					NDebugOverlay::EntityTextAtPosition(vecAbove, 0, CFmtStrN<64>("%s", player->GetPlayerName()), 0.100f, 0xff, 0xff, 0xff, 0xff);
					NDebugOverlay::EntityTextAtPosition(vecAbove, 2, CFmtStrN<64>("%3d HP", player->GetHealth()), 0.100f, r, g, 0x00, 0xff);
				}
			}
		}
	}
	
	void PostThink_StatOverlay()
	{
		static CountdownTimer ctStatOverlay;
		if (ctStatOverlay.IsElapsed()) {
			ctStatOverlay.Start(cvar_stats_interval.GetFloat());
			
#if 0
			double sum    = std::accumulate(stats_dist.begin(), stats_dist.end(), 0.0);
			double mean   = sum / (double)stats_dist.size();
			double stddev = ComputeStdDev(mean);
			
			if (stats_dist.size() == 0) mean = 0.0;
			
			DrawStatTextLine( 0, CFmtStrN<256>("DISTANCE STATISTICS FOR THE LAST %u FLAMES:", stats_dist.size()));
			
			DrawStatTextLine( 2, CFmtStrN<256>("  Mean: %4.0f HU", mean));
			DrawStatTextLine( 3, CFmtStrN<256>("StdDev: %4.0f HU", stddev));
			
			double min, q1, median, q3, max;
			ComputeQuartiles(min, q1, median, q3, max);
			
			DrawStatTextLine( 5, CFmtStrN<256>("   Max: %4.0f HU", max));
			DrawStatTextLine( 6, CFmtStrN<256>("    Q3: %4.0f HU", q3));
			DrawStatTextLine( 7, CFmtStrN<256>("Median: %4.0f HU", median));
			DrawStatTextLine( 8, CFmtStrN<256>("    Q1: %4.0f HU", q1));
			DrawStatTextLine( 9, CFmtStrN<256>("   Min: %4.0f HU", min));
			
			float range = max - min;
			
			DrawStatTextLine(11, CFmtStrN<256>(" Range: %4.0f HU  (max-min)", range));
#endif
			
			// let's draw hit ratio stats instead of distance stats, shall we?
			
			int n_hit   = 0;
			int n_miss  = 0;
			int n_total = 0;
			std::for_each(stats_hit.begin(), stats_hit.end(), [&](const bool hit){
				if (hit) ++n_hit; else ++n_miss;
				++n_total;
			});
			
			double r_hit  = (double)n_hit  / (double)n_total;
			double r_miss = (double)n_miss / (double)n_total;
			
			Color c_hit (0xc0, 0xc0, 0xc0, 0xff);
			Color c_miss(0xc0, 0xc0, 0xc0, 0xff);
			
			if (n_total == 0) {
				r_hit  = 0.0;
				r_miss = 0.0;
			} else {
				// hit: @100% green; @75% yellow; @50% red
				c_hit[0] = (uint8_t)RemapValClamped(r_hit, 0.75, 1.00, 255.0,   0.0);
				c_hit[1] = (uint8_t)RemapValClamped(r_hit, 0.50, 0.75,   0.0, 255.0);
				c_hit[2] = 0x00;
				
				// miss: @50% red; @25% yellow; @0% green
				c_miss[0] = (uint8_t)RemapValClamped(r_miss, 0.00, 0.25,   0.0, 255.0);
				c_miss[1] = (uint8_t)RemapValClamped(r_miss, 0.25, 0.50, 255.0,   0.0);
				c_miss[2] = 0x00;
			}
			
			DrawStatTextLine(0, "FLAME HIT STATISTICS:");
			
			DrawStatTextLine(2, "        FLAMES  PERCENT");
			DrawStatTextLine(3, CFmtStrN<256>("TOTAL:  %6u", stats_hit.size()));
			DrawStatTextLine(4, CFmtStrN<256>("HITS:   %6u    %3.0f %%", n_hit, r_hit  * 100.0f), c_hit.r(),  c_hit.g(),  c_hit.b(),  0xff);
			DrawStatTextLine(5, CFmtStrN<256>("MISSES: %6u    %3.0f %%", n_miss, r_miss * 100.0f), c_miss.r(), c_miss.g(), c_miss.b(), 0xff);
		}
	}
	
	void PostThink_NetOverlay()
	{
		static CountdownTimer ctNetOverlay;
		if (ctNetOverlay.IsElapsed()) {
			ctNetOverlay.Start(cvar_net_interval.GetFloat());
			
			static ConVarRef net_fakelag             ("net_fakelag");
			static ConVarRef net_fakejitter          ("net_fakejitter");
			static ConVarRef sv_clockcorrection_msecs("sv_clockcorrection_msecs");
			static ConVarRef host_timescale          ("host_timescale");
			
			DrawNetTextLine(0, "SERVER NETWORK SETTINGS:");
			DrawNetTextLine(1, "(this is NOT a listen server!)");
			
			DrawNetTextLine(3, CFmtStrN<256>("net_fakelag    = %3.0f", net_fakelag.GetFloat()));
			DrawNetTextLine(4, CFmtStrN<256>("net_fakejitter = %3.0f", net_fakejitter.GetFloat()));
			
			float lag    = net_fakelag.GetFloat();
			float jitter = Min(lag * 0.5f, net_fakejitter.GetFloat());
			
			DrawNetTextLine(6, "EFFECTIVE LAG:");
			DrawNetTextLine(7, CFmtStrN<256>("%.0f +/- %.0f milliseconds", lag, jitter));
			
			DrawNetTextLine(10, CFmtStrN<256>("sv_clockcorrection_msecs = %3d", sv_clockcorrection_msecs.GetInt()));
			
			DrawNetTextLine(13, CFmtStrN<256>("host_timescale = %.3f", host_timescale.GetFloat()));
		}
	}
	
	void PostThink_ParamOverlay()
	{
		static CountdownTimer ctParamOverlay;
		if (ctParamOverlay.IsElapsed()) {
			ctParamOverlay.Start(cvar_param_interval.GetFloat());
			
			static ConVarRef tf_flamethrower_boxsize          ("tf_flamethrower_boxsize");
			static ConVarRef tf_flamethrower_flametime        ("tf_flamethrower_flametime");
			
			static ConVarRef tf_flamethrower_drag             ("tf_flamethrower_drag");
			static ConVarRef tf_flamethrower_float            ("tf_flamethrower_float");
			
			static ConVarRef tf_flamethrower_velocity         ("tf_flamethrower_velocity");
			static ConVarRef tf_flamethrower_vecrand          ("tf_flamethrower_vecrand");
			static ConVarRef tf_flamethrower_velocityfadestart("tf_flamethrower_velocityfadestart");
			static ConVarRef tf_flamethrower_velocityfadeend  ("tf_flamethrower_velocityfadeend");
			
			DrawParamTextLine(0, "FLAMETHROWER CONVAR SETTINGS:");
			
			DrawParamConVarLine( 2, tf_flamethrower_boxsize);
			DrawParamConVarLine( 3, tf_flamethrower_flametime);
			
			DrawParamConVarLine( 5, tf_flamethrower_drag);
			DrawParamConVarLine( 6, tf_flamethrower_float);
			
			DrawParamConVarLine( 8, tf_flamethrower_velocity);
			DrawParamConVarLine( 9, tf_flamethrower_vecrand);
			DrawParamConVarLine(10, tf_flamethrower_velocityfadestart);
			DrawParamConVarLine(11, tf_flamethrower_velocityfadeend);
		}
	}
	
	void PostThink_RefillAmmo()
	{
		static CountdownTimer ctRefillAmmo;
		if (ctRefillAmmo.IsElapsed()) {
			ctRefillAmmo.Start(1.0f);
			
			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				CBasePlayer *player = UTIL_PlayerByIndex(i);
				if (player == nullptr) continue;
				
				for (int j = 0; j < MAX_AMMO_SLOTS; ++j) {
					player->GiveAmmo(1000, j, true);
				}
			}
		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Flamethrower_Mojo")
		{
			MOD_ADD_DETOUR_MEMBER(CTFFlameThrower_GetFlameOriginPos, "CTFFlameThrower::GetFlameOriginPos");
			
			MOD_ADD_DETOUR_STATIC(CTFFlameEntity_Create, "CTFFlameEntity::Create");
			
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_Spawn, "CTFFlameEntity::Spawn");
			
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_RemoveFlame, "CTFFlameEntity::RemoveFlame");
			
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_FlameThink, "CTFFlameEntity::FlameThink");
			
			MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay, "IEngineTrace::TraceRay");
			
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_OnCollide, "CTFFlameEntity::OnCollide");
			
			MOD_ADD_DETOUR_MEMBER(CBaseProjectile_CollideWithTeammatesThink, "CBaseProjectile::CollideWithTeammatesThink");
			
		//	this->AddDetour(new CDetour("RandomInt", reinterpret_cast<void *>(&RandomInt), GET_STATIC_CALLBACK(D_RandomInt), GET_STATIC_INNERPTR(D_RandomInt)));
			
		//	MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_StartLagCompensation,  "CLagCompensationManager::StartLagCompensation");
		//	MOD_ADD_DETOUR_MEMBER(CLagCompensationManager_FinishLagCompensation, "CLagCompensationManager::FinishLagCompensation");
			
			MOD_ADD_DETOUR_MEMBER(IServerGameDLL_GameFrame,            "IServerGameDLL::GameFrame");
			MOD_ADD_DETOUR_STATIC(Physics_RunThinkFunctions,           "Physics_RunThinkFunctions");
			MOD_ADD_DETOUR_STATIC(Physics_SimulateEntity,              "Physics_SimulateEntity");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PhysicsSimulate,         "CBaseEntity::PhysicsSimulate");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PhysicsNoclip,           "CBaseEntity::PhysicsNoclip");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PhysicsRunThink,         "CBaseEntity::PhysicsRunThink");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PhysicsRunSpecificThink, "CBaseEntity::PhysicsRunSpecificThink");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_PhysicsDispatchThink,    "CBaseEntity::PhysicsDispatchThink");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			for (auto& pair : flames) {
				pair.second.spawned_this_tick = false;
				pair.second.thought_this_tick = false;
			}
			
		//	DevMsg("\nPreEntityThink:  curtime = %.3f, tickcount = %d, realtime = %.3f\n", gpGlobals->curtime, gpGlobals->tickcount, gpGlobals->realtime);
		}
		
		virtual void FrameUpdatePostEntityThink() override
		{
		//	DevMsg("PostEntityThink: curtime = %.3f, tickcount = %d, realtime = %.3f\n", gpGlobals->curtime, gpGlobals->tickcount, gpGlobals->realtime);
			
			PostThink_DrawFlames();
			PostThink_PyroOverlay();
			PostThink_MedicOverlay();
			PostThink_StatOverlay();
			PostThink_NetOverlay();
			PostThink_ParamOverlay();
			PostThink_RefillAmmo();
			
			for (auto it : dead_flames) {
				flames.erase(it);
			}
			dead_flames.clear();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_flamethrower_mojo", "0", FCVAR_NOTIFY,
		"Debug: investigate the 'flamethrower mojo' phenomenon",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
