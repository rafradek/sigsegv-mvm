#include "mod.h"
#include "util/scope.h"
#include "link/link.h"


namespace Mod::Debug::Override_Step_Sound
{
	DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetOverrideStepSound, const char *pszBaseStepSoundName)
	{
		DevMsg("CTFPlayer::OverrideStepSound(\"%s\")\n", pszBaseStepSoundName);
		return pszBaseStepSoundName;
	}
	
	DETOUR_DECL_MEMBER(const char *, CTFPlayer_GetSceneSoundToken)
	{
		DevMsg("CTFPlayer::GetSceneSoundToken\n");
		return "";
	}
	
	
	DETOUR_DECL_STATIC(void, SV_ReplicateConVarChange, const ConVar *var, const char *newValue)
	{
		
		
		
	}
	
	
	RefCount rc_PlayStepSound;
	DETOUR_DECL_MEMBER(void, CBasePlayer_PlayStepSound, Vector& vecOrigin, surfacedata_t *psurface, float fvol, bool force)
	{
		SCOPED_INCREMENT(rc_PlayStepSound);
		DETOUR_MEMBER_CALL(vecOrigin, psurface, fvol, force);
	}
	
	DETOUR_DECL_MEMBER(void, CRecipientFilter_RemoveRecipientsByPVS, const Vector& origin)
	{
		if (rc_PlayStepSound > 0) {
			return;
		}
		
		DETOUR_MEMBER_CALL(origin);
	}
	
	
	// alternative approach:
	// legitimately change sv_footsteps to 0 and replicate it to the clients
	// and then on the server side, lie to the code anytime it checks the convar value
	
	// places where it's accessed by the server:
	// - CBasePlayer::UpdateStepSound
	// - CBasePlayer::PlayStepSound
	
	
	StaticFuncThunk<void, const ConVar *, const char *> SV_ReplicateConVarChange("SV_ReplicateConVarChange");
	void SpoofFootstepConVar(bool value)
	{
		static ConVar *sv_footsteps = icvar->FindVar("sv_footsteps");
		if (sv_footsteps == nullptr) {
			Warning("Can't find ConVar sv_footsteps!\n");
			return;
		}
		
		SV_ReplicateConVarChange(sv_footsteps, (value ? "1" : "0"));
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Override_Step_Sound")
		{
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetOverrideStepSound, "CTFPlayer::GetOverrideStepSound");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_GetSceneSoundToken,   "CTFPlayer::GetSceneSoundToken");
			
			MOD_ADD_DETOUR_MEMBER(CBasePlayer_PlayStepSound,              "CBasePlayer::PlayStepSound");
			MOD_ADD_DETOUR_MEMBER(CRecipientFilter_RemoveRecipientsByPVS, "CRecipientFilter::RemoveRecipientsByPVS");
		}
		
		virtual bool OnLoad() override
		{
			SpoofFootstepConVar(0);
			return true;
		}
		virtual void OnUnload() override
		{
			SpoofFootstepConVar(1);
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			SpoofFootstepConVar(0);
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_override_step_sound", "0", FCVAR_NOTIFY,
		"Debug: investigate ways of overriding bot step sounds",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
	
	
	CON_COMMAND(sig_debug_override_step_sound_spoof, "Spoof clients into thinking sv_footsteps is 0 when it's still 1 on the server")
	{
		ConVar *sv_footsteps = icvar->FindVar("sv_footsteps");
		if (sv_footsteps == nullptr) {
			Warning("Spoofing failed: could not find ConVar sv_footsteps.\n");
			return;
		}
		
		constexpr ptrdiff_t OFF_m_pParent = 0x1c;
		constexpr ptrdiff_t OFF_m_nFlags  = 0x14;
		
		auto parent   = *reinterpret_cast<ConVar **>((uintptr_t)sv_footsteps + OFF_m_pParent);
		auto m_nFlags =  reinterpret_cast<int *>    ((uintptr_t)parent       + OFF_m_nFlags);
		
		ConMsg("  Flags: "); ConVar_PrintFlags(sv_footsteps);
		
		DevMsg("> Setting sv_footsteps to 0 for clients to see\n");
		sv_footsteps->SetValue(0);
		
		DevMsg("> Unsetting FCVAR_REPLICATED\n");
		*m_nFlags &= ~FCVAR_REPLICATED;
		
		ConMsg("  Flags: "); ConVar_PrintFlags(sv_footsteps);
		
		DevMsg("> Setting sv_footsteps back to 1 for server to see\n");
		sv_footsteps->SetValue(1);
		
		DevMsg("> Setting FCVAR_REPLICATED\n");
		*m_nFlags |= ~FCVAR_REPLICATED;
		
		ConMsg("  Flags: "); ConVar_PrintFlags(sv_footsteps);
		
		
		// TODO: this won't affect new clients that join after this is executed
		// TODO: would want a "cleanup" type function that would put FCVAR_REPLICATED back and reset the value to 1
	}
	
}

// step sounds:
// done on the client ultimately
// here are our options:
// - make the client think TFGameRules() is nullptr
// - make the client think TFGameRules()->IsMannVsMachineMode() is false
// - make the client think the bot is not on TF_TEAM_BLUE
// - make the client think the bot is a miniboss
// - make the client think the bot is in TF_COND_DISGUISED

// spoofing miniboss to the client will have these effects:
// - health overlay will show up
// - health gains >= 100 will show particle "healthgained_*_giant" instead of "healthgained_*_large"
// - C_TFKnife::GetMeleeDamage will come up with the wrong value
// - robot eye particles will attempt to attach to "eye_boss_*" instead of "eye_*"
//   (probably shouldn't matter since it's not using the robot model anyway)
// - C_TFRagdoll will change some aspect of its behavior in OnDataChanged
// - C_TFWeaponBase::ShootSound will probably use the giant weapon sounds from the "visuals_mvm_boss" block
//   (but are these even done from the client side?)
// - ImpactCallback will play sound "MVM_Giant.BulletImpact" instead of "MVM_Robot.BulletImpact"
// - C_TFPlayer::TeamFortress_CalculateMaxSpeed will come up with the wrong value


#if 0
// RE: 20151007a (shared server/client code)
const char *CTFPlayer::GetOverrideStepSound(const char *pszBaseStepSoundName)
{
	struct StepSoundOverride_t
	{
		int num;
		const char *str_default;
		const char *str_override;
	};
	
	static const StepSoundOverride_t s_ReplacementSounds[] = {
		// Used by cosmetic: Ball-Kicking Boots
		{ 1, "Default.StepLeft",   "cleats_conc.StepLeft"  },
		{ 1, "Default.StepRight",  "cleats_conc.StepRight" },
		{ 1, "Dirt.StepLeft",      "cleats_dirt.StepLeft"  },
		{ 1, "Dirt.StepRight",     "cleats_dirt.StepRight" },
		{ 1, "Concrete.StepLeft",  "cleats_conc.StepLeft"  },
		{ 1, "Concrete.StepRight", "cleats_conc.StepRight" },
		
		// Used by cosmetic: The Sprinting Cephalopod
		{ 9, "Default.StepLeft",   "Octopus.StepCommon" },
		{ 9, "Default.StepRight",  "Octopus.StepCommon" },
		{ 9, "Dirt.StepLeft",      "Octopus.StepCommon" },
		{ 9, "Dirt.StepRight",     "Octopus.StepCommon" },
		{ 9, "Concrete.StepLeft",  "Octopus.StepCommon" },
		{ 9, "Concrete.StepRight", "Octopus.StepCommon" },
		
		// Used by MvM giant robot templates
		{ 2, "", "MVM.GiantHeavyStep"   },
		{ 3, "", "MVM.GiantSoldierStep" },
		{ 4, "", "MVM.GiantDemomanStep" },
		{ 5, "", "MVM.GiantScoutStep"   },
		{ 6, "", "MVM.GiantPyroStep"    },
		{ 7, "", "MVM.SentryBusterStep" },
		
		// Unused
		{ 8, "", "Chest.Step" },
	};
	
	if (TFGameRules() != nullptr && TFGameRules()->IsMannVsMachineMode() &&
		this->GetTeamNumber() == TF_TEAM_BLUE && !this->IsMiniBoss() &&
		!this->m_Shared.InCond(TF_COND_DISGUISED)) {
		return "MvM.BotStep";
	}
	
	int attr = CAttributeManager::AttribHookValue<int>(0, "override_footstep_sound_set", this);
	if (attr != 0) {
		for (int i = 0; i < 19; ++i) {
			if (attr == s_ReplacementSounds[i].num) {
				if (s_ReplacementSounds[i].str_default[0] == '\0' || V_stricmp(pszBaseStepSoundName, s_ReplacementSounds[i].str_default) == 0) {
					return s_ReplacementSounds[i].str_override;
				}
			}
		}
	}
	
	return pszBaseStepSoundName;
}
#endif
