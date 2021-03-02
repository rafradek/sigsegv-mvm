#include "mod.h"
#include "util/scope.h"
#include "stub/tfplayer.h"
#include "stub/usermessages_sv.h"
#include "stub/particles.h"
#include "util/iterate.h"

namespace Mod::Pop::Red_Robot_Eye_Glow
{

    void SetEyeParticle(CTFPlayer *player) {
         DevMsg("setting player particles\n");
        Vector vColor = !player->IsBot() || player->m_nBotSkill >= 2 ? Vector( 255, 180, 36 ) : Vector( 0, 240, 255 );
        

        //StopParticleEffects(player);

        //CRecipientFilter filter;
        //ForEachTFPlayer([&](CTFPlayer *playerin){
        //    if (playerin != player)
        //        filter.AddRecipient(playerin);
        //});

        CReliableBroadcastRecipientFilter filter;
        //filter.RemoveRecipient(player);
        te_tf_particle_effects_control_point_t cp = { PATTACH_ABSORIGIN, vColor };

        const char *eye1 = player->IsMiniBoss() ? "eye_boss_1" : "eye_1";
        DispatchParticleEffect("bot_eye_glow", PATTACH_POINT_FOLLOW, player, eye1, vColor, vColor, false, false, &cp, &filter);

        const char *eye2 = player->IsMiniBoss() ? "eye_boss_2" : "eye_2";
        DispatchParticleEffect("bot_eye_glow", PATTACH_POINT_FOLLOW, player, eye2, vColor, vColor, false, false, &cp, &filter);

    }

    DETOUR_DECL_MEMBER(void, CTFPlayer_ChangeTeam, int iTeamNum, bool b1, bool b2, bool b3)
	{
		auto player = reinterpret_cast<CTFPlayer *>(this);
		
		if (iTeamNum == TF_TEAM_RED) {
            const char *modelname = modelinfo->GetModelName(player->GetModel());
            DevMsg("player joined with model %s\n",modelname);
            if (modelname != nullptr && strncmp(modelname,"models/bots/", strlen("models/bots/")) == 0)
                SetEyeParticle(player);
        }
		
		DETOUR_MEMBER_CALL(CTFPlayer_ChangeTeam)(iTeamNum, b1, b2, b3);
	}

    DETOUR_DECL_MEMBER(void, CTFPlayerClassShared_SetCustomModel, const char *s1, bool b1)
	{
		auto shared = reinterpret_cast<CTFPlayerClassShared *>(this);
        auto player = shared->GetOuter();
        
        if (strncmp(shared->GetCustomModel(),"models/bots/", strlen("models/bots/")) == 0)
            StopParticleEffects(player);

        if (player != nullptr && player->GetTeamNumber() == TF_TEAM_RED && s1 != nullptr && strncmp(s1,"models/bots/", strlen("models/bots/")) == 0) {
            SetEyeParticle(player);
        }

		DETOUR_MEMBER_CALL(CTFPlayerClassShared_SetCustomModel)(s1, b1);
	}

    class CMod : public IMod
	{
	public:
		CMod() : IMod("MvM:Red_Robot_Eye_Glow")
		{
            MOD_ADD_DETOUR_MEMBER(CTFPlayer_ChangeTeam, "CBasePlayer::ChangeTeam [int, bool, bool, bool]");
            MOD_ADD_DETOUR_MEMBER(CTFPlayerClassShared_SetCustomModel, "CTFPlayerClassShared::SetCustomModel");
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_mvm_red_robot_eye_glow", "0", FCVAR_NOTIFY,
		"Mod: add eye glow to red robots",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}