#include "mod.h"
#include "stub/baseplayer.h"
#include "util/iterate.h"


namespace Mod::Debug::Damage_Overlay
{
	ConVar cvar_clear("sig_debug_damage_overlay_clear", "1", FCVAR_NOTIFY,
		"Debug: clear overlays each time");
	ConVar cvar_duration("sig_debug_damage_overlay_duration", "3600.0", FCVAR_NOTIFY,
		"Debug: overlay duration");
	
	
	int   pre_hp;
	float pre_dmg;
	
	int   post_hp;
	float post_dmg;
	
	
	DETOUR_DECL_MEMBER(int, CBaseCombatCharacter_OnTakeDamage, const CTakeDamageInfo& info)
	{
		pre_dmg = info.GetDamage();
		
		return DETOUR_MEMBER_CALL(CBaseCombatCharacter_OnTakeDamage)(info);
	}
	
	DETOUR_DECL_MEMBER(int, CBaseEntity_TakeDamage, const CTakeDamageInfo& inputInfo)
	{
		auto ent = reinterpret_cast<CBaseEntity *>(this);
		
		pre_hp   = ent->GetHealth();
		post_dmg = 0.0f;
		
		auto result = DETOUR_MEMBER_CALL(CBaseEntity_TakeDamage)(inputInfo);
		
		post_hp = ent->GetHealth();
		
		Vector where = ent->GetAbsOrigin();
		where.z += (ent->CollisionProp()->OBBMaxs().z + 3.0f);
		
		if (ent->IsCombatCharacter()) {
			if (cvar_clear.GetBool()) {
				NDebugOverlay::Clear();
			}
			
			NDebugOverlay::EntityTextAtPosition(where, -6, CFmtStr("HP before: %3d (%3d%%)",  pre_hp, (int)(100.0f *  pre_hp / ent->GetMaxHealth())),
				cvar_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityTextAtPosition(where, -5, CFmtStr("HP after:  %3d (%3d%%)", post_hp, (int)(100.0f * post_hp / ent->GetMaxHealth())),
				cvar_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			
			NDebugOverlay::EntityTextAtPosition(where, -3, CFmtStr("\"Raw\"  damage: %.1f", pre_dmg), cvar_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff);
			NDebugOverlay::EntityTextAtPosition(where, -2, CFmtStr("Actual damage: %.1f",  post_dmg), cvar_duration.GetFloat(), 0xff, 0xff, 0xff, 0xff);
		}
		
		return result;
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener, public IGameEventListener2
	{
	public:
		CMod() : IMod("Debug:Damage_Overlay")
		{
			MOD_ADD_DETOUR_MEMBER(CBaseCombatCharacter_OnTakeDamage, "CBaseCombatCharacter::OnTakeDamage");
			MOD_ADD_DETOUR_MEMBER(CBaseEntity_TakeDamage,            "CBaseEntity::TakeDamage");
		}
		
		virtual void OnEnable() override
		{
			gameeventmanager->AddListener(this, "player_hurt", true);
		}
		virtual void OnDisable() override
		{
			gameeventmanager->RemoveListener(this);
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 == 0) return;
			
		//	ForEachBot([](CBasePlayer *bot){
		//		NDebugOverlay::
		//	});
		}
		
		virtual void FireGameEvent(IGameEvent *event) override
		{
			post_dmg = (float)event->GetInt("damageamount");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_damage_overlay", "0", FCVAR_NOTIFY,
		"Debug: draw some overlays to show damage information",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
