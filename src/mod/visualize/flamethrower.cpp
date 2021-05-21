#include "mod.h"
#include "stub/entities.h"
#include "stub/tfplayer.h"


// adapted from Debug:Flamethrower_Mojo


namespace Mod::Visualize::Flamethrower
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
			
		//	col = RainbowGenerator();
		//	
		//	CBaseEntity *flamethrower = flame->GetOwnerEntity();
		//	if (flamethrower != nullptr) {
		//		CTFPlayer *player = ToTFPlayer(flamethrower->GetOwnerEntity());
		//		if (player != nullptr) {
		//			
		//			
		//			if (player->IsMiniBoss()) {
		//				
		//			} else {
		//				
		//			}
		//			
		//			
		//		}
		//	}
			col = Color(0xff, 0xff, 0xff, 0xff);
			
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
	
	
	DETOUR_DECL_STATIC(CTFFlameEntity *, CTFFlameEntity_Create, const Vector& origin, const QAngle& angles, CBaseEntity *owner, float f1, int i1, float f2, bool b1, bool b2)
	{
		auto result = DETOUR_STATIC_CALL(CTFFlameEntity_Create)(origin, angles, owner, f1, i1, f2, b1, b2);
		
		if (result != nullptr) {
			flames.emplace(std::make_pair(result, result));
		}
		
		return result;
	}
	
	
	ConVar cvar_dead_flame_duration("sig_visualize_flamethrower_dead_flame_duration", "1.0", FCVAR_NOTIFY,
		"Visualization: How long to show boxes for flame entities after they've been removed");
	
	
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
			
			info.removed_this_tick = true;
			dead_flames.push_back(it);
		}
		
		//	DevMsg("[CTFFlameEntity::RemoveFlame]  [%+6.1f %+6.1f %+6.1f]\n",
		//		flame->GetAbsOrigin().x, flame->GetAbsOrigin().y, flame->GetAbsOrigin().z);
		
		DETOUR_MEMBER_CALL(CTFFlameEntity_RemoveFlame)();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_FlameThink)
	{
		auto flame = reinterpret_cast<CTFFlameEntity *>(this);
		
		auto it = flames.find(flame);
		if (it != flames.end()) {
			FlameInfo& info = (*it).second;
			
			info.thought_this_tick = true;
			int num_thinks = info.num_thinks++;
		}
		
		DETOUR_MEMBER_CALL(CTFFlameEntity_FlameThink)();
	}
	
	
	DETOUR_DECL_MEMBER(void, CTFFlameEntity_OnCollide, CBaseEntity *pOther)
	{
		auto flame = reinterpret_cast<CTFFlameEntity *>(this);
		
		auto it = flames.find(flame);
		if (it != flames.end()) {
			FlameInfo& info = (*it).second;
			
			info.hit_an_entity = true;
			
		//	NDebugOverlay::EntityTextAtPosition(flame->WorldSpaceCenter(), -2, "HIT", 0.10f, info.col_lt.r(), info.col_lt.g(), info.col_lt.b(), 0xff);
		}
		
		DETOUR_MEMBER_CALL(CTFFlameEntity_OnCollide)(pOther);
	}
	
	
	ConVar cvar_show_thinks("sig_visualize_flamethrower_show_thinks", "0", FCVAR_NOTIFY,
		"Visualization: show think overlays on active flame entities");
	
	
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
	}
	
	void PostThink_PyroOverlay()
	{
		static CountdownTimer ctPyroOverlay;
		if (ctPyroOverlay.IsElapsed()) {
			ctPyroOverlay.Start(1.00f);
			
			for (int i = 1; i <= gpGlobals->maxClients; ++i) {
				CTFPlayer *player = ToTFPlayer(UTIL_PlayerByIndex(i));
				if (player == nullptr) continue;
				
				if (player->IsPlayerClass(TF_CLASS_PYRO)) {
					NDebugOverlay::EntityBounds(player, 0xff, 0xff, 0xff, 0x00, 1.00f);
					
					Vector vecFwd;
					player->EyeVectors(&vecFwd);
					Vector vecStart = player->EyePosition() + Vector(0.0f, 0.0f, 50.0f);
					
					vecFwd.z = 0.0f;
					vecFwd.NormalizeInPlace();
					
				//	NDebugOverlay::Line(vecStart, vecStart + (1000.0f * vecFwd), 0x80, 0x80, 0x80, true, 1.00f);
					
					NDebugOverlay::EntityTextAtPosition(vecStart + (-30.0f * vecFwd) + Vector(0.0f, 0.0f, 15.0f), 0, "HAMMER", 1.00f, 0xc0, 0xc0, 0xc0, 0xff);
					NDebugOverlay::EntityTextAtPosition(vecStart + (-30.0f * vecFwd) + Vector(0.0f, 0.0f, 15.0f), 1, " UNITS", 1.00f, 0xc0, 0xc0, 0xc0, 0xff);
				
					for (float x = 0.0f; x <= 1000.0f; x += 50.0f) {
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
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Flamethrower")
		{
			MOD_ADD_DETOUR_STATIC(CTFFlameEntity_Create, "CTFFlameEntity::Create");
			
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_RemoveFlame, "CTFFlameEntity::RemoveFlame");
			
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_FlameThink, "CTFFlameEntity::FlameThink");
			
			MOD_ADD_DETOUR_MEMBER(CTFFlameEntity_OnCollide, "CTFFlameEntity::OnCollide");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			for (auto& pair : flames) {
				pair.second.spawned_this_tick = false;
				pair.second.thought_this_tick = false;
			}
		}
		
		virtual void FrameUpdatePostEntityThink() override
		{
			PostThink_DrawFlames();
		//	PostThink_PyroOverlay();
			
			for (auto it : dead_flames) {
				flames.erase(it);
			}
			dead_flames.clear();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_flamethrower", "0", FCVAR_NOTIFY,
		"Visualization: show flame entity boxes, distances, etc.",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
