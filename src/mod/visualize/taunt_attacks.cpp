#include "mod.h"
#include "util/scope.h"
#include "stub/tfbot.h"
#include "stub/projectiles.h"
#include "stub/tf_shareddefs.h"


//  0 TAUNTATK_NONE
//  1 TAUNTATK_PYRO_HADOUKEN
//  2 TAUNTATK_HEAVY_EAT
//  3 TAUNTATK_HEAVY_RADIAL_BUFF
//  4 TAUNTATK_HEAVY_HIGH_NOON
//  5 TAUNTATK_SCOUT_DRINK
//  6 TAUNTATK_SCOUT_GRAND_SLAM
//  7 TAUNTATK_MEDIC_INHALE
//  8 TAUNTATK_SPY_FENCING_SLASH_A
//  9 TAUNTATK_SPY_FENCING_SLASH_B
//  a TAUNTATK_SPY_FENCING_STAB
//  b TAUNTATK_RPS_KILL
//  c TAUNTATK_SNIPER_ARROW_STAB_IMPALE
//  d TAUNTATK_SNIPER_ARROW_STAB_KILL
//  e TAUNTATK_SOLDIER_GRENADE_KILL
//  f TAUNTATK_DEMOMAN_BARBARIAN_SWING
// 10 TAUNTATK_MEDIC_UBERSLICE_IMPALE
// 11 TAUNTATK_MEDIC_UBERSLICE_KILL
// 12 TAUNTATK_FLIP_LAND_PARTICLE
// 13 TAUNTATK_RPS_PARTICLE
// 14 TAUNTATK_HIGHFIVE_PARTICLE
// 15 TAUNTATK_ENGINEER_GUITAR_SMASH
// 16 TAUNTATK_ENGINEER_ARM_IMPALE
// 17 TAUNTATK_ENGINEER_ARM_KILL
// 18 TAUNTATK_ENGINEER_ARM_BLEND
// 19 TAUNTATK_SOLDIER_GRENADE_KILL_WORMSIGN
// 1a TAUNTATK_SHOW_ITEM
// 1b TAUNTATK_MEDIC_RELEASE_DOVES
// 1c TAUNTATK_PYRO_ARMAGEDDON
// 1d TAUNTATK_PYRO_SCORCHSHOT
// 1e TAUNTATK_ALLCLASS_GUITAR_RIFF
// 1f TAUNTATK_MEDIC_HEROIC_TAUNT


// video with taunt attacks: hit detection type, damage type, distances, multi target, etc

// 1d TAUNTATK_PYRO_SCORCHSHOT
// ?

// 1c TAUNTATK_PYRO_ARMAGEDDON
// multi target
// sphere detection: 100 HU radius from your abs origin

// 04 TAUNTATK_HEAVY_HIGH_NOON
// single target
// trace from eye, extending 500 HU

// 16 TAUNTATK_ENGINEER_ARM_IMPALE
// 17 TAUNTATK_ENGINEER_ARM_KILL
// 18 TAUNTATK_ENGINEER_ARM_BLEND
// 0c TAUNTATK_SNIPER_ARROW_STAB_IMPALE
// 0d TAUNTATK_SNIPER_ARROW_STAB_KILL
// 10 TAUNTATK_MEDIC_UBERSLICE_IMPALE
// 11 TAUNTATK_MEDIC_UBERSLICE_KILL
// 0f TAUNTATK_DEMOMAN_BARBARIAN_SWING
// 15 TAUNTATK_ENGINEER_GUITAR_SMASH
// single target
// trace from eye, extending 128 HU

// 0e TAUNTATK_SOLDIER_GRENADE_KILL
// detection center is from the soldier's actual hand bone position
// is radius damage with splash falloff

// 06 TAUNTATK_SCOUT_GRAND_SLAM
// 01 TAUNTATK_PYRO_HADOUKEN
// 08 TAUNTATK_SPY_FENCING_SLASH_A
// 09 TAUNTATK_SPY_FENCING_SLASH_B
// 0a TAUNTATK_SPY_FENCING_STAB
// multi target
// box detection: 48x48x48, 64 HU from your WSC in ? direction


namespace Mod::Visualize::Taunt_Attacks
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
	
	
	void GiveItem(const char *item)
	{
		for (int i = 0; i < 32; ++i) {
			CBasePlayer *player = UTIL_PlayerByIndex(i);
			if (player == nullptr) continue;
			if (!player->IsBot()) continue;
			
			DevMsg("GiveItem #%d \"%s\"\n", i, item);
			reinterpret_cast<CTFBot *>(player)->AddItem(item);
		}
	}
	
	
	/* this is a horrible horrible mess */
	void DoBotCommand(const char *category, int taunt, int step)
	{
		static char saved_category[1024];
		static int saved_taunt = 0;
		
		if (step == 0) {
			DevMsg("DoBotCommand save\n");
			snprintf(saved_category, sizeof(saved_category), "%s", category);
			saved_taunt = taunt;
		}
		if (step == 1) {
			DevMsg("DoBotCommand restore\n");
			category = saved_category;
			taunt = saved_taunt;
		}
		
		DevMsg("DoBotCommand step %d cat \"%s\" taunt %d\n", step, category, taunt);
		
		if (step == 0) ServerCommand("bot_kick all\n");
		
		if (V_stricmp(category, "ray") == 0) {
			switch (taunt) {
			case 1:
				if (step == 0) AddPuppetBot("red", "heavy", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 2\n");
				break;
			case 2:
				if (step == 0) AddPuppetBot("red", "engineer", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 2\n");
				if (step == 1) GiveItem("The Gunslinger");
				break;
			case 3:
				if (step == 0) AddPuppetBot("red", "sniper", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 0\n");
				if (step == 1) GiveItem("The Huntsman");
				break;
			case 4:
				if (step == 0) AddPuppetBot("red", "medic", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 2\n");
				if (step == 1) GiveItem("The Ubersaw");
				break;
			case 5:
				if (step == 0) AddPuppetBot("red", "demoman", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 2\n");
				if (step == 1) GiveItem("The Eyelander");
				break;
			case 6:
				if (step == 0) AddPuppetBot("red", "engineer", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 0\n");
				if (step == 1) GiveItem("The Frontier Justice");
				break;
			}
		} else if (V_stricmp(category, "box") == 0) {
			switch (taunt) {
			case 1:
				if (step == 0) AddPuppetBot("red", "scout", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 2\n");
				if (step == 1) GiveItem("The Sandman");
				break;
			case 2:
				if (step == 0) AddPuppetBot("red", "pyro", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 1\n");
				break;
			case 3:
				if (step == 0) AddPuppetBot("red", "spy", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 2\n");
				break;
			}
		} else if (V_stricmp(category, "other") == 0) {
			switch (taunt) {
			case 1:
				if (step == 0) AddPuppetBot("red", "pyro", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 0\n");
				if (step == 1) GiveItem("The Rainblower");
				break;
			case 2:
				if (step == 0) AddPuppetBot("red", "soldier", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 2\n");
				if (step == 1) GiveItem("The Equalizer");
				break;
			case 3:
				if (step == 0) AddPuppetBot("red", "pyro", "Bot");
				if (step == 0) ServerCommand("bot_selectweaponslot 1\n");
				if (step == 1) GiveItem("The Scorch Shot");
				break;
			}
		}
		
	//	if (step == 1) {
	//		ServerCommand("bot_teleport Bot 0 300 -170 0 0 0\n");
	//	}
	}
	
	
	CON_COMMAND(sig_visualize_taunt_attacks_bot0, "")
	{
		if (args.ArgC() < 3) return;
		
		const char *category = args[1];
		int taunt = atoi(args[2]);
		
		DoBotCommand(category, taunt, 0);
	}
	
	CON_COMMAND(sig_visualize_taunt_attacks_bot1, "")
	{
		DoBotCommand("", 0, 1);
	}
	
	
	void DrawCone(const Vector& origin, const QAngle& angle, float dot, float depth)
	{
		Vector fwd;
		Vector rt;
		Vector up;
		AngleVectors(angle, &fwd, &rt, &up);
		
		for (int i = 1; i <= 20; ++i) {
			float dist = depth * (i / 20.0f);
			
			Vector center = origin + (fwd * dist);
			
			float radius = dist * tan(acos(dot));
			
			NDebugOverlay::Circle(center, angle, radius, 0xff, 0xff, 0xff, 0x00, false, 10.0f);
			
		//	NDebugOverlay::Box(center, Vector(-1, -1, -1), Vector(1, 1, 1), 0xff, 0xff, 0xff, 0xff, 10.0f);
		}
	}
	
	
	RefCount rc_CTFPlayer_DoTauntAttack;
	int attack = TAUNTATK_NONE;
	CTFPlayer *taunt_player = nullptr;
	DETOUR_DECL_MEMBER(void, CTFPlayer_DoTauntAttack)
	{
		/* correct for ServerLinux 20160511a */
		attack = *(int *)((uintptr_t)this + 0x1eec);
		
		taunt_player = reinterpret_cast<CTFPlayer *>(this);
		
		SCOPED_INCREMENT(rc_CTFPlayer_DoTauntAttack);
		DETOUR_MEMBER_CALL();
	}
	
	
	DETOUR_DECL_MEMBER(void, IEngineTrace_TraceRay, const Ray_t& ray, unsigned int fMask, ITraceFilter *pTraceFilter, trace_t *pTrace)
	{
		DETOUR_MEMBER_CALL(ray, fMask, pTraceFilter, pTrace);
		
		if (rc_CTFPlayer_DoTauntAttack > 0 &&
			((ray.m_Delta.Length() > 127.9f && ray.m_Delta.Length() < 128.1f) ||
			(ray.m_Delta.Length() > 499.9f && ray.m_Delta.Length() < 500.1f))) {
			Vector begin = ray.m_Start;
			Vector end = ray.m_Start + ray.m_Delta;
			
			float duration = 0.00f;
			uint8_t r = 0xff;
			uint8_t g = 0xff;
			uint8_t b = 0xff;
			
			switch (attack) {
			case TAUNTATK_HEAVY_HIGH_NOON:
				duration = 3.00f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "500 damage (DMG_BULLET)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
				
			case TAUNTATK_ENGINEER_ARM_IMPALE:
				duration = 0.05f;
				r = 0x00; g = 0xff; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "1 damage (DMG_BULLET)", duration, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityTextAtPosition(end, 2, "stun: 3.0 seconds", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			case TAUNTATK_ENGINEER_ARM_KILL:
				duration = 3.00f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "500 damage (DMG_BLAST)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			case TAUNTATK_ENGINEER_ARM_BLEND:
				duration = 0.05f;
				r = 0x00; g = 0x00; b = 0xff;
				NDebugOverlay::EntityTextAtPosition(end, 1, "1 damage (DMG_BULLET)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
				
			case TAUNTATK_SNIPER_ARROW_STAB_IMPALE:
				duration = 1.20f;
				r = 0x00; g = 0xff; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "no damage",         duration, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityTextAtPosition(end, 2, "stun: 3.0 seconds", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			case TAUNTATK_SNIPER_ARROW_STAB_KILL:
				duration = 3.00f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "500 damage (DMG_BULLET)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
				
			case TAUNTATK_MEDIC_UBERSLICE_IMPALE:
				duration = 0.60f;
				r = 0x00; g = 0xff; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "1 damage (DMG_BULLET)", duration, 0xff, 0xff, 0xff, 0xff);
				NDebugOverlay::EntityTextAtPosition(end, 2, "stun: 1.5 seconds", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			case TAUNTATK_MEDIC_UBERSLICE_KILL:
				duration = 3.00f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "500 damage (DMG_BULLET)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
				
			case TAUNTATK_DEMOMAN_BARBARIAN_SWING:
				duration = 3.00f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "500 damage (DMG_CLUB)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
				
			case TAUNTATK_ENGINEER_GUITAR_SMASH:
				duration = 3.00f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(end, 1, "500 damage (DMG_CLUB)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			}
			
			if (duration != 0.00f) {
				NDebugOverlay::Line(begin, end, r, g, b, true, duration);
			}
		}
	}
	
	DETOUR_DECL_MEMBER(void, ISpatialPartition_EnumerateElementsInBox, SpatialPartitionListMask_t listMask, const Vector& mins, const Vector& maxs, bool coarseTest, IPartitionEnumerator *pIterator)
	{
		if (rc_CTFPlayer_DoTauntAttack > 0) {
			Vector center = (mins + maxs) * 0.5f;
			
			float duration = 0.00f;
			uint8_t r = 0xff;
			uint8_t g = 0xff;
			uint8_t b = 0xff;
			uint8_t a = 0x80;
			
			switch (attack) {
			case TAUNTATK_SCOUT_GRAND_SLAM: {
				/* correct for ServerLinux 20160511a */
				float yaw = *(float *)((uintptr_t)taunt_player + 0x2044);
				
				DrawCone(taunt_player->WorldSpaceCenter(), QAngle(0.0f, yaw, 0.0f), 0.8f, 128.0f);
				/* (aim yaw).fwd dotprod wsc_to_wsc >= 0.8 */
				
				/* checks if dot product >= 0.8
				 * does a custom WSC-to-WSC line of sight check */
				duration = 3.0f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(center, 1, "500 damage (DMG_BULLET)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			}
			case TAUNTATK_PYRO_HADOUKEN:
				/* also checks FVisible */
				duration = 3.0f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(center, 1, "500 damage (DMG_IGNITE|DMG_BURN)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
				
			case TAUNTATK_SPY_FENCING_SLASH_A:
				/* also checks FVisible */
				duration = 0.40f;
				r = 0x00; g = 0xff; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(center, 1, "25 damage (DMG_SLASH)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			case TAUNTATK_SPY_FENCING_SLASH_B:
				/* also checks FVisible */
				duration = 1.50f;
				r = 0x00; g = 0x00; b = 0xff;
				NDebugOverlay::EntityTextAtPosition(center, 1, "25 damage (DMG_SLASH)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			case TAUNTATK_SPY_FENCING_STAB:
				/* also checks FVisible */
				duration = 3.00f;
				r = 0xff; g = 0x00; b = 0x00;
				NDebugOverlay::EntityTextAtPosition(center, 1, "500 damage (DMG_SLASH)", duration, 0xff, 0xff, 0xff, 0xff);
				break;
			}
			
			if (duration != 0.00f) {
				NDebugOverlay::Box(mins, vec3_origin, maxs - mins, r, g, b, a, duration);
			}
		}
		
		DETOUR_MEMBER_CALL(listMask, mins, maxs, coarseTest, pIterator);
	}
	
	DETOUR_DECL_MEMBER(void, CEntitySphereQuery_ctor, const Vector& center, float radius, int flagMask)
	{
		/* must have line of sight too */
		
		if (rc_CTFPlayer_DoTauntAttack > 0) {
			switch (attack) {
			case TAUNTATK_PYRO_ARMAGEDDON:
				NDebugOverlay::Sphere(center, vec3_angle, radius, 0xff, 0x00, 0x00, 0x00, false, 3.00f);
				break;
				
			case TAUNTATK_SOLDIER_GRENADE_KILL:
				NDebugOverlay::Sphere(center, vec3_angle, 1.0f,   0xff, 0xff, 0xff, 0xff, false, 3.00f);
				NDebugOverlay::Sphere(center, vec3_angle, radius, 0xff, 0x00, 0x00, 0x00, false, 3.00f);
				break;
			}
		}
		
		DETOUR_MEMBER_CALL(center, radius, flagMask);
	}
	
	
	void DrawScorchShotTrail()
	{
		static bool valid = false;
		static Vector first = vec3_origin;
		static Vector last  = vec3_origin;
		
		bool reset = true;
		for (int i = 0; i < IBaseProjectileAutoList::AutoList().Count(); ++i) {
			auto flare = rtti_cast<CTFProjectile_Flare *>(IBaseProjectileAutoList::AutoList()[i]);
			if (flare == nullptr) continue;
			
			/* correct for ServerLinux 20160511a */
			float dt_fired = gpGlobals->curtime - *(float *)((uintptr_t)flare + 0x4d4);
			bool lethal = (dt_fired < 0.050f);
			
			uint8_t r;
			uint8_t g;
			uint8_t b;
			
			if (lethal) {
				r = 0xff; g = 0x00; b = 0x00;
			} else {
				r = 0x00; g = 0xff; b = 0x00;
			}
			
			if (valid) {
				NDebugOverlay::Line(last, flare->WorldSpaceCenter(), r, g, b, true, 10.0f);
			} else {
				first = flare->WorldSpaceCenter();
			}
			
			float dist = (first - flare->WorldSpaceCenter()).Length();
			
			NDebugOverlay::EntityTextAtPosition(flare->WorldSpaceCenter(), 1, (lethal ? "LETHAL" : "NOPE"), 10.0f, 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityTextAtPosition(flare->WorldSpaceCenter(), 2, CFmtStrN<64>("%.0fms", 1000.0f * dt_fired), 10.0f, 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityTextAtPosition(flare->WorldSpaceCenter(), 3, CFmtStrN<64>("~%.0fHU", dist), 10.0f, 0xff, 0xff, 0xff, 0xff);
			
			last = flare->WorldSpaceCenter();
			
			reset = false;
		}
		
		if (reset) {
			valid = false;
		} else {
			valid = true;
		}
	}
	
	
	// TAUNTATK_PYRO_ARMAGEDDON:
	// CEntitySphereQuery ctor
	
	// TAUNTATK_SOLDIER_GRENADE_KILL:
	// use Visualize:Blast_Radius!
	
	// detour CBaseEntity::TakeDamage
	// highlight each affected player and show the amount of damage taken
	// (actually, find a better way to hook this so we actually get the adjusted/final damage amount)
	
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Visualize:Taunt_Attacks")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_DoTauntAttack, "CTFPlayer::DoTauntAttack");
			
			MOD_ADD_DETOUR_MEMBER(IEngineTrace_TraceRay,                    "IEngineTrace::TraceRay");
			MOD_ADD_DETOUR_MEMBER(ISpatialPartition_EnumerateElementsInBox, "ISpatialPartition::EnumerateElementsInBox");
			MOD_ADD_DETOUR_MEMBER(CEntitySphereQuery_ctor,                  "CEntitySphereQuery::CEntitySphereQuery [C1]");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			DrawScorchShotTrail();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_visualize_taunt_attacks", "0", FCVAR_NOTIFY,
		"Visualization: draw taunt kill detection",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
