#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/usermessages_sv.h"
#include "stub/particles.h"
#include "stub/extraentitydata.h"
#include "util/iterate.h"

namespace Mod::Pop::Red_Robot_Eye_Glow
{
    ConVar cvar_eye_particle("sig_mvm_eye_particle", "", FCVAR_NOTIFY,
		"Mod: add eye particle to models");

    ConVar cvar_human_eye_particle("sig_mvm_human_eye_particle", "0", FCVAR_NOTIFY,
		"Mod: add eye particle to humans");
    
    ConVar cvar_extended_difficulty_colors("sig_mvm_eye_extended_difficulty_colors", "0", FCVAR_NOTIFY,
		"Mod: special eye colors for normal and expert bots");

    void SetEyeColorForDiff(Vector &vec, int difficulty, bool isspec) {

        if (!isspec) {
            if (!cvar_extended_difficulty_colors.GetBool()) {
                difficulty >= 2 ? vec.Init( 255, 180, 36) : vec.Init( 0, 240, 255 );
            }
            switch (difficulty) {
                case 0: vec.Init( 0, 240, 255 ); break;
                case 1: vec.Init( 0, 120, 255 ); break;
                case 3: vec.Init( 255, 100, 36); break;
                case 2: vec.Init( 255, 180, 36); break;
                default: vec.Init( 0, 240, 255 );
            }
        }
        else {
            if (!cvar_extended_difficulty_colors.GetBool()) {
                difficulty >= 2 ? vec.Init( 255, 200, 100) : vec.Init( 120, 248, 255 );
            }
            switch (difficulty) {
                case 0: vec.Init( 120, 248, 255 ); break;
                case 1: vec.Init( 120, 180, 255 ); break;
                case 3: vec.Init( 255, 160, 120); break;
                case 2: vec.Init( 255, 205, 120); break;
                default: vec.Init( 120, 248, 255 );
            }
        }
    }
    THINK_FUNC_DECL(EyeParticle)
    {
        auto player = reinterpret_cast<CTFPlayer *>(this);
        Vector vColor;
        SetEyeColorForDiff(vColor, !player->IsBot() ? 2 : player->m_nBotSkill, player->GetTeamNumber() == TEAM_SPECTATOR);
        
        Vector vColorL = vColor / 255;
        
        StopParticleEffects(player);

        CRecipientFilter filter;
        filter.AddAllPlayers();
        filter.RemoveRecipient(player);
        te_tf_particle_effects_control_point_t cp = { PATTACH_ABSORIGIN, vColor };

        const char *particle = "bot_eye_glow";
        if (strlen(cvar_eye_particle.GetString()) != 0)
            particle = cvar_eye_particle.GetString();

        const char *eye1 = player->IsMiniBoss() ? "eye_boss_1" : "eye_1";
        if (cvar_human_eye_particle.GetBool() && player->LookupAttachment(eye1) == 0)
            eye1 = "eyeglow_L";
        
        DispatchParticleEffect(particle, PATTACH_POINT_FOLLOW, player, eye1, vec3_origin, false, vColorL, vColorL, true, false, &cp, &filter);

        const char *eye2 = player->IsMiniBoss() ? "eye_boss_2" : "eye_2";
        if (cvar_human_eye_particle.GetBool() && player->LookupAttachment(eye2) == 0)
            eye2 = "eyeglow_R";

        DispatchParticleEffect(particle, PATTACH_POINT_FOLLOW, player, eye2, vec3_origin, false, vColorL, vColorL, true, false, &cp, &filter);

        variant_t var;
        var.SetFloat(1.0f);
        player->SetCustomVariable("CustomEyeGlow", var);
    }

    void DeployModel(CTFPlayer *player, const char *model)
    {
        if (player->GetCustomVariableFloat<"CustomEyeGlow">()) {
            StopParticleEffects(player);
            variant_t var;
            var.SetFloat(0.0f);
            player->SetCustomVariable("CustomEyeGlow", var);
        }

        bool bot = player != nullptr && player->IsBot();
        if (player != nullptr && ((player->GetTeamNumber() != TF_TEAM_BLUE && player->IsAlive() && model != nullptr && strncmp(model,"models/bots/", strlen("models/bots/")) == 0) 
            || (bot && cvar_human_eye_particle.GetBool())
            || (bot && strlen(cvar_eye_particle.GetString()) > 0)
            || (bot && cvar_extended_difficulty_colors.GetBool()))) {

            THINK_FUNC_SET(player, EyeParticle, gpGlobals->curtime + 0.05f);
        }
    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_ChangeTeam, int iTeamNum, bool b1, bool b2, bool b3)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);

		DETOUR_MEMBER_CALL(CTFPlayer_ChangeTeam)(iTeamNum, b1, b2, b3);

		if (iTeamNum == TF_TEAM_RED || (iTeamNum == TEAM_SPECTATOR && player->IsAlive())) {
            const char *modelname = player->GetPlayerClass()->GetCustomModel();
            DeployModel(player, modelname);
            DevMsg("player joined with model %s\n",modelname);
        }
	}

    DETOUR_DECL_MEMBER(void, CTFPlayerClassShared_SetCustomModel, const char *s1, bool b1)
	{
		
        auto shared = reinterpret_cast<CTFPlayerClassShared *>(this);
        auto player = shared->GetOuter();
        DeployModel(player, s1);

		DETOUR_MEMBER_CALL(CTFPlayerClassShared_SetCustomModel)(s1, b1);
	}

    DETOUR_DECL_MEMBER(void, CTFBot_OnEventChangeAttributes, void *ecattr)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
        DeployModel(player, player->GetPlayerClass()->GetCustomModel());
		DETOUR_MEMBER_CALL(CTFBot_OnEventChangeAttributes)(ecattr);
    }

    DETOUR_DECL_MEMBER(void, CTFGameRules_PlayerKilled, CBasePlayer *pVictim, const CTakeDamageInfo& info)
	{
		DETOUR_MEMBER_CALL(CTFGameRules_PlayerKilled)(pVictim, info);

		if (pVictim->GetCustomVariableFloat<"CustomEyeGlow">()) {
            StopParticleEffects(pVictim);
            variant_t var;
            var.SetFloat(0.0f);
            pVictim->SetCustomVariable("CustomEyeGlow", var);
        }
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Red_Robot_Eye_Glow")
		{
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_ChangeTeam, "CBasePlayer::ChangeTeam [int, bool, bool, bool]");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerClassShared_SetCustomModel, "CTFPlayerClassShared::SetCustomModel");
            MOD_ADD_DETOUR_MEMBER(CTFBot_OnEventChangeAttributes, "CTFBot::OnEventChangeAttributes");
            MOD_ADD_DETOUR_MEMBER(CTFGameRules_PlayerKilled, "CTFGameRules::PlayerKilled");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_red_robot_eye_glow", "0", FCVAR_NOTIFY,
		"Mod: add eye glow to red robots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}