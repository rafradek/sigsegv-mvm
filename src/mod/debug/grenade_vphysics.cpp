#include "mod.h"
#include "stub/baseentity.h"
#include "stub/tfbot.h"
#include "stub/projectiles.h"


#define TRACE_ENABLE 1
#define TRACE_TERSE  0
#include "util/trace.h"


namespace Mod::Debug::Grenade_VPhysics
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
		
		constexpr int damage = -1000000;
		ServerCommand("bot_hurt -name %s -damage %d\n", name, damage);
	}
	
	void RefillBots()
	{
		ServerCommand("bot_refill\n");
	}
	
	
	CON_COMMAND(sig_debug_grenade_vphysics_bot_add, "")
	{
		ServerCommand("bot_kick all\n");
		
	//	AddPuppetBot("red", "demoman", "GrenadeLauncher");
	//	AddPuppetBot("red", "demoman", "LochNLoad");
	//	AddPuppetBot("red", "demoman", "IronBomber");
	//	AddPuppetBot("red", "demoman", "LooseCannon");
		
	//	AddPuppetBot("red", "demoman", "OldLochNLoad");
	//	AddPuppetBot("red", "demoman", "NewLochNLoad");
		
	//	AddPuppetBot("red", "sniper", "Normal");
	//	AddPuppetBot("red", "sniper", "Festive");
	//	AddPuppetBot("red", "sniper", "BreadMonster");
		
	//	AddPuppetBot("red", "scout", "Normal");
	//	AddPuppetBot("red", "scout", "BreadMonster");
		
	//	AddPuppetBot("red", "scout", "Sandman");
	//	AddPuppetBot("red", "scout", "WrapAssassin");
		
	//	AddPuppetBot("red", "demoman", "GL_Spin");
	//	AddPuppetBot("red", "demoman", "GL_NoSpin");
	//	AddPuppetBot("red", "demoman", "GL_NoSpin_Yaw45");
	//	AddPuppetBot("red", "demoman", "GL_NoSpin_Yaw90");
	//	AddPuppetBot("red", "demoman", "GL_NoSpin_Pitch45");
	//	AddPuppetBot("red", "demoman", "GL_NoSpin_Pitch90");
	//	AddPuppetBot("red", "demoman", "GL_NoSpin_Yaw45_Pitch45");
	//	AddPuppetBot("red", "demoman", "GL_NoSpin_Yaw180");
		
	//	AddPuppetBot("red", "medic", "Xbow");
	//	AddPuppetBot("red", "medic", "FestiveXbow");
		
		AddPuppetBot("red", "demoman", "GrenadeLauncher");
	}
	
	CON_COMMAND(sig_debug_grenade_vphysics_bot_move, "")
	{
	//	TeleportBot("LochNLoad",       Vector(1000.0f, -100.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("IronBomber",      Vector(1000.0f,    0.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GrenadeLauncher", Vector(1000.0f,  100.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("LooseCannon",     Vector(1000.0f,  200.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
		
	//	TeleportBot("OldLochNLoad", Vector(1000.0f,   0.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
	//	TeleportBot("NewLochNLoad", Vector(1000.0f, 100.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
		
	//	TeleportBot("Normal",       Vector(1000.0f,  -50.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
	//	TeleportBot("Festive",      Vector(1000.0f,   50.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
	//	TeleportBot("BreadMonster", Vector(1000.0f,  150.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
		
	//	TeleportBot("Normal",       Vector(1000.0f,  -50.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
	//	TeleportBot("BreadMonster", Vector(1000.0f,  150.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
		
	//	TeleportBot("Sandman",      Vector(1500.0f,   50.0f, -150.0f), QAngle(-45.0f, 180.0f, 0.0f));
	//	TeleportBot("WrapAssassin", Vector(1500.0f,  250.0f, -150.0f), QAngle(-45.0f, 180.0f, 0.0f));
		
	//	TeleportBot("GL_NoSpin",               Vector(1000.0f, -100.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GL_NoSpin_Yaw180",        Vector(1000.0f,  -50.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GL_NoSpin_Yaw45",         Vector(1000.0f,    0.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GL_NoSpin_Pitch45",       Vector(1000.0f,   50.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GL_NoSpin_Pitch90",       Vector(1000.0f,  100.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GL_NoSpin_Yaw90",         Vector(1000.0f,  150.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GL_Spin",                 Vector(1000.0f,  200.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
	//	TeleportBot("GL_NoSpin_Yaw45_Pitch45", Vector(1000.0f,  250.0f, -150.0f), QAngle(-6.0f, 180.0f, 0.0f));
		
	//	TeleportBot("Xbow",        Vector(1000.0f,  -50.0f, -150.0f), QAngle(-10.0f, 180.0f, 0.0f));
	//	TeleportBot("FestiveXbow", Vector(1000.0f,  150.0f, -150.0f), QAngle(-10.0f, 180.0f, 0.0f));
		
		TeleportBot("GrenadeLauncher", Vector(1000.0f, 0.0f, -150.0f), QAngle(-8.0f, 180.0f, 0.0f));
	}
	
	CON_COMMAND(sig_debug_grenade_vphysics_bot_item, "")
	{
		for (int i = 0; i < 32; ++i) {
			CBasePlayer *player = UTIL_PlayerByIndex(i);
			if (player == nullptr) continue;
			
			const char *item = nullptr;
			
			const char *name = player->GetPlayerName();
			
		//	if (strstr(name, "FestiveXbow") != nullptr) {
		//		item = "Festive Crusader's Crossbow";
		//	} else if (strstr(name, "Xbow") != nullptr) {
		//		item = "The Crusader's Crossbow";
		//	}
			
		//	if (strstr(name, "Sandman") != nullptr) {
		//		item = "The Sandman";
		//	} else if (strstr(name, "WrapAssassin") != nullptr) {
		//		item = "The Wrap Assassin";
		//	}
			
		//	if (strstr(name, "Normal") != nullptr) {
		//		item = "Mad Milk";
		//	} else if (strstr(name, "BreadMonster") != nullptr) {
		//		item = "Mutated Milk";
		//	}
			
		//	if (strstr(name, "Normal") != nullptr) {
		//		item = "Jarate";
		//	} else if (strstr(name, "Festive") != nullptr) {
		//		item = "Festive Jarate";
		//	} else if (strstr(name, "BreadMonster") != nullptr) {
		//		item = "The Self-Aware Beauty Mark";
		//	}
			
		//	if (strstr(name, "LochNLoad") != nullptr) {
		//		item = "The Loch-n-Load";
		//	} else if (strstr(name, "IronBomber") != nullptr) {
		//		item = "The Iron Bomber";
		//	} else if (strstr(name, "LooseCannon") != nullptr) {
		//		item = "The Loose Cannon";
		//	}
			
			if (item == nullptr) continue;
			
			DevMsg("Giving item \"%s\" to player \"%s\"\n", item, name);
			reinterpret_cast<CTFBot *>(player)->AddItem(item);
		}
	}
	
	
#if 0
	DETOUR_DECL_MEMBER(IPhysicsObject *, CBaseEntity_VPhysicsInitNormal, SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		TRACE("[this: #%d \"%s\"] [solidType: %d] [nSolidFlags: %08x] [pSolid: %08x]",
			ENTINDEX(ent), ent->GetClassname(), solidType, nSolidFlags, (uintptr_t)pSolid);
		auto result = DETOUR_MEMBER_CALL(solidType, nSolidFlags, createAsleep, pSolid);
		TRACE_EXIT("[result: %08x]", (uintptr_t)result);
		return result;
	}
	
	DETOUR_DECL_STATIC(IPhysicsObject *, PhysModelCreate, CBaseEntity *pEntity, int modelIndex, const Vector& origin, const QAngle& angles, solid_t *pSolid)
	{
		TRACE("[pEntity: #%d \"%s\"] [modelIndex: %d] [pSolid: %08x]",
			ENTINDEX(pEntity), pEntity->GetClassname(), modelIndex, (uintptr_t)pSolid);
		auto result = DETOUR_STATIC_CALL(pEntity, modelIndex, origin, angles, pSolid);
		TRACE_EXIT("[result: %08x]", (uintptr_t)result);
		return result;
	}
#endif
	
	
	DETOUR_DECL_STATIC(int, CAttributeManager_AttribHookValue_int, int value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
		if (strcmp(attr, "mult_clipsize") == 0) {
		//	DevMsg("Overriding \"mult_clipsize\" to 100\n");
			return value * 100;
		}
		if (strcmp(attr, "grenade_no_spin") == 0) {
			if (ent != nullptr) {
				CBasePlayer *owner = ToBasePlayer(ent->GetOwnerEntity());
				if (owner != nullptr) {
					if (strstr(owner->GetPlayerName(), "NoSpin") != nullptr) {
						return 1;
					}
				}
			}
		}
		if (strcmp(attr, "mult_maxammo_grenades1") == 0) {
		//	DevMsg("Overriding \"mult_maxammo_grenades1\" to 100\n");
			return value * 100;
		}
		
		return DETOUR_STATIC_CALL(value, attr, ent, vec, b1);
	}
	
	DETOUR_DECL_STATIC(float, CAttributeManager_AttribHookValue_float, float value, const char *attr, const CBaseEntity *ent, CUtlVector<CBaseEntity *> *vec, bool b1)
	{
	//	if (strcmp(attr, "mult_projectile_speed") == 0) {
	//	//	DevMsg("Overriding \"mult_projectile_speed\" to 1.0\n");
	//		return value * 1.0f;
	//	}
	//	if (strcmp(attr, "grenade_launcher_mortar_mode") == 0) {
	//	//	DevMsg("Overriding \"grenade_launcher_mortar_mode\" to 0.0\n");
	//		return value * 0.0f;
	//	}
		if (strcmp(attr, "effectbar_recharge_rate") == 0) {
		//	DevMsg("Overriding \"effectbar_recharge_rate\" to 0.0\n");
			return value * 0.0f;
		}
		
		return DETOUR_STATIC_CALL(value, attr, ent, vec, b1);
	}
	
	
	DETOUR_DECL_STATIC(CBaseEntity *, CBaseEntity_CreateNoSpawn, const char *szName, const Vector& vecOrigin, const QAngle& vecAngles, CBaseEntity *pOwner)
	{
		QAngle angles = vecAngles;
		const QAngle& ref_angles = angles;
		
		CBasePlayer *player = ToBasePlayer(pOwner);
		if (player != nullptr) {
			if (strstr(player->GetPlayerName(), "Yaw45") != nullptr) {
				angles.y += 45.0f;
				if (angles.y >= 360.0f) angles.y -= 360.0f;
			}
			if (strstr(player->GetPlayerName(), "Yaw90") != nullptr) {
				angles.y += 90.0f;
				if (angles.y >= 360.0f) angles.y -= 360.0f;
			}
			if (strstr(player->GetPlayerName(), "Yaw180") != nullptr) {
				angles.y += 180.0f;
				if (angles.y >= 360.0f) angles.y -= 360.0f;
			}
			
			if (strstr(player->GetPlayerName(), "Pitch45") != nullptr) {
				angles.x += 45.0f;
				if (angles.x >= 360.0f) angles.x -= 360.0f;
			}
			if (strstr(player->GetPlayerName(), "Pitch90") != nullptr) {
				angles.x += 90.0f;
				if (angles.x >= 360.0f) angles.x -= 360.0f;
			}
		}
		
		return DETOUR_STATIC_CALL(szName, vecOrigin, ref_angles, pOwner);
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Grenade_VPhysics")
		{
		//	for (auto pattern : {
		//		"CTFWeaponBaseGrenadeProj\\w+Spawn",
		//	}) {
		//		this->AddDetour(new CFuncTrace(Library::SERVER, pattern));
		//	}
		//	
		//	MOD_ADD_DETOUR_MEMBER(CBaseEntity_VPhysicsInitNormal, "CBaseEntity::VPhysicsInitNormal");
		//	MOD_ADD_DETOUR_STATIC(PhysModelCreate,                "PhysModelCreate");
			
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_int,   "CAttributeManager::AttribHookValue<int>");
			MOD_ADD_DETOUR_STATIC(CAttributeManager_AttribHookValue_float, "CAttributeManager::AttribHookValue<float>");
			
			MOD_ADD_DETOUR_STATIC(CBaseEntity_CreateNoSpawn, "CBaseEntity::CreateNoSpawn");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			++frame;
			
			if (frame % 50 == 0) {
				float duration = 50 * gpGlobals->interval_per_tick;
				
				for (int i = 0; i < 32; ++i) {
					CBasePlayer *player = UTIL_PlayerByIndex(i);
					if (player == nullptr) continue;
					if (!player->IsBot()) continue;
					
					NDebugOverlay::EntityText(i, 0, CFmtStrN<128>("%s", player->GetPlayerName()),
						duration, 0xff, 0xff, 0xff, 0xff);
				}
			}
			
			if (frame % 50 == 0) {
				RefillBots();
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_grenade_vphysics", "0", FCVAR_NOTIFY,
		"Debug: grenade vphysics",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
