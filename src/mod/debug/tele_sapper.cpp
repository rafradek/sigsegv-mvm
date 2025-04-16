#include "mod.h"
#include "stub/objects.h"
#include "stub/tfbot_behavior.h"
#include "re/nextbot.h"


namespace Mod::Debug::Tele_Sapper
{
	bool  *CBaseObject_IsPlasmaDisabled     (CBaseObject *obj) { return reinterpret_cast<bool  *>((uintptr_t)obj + 0x0a1c); }
	float *CBaseObject_GetPlasmaDisabledTime(CBaseObject *obj) { return reinterpret_cast<float *>((uintptr_t)obj + 0x0a18); }
	bool  *CBaseObject_m_bHasSapper         (CBaseObject *obj) { return reinterpret_cast<bool  *>((uintptr_t)obj + 0x0a30); }
	
	
	class CTFBotHelperSpy : public Action<CTFBot> /*IHotplugAction*/
	{
	public:
		virtual const char *GetName() const override { return "HelperSpy"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			actor->PushRequiredWeapon(actor->Weapon_OwnsThisID(TF_WEAPON_BUILDER));
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			if (this->m_bSapping) {
				actor->PressFireButton();
			} else {
				actor->ReleaseFireButton();
			}
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual void OnEnd(CTFBot *actor, Action<CTFBot> *action) override
		{
			actor->PopRequiredWeapon();
		}
		
		
		virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd)
		{
			if (V_stricmp(cmd, "spy_on") == 0) {
				this->m_bSapping = true;
			}
			if (V_stricmp(cmd, "spy_off") == 0) {
				this->m_bSapping = false;
			}
			if (V_stricmp(cmd, "spy_toggle") == 0) {
				this->m_bSapping = !this->m_bSapping;
			}
			
			if (V_strnicmp(cmd, "spy_lookat:", strlen("spy_lookat:")) == 0) {
				int entindex = atoi(cmd + strlen("spy_lookat:"));
				CBaseEntity *ent = UTIL_EntityByIndex(entindex);
				
				if (ent != nullptr) {
					actor->GetBodyInterface()->AimHeadTowards(ent->WorldSpaceCenter(), IBody::LookAtPriorityType::OVERRIDE_ALL, 3600.0f, nullptr, CFmtStrN<64>("Looking at entity #%d", entindex));
				}
			}
			
			return EventDesiredResult<CTFBot>::Continue();
		}
		
	private:
		bool m_bSapping = false;
	};
	
	
	class CTFBotHelperSoldier : public Action<CTFBot> /*IHotplugAction*/
	{
	public:
		virtual const char *GetName() const override { return "HelperSoldier"; }
		
		virtual ActionResult<CTFBot> OnStart(CTFBot *actor, Action<CTFBot> *action) override
		{
			actor->AddItem("The Cow Mangler 5000");
			
			return ActionResult<CTFBot>::Continue();
		}
		
		virtual ActionResult<CTFBot> Update(CTFBot *actor, float dt) override
		{
			if (this->m_bCharging) {
				actor->PressAltFireButton();
			} else {
				actor->ReleaseAltFireButton();
			}
			
			return ActionResult<CTFBot>::Continue();
		}
		
		
		virtual EventDesiredResult<CTFBot> OnCommandString(CTFBot *actor, const char *cmd)
		{
			if (V_stricmp(cmd, "soldier_on") == 0) {
				this->m_bCharging = true;
			}
			if (V_stricmp(cmd, "soldier_off") == 0) {
				this->m_bCharging = false;
			}
			if (V_stricmp(cmd, "soldier_toggle") == 0) {
				this->m_bCharging = !this->m_bCharging;
			}
			
			if (V_strnicmp(cmd, "soldier_lookat:", strlen("soldier_lookat:")) == 0) {
				int entindex = atoi(cmd + strlen("soldier_lookat:"));
				CBaseEntity *ent = UTIL_EntityByIndex(entindex);
				
				if (ent != nullptr) {
					actor->GetBodyInterface()->AimHeadTowards(ent->WorldSpaceCenter(), IBody::LookAtPriorityType::OVERRIDE_ALL, 3600.0f, nullptr, CFmtStrN<64>("Looking at entity #%d", entindex));
				}
			}
			
			return EventDesiredResult<CTFBot>::Continue();
		}
		
	private:
		bool m_bCharging = false;
	};
	
	
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotMainAction_InitialContainedAction, CTFBot *actor)
	{
		return nullptr;
	}
	
	DETOUR_DECL_MEMBER(EventDesiredResult<CTFBot>, Action_CTFBot_OnCommandString, CTFBot *actor, const char *cmd)
	{
		if (V_stricmp(cmd, "helper_spy") == 0 && actor->IsPlayerClass(TF_CLASS_SPY)) {
			auto action = reinterpret_cast<Action<CTFBot> *>(this);
			if (rtti_cast<CTFBotMainAction *>(action) != nullptr) {
				return EventDesiredResult<CTFBot>::ChangeTo(new CTFBotHelperSpy(), "Enabling helper spy mode");
			}
		}
		
		if (V_stricmp(cmd, "helper_soldier") == 0 && actor->IsPlayerClass(TF_CLASS_SOLDIER)) {
			auto action = reinterpret_cast<Action<CTFBot> *>(this);
			if (rtti_cast<CTFBotMainAction *>(action) != nullptr) {
				return EventDesiredResult<CTFBot>::ChangeTo(new CTFBotHelperSoldier(), "Enabling helper soldier mode");
			}
		}
		
		return DETOUR_MEMBER_CALL(actor, cmd);
	}
	
	
	// TODO: may also need to mess with GetSapper?
	ConVar cvar_fix("sig_debug_tele_sapper_fix", "0", FCVAR_NOTIFY,
		"Debug: make CBaseObject::HasSapper() tell the truth");
	
	/* ugh, seems there are inlined cases of this that we can't fix... */
	DETOUR_DECL_MEMBER(bool, CBaseObject_HasSapper)
	{
		if (cvar_fix.GetBool()) {
			return *CBaseObject_m_bHasSapper(reinterpret_cast<CBaseObject *>(this));
		}
		
		return DETOUR_MEMBER_CALL();
	}
	
	
	ConVar cvar_plasma_duration("sig_debug_tele_sapper_plasma_duration", "-1", FCVAR_NOTIFY,
		"Debug: override SetPlasmaDisabled duration (-1 won't override)");
	
	DETOUR_DECL_MEMBER(void, CBaseObject_SetPlasmaDisabled, float duration)
	{
		auto obj = reinterpret_cast<CBaseObject *>(this);
		
		if (cvar_plasma_duration.GetFloat() >= 0.0f) {
			duration = cvar_plasma_duration.GetFloat();
		}
		
		DevMsg("CBaseObject::SetPlasmaDisabled(#%d, %.1f)\n", ENTINDEX(obj), duration);
		
		DETOUR_MEMBER_CALL(duration);
	}
	
	
	class CMod : public IMod, public IFrameUpdatePreEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Tele_Sapper")
		{
			MOD_ADD_DETOUR_MEMBER(CTFBotMainAction_InitialContainedAction, "CTFBotMainAction::InitialContainedAction");
			MOD_ADD_DETOUR_MEMBER(Action_CTFBot_OnCommandString,           "Action<CTFBot>::OnCommandString");
			
			MOD_ADD_DETOUR_MEMBER(CBaseObject_HasSapper, "CBaseObject::HasSapper");
			
			MOD_ADD_DETOUR_MEMBER(CBaseObject_SetPlasmaDisabled, "CBaseObject::SetPlasmaDisabled");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePreEntityThink() override
		{
			static long frame = 0;
			if (++frame % 2 == 0) return;
			
			float duration = 2 * gpGlobals->interval_per_tick;
			
			for (int i = gpGlobals->maxClients + 1; i < 2048; ++i) {
				CBaseEntity *ent = UTIL_EntityByIndex(i);
				if (ent == nullptr) continue;
				
				Vector where = ent->WorldSpaceCenter() + Vector(0.0f, 0.0f, 40.0f);
				
				if (ent->ClassMatches("obj_attachment_sapper")) {
				//	NDebugOverlay::EntityBounds(ent, 0xff, 0x00, 0x00, 0x20, duration);
				//	NDebugOverlay::EntityText(i, -1, CFmtStrN<64>("SAPPER #%d", i), duration);
				} else if (ent->ClassMatches("obj_teleporter")) {
					NDebugOverlay::EntityTextAtPosition(where, 0, CFmtStrN<64>("TELE #%d", i), duration);
					
					auto obj = static_cast<CBaseObject *>(ent);
					
					bool is_plasma_disabled = *CBaseObject_IsPlasmaDisabled(obj);
					float plasma_disabled_time = *CBaseObject_GetPlasmaDisabledTime(obj);
					
					if (is_plasma_disabled) {
						float time_left = plasma_disabled_time - gpGlobals->curtime;
						
						NDebugOverlay::EntityTextAtPosition(where, 1, CFmtStrN<64>("PLASMA DISABLED: %.1f", time_left), duration);
					} else {
						NDebugOverlay::EntityTextAtPosition(where, 1, "PLASMA DISABLED: NOPE", duration);
					}
					
					NDebugOverlay::EntityTextAtPosition(where, 2, CFmtStrN<64>("obj->HasSapper() = %s", (obj->HasSapper() ? "TRUE" : "FALSE")), duration);
				}
			}
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_tele_sapper", "0", FCVAR_NOTIFY,
		"Debug: investigate why spamming sappers and wrenching can result in a sapper on one tele but not the other",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_debug_tele_sapper_spawn_bots, "")
	{
		
	}
	
	CON_COMMAND(sig_debug_tele_sapper_plasma_disable, "")
	{
		if (args.ArgC() < 3) return;
		
		int entindex   = atoi(args[1]);
		float duration = atof(args[2]);
		
		auto obj = rtti_cast<CBaseObject *>(UTIL_EntityByIndex(entindex));
		if (obj == nullptr) return;
		
		obj->SetPlasmaDisabled(duration);
	}
}
