#include "mod.h"
#include "stub/tfbot.h"
#include "stub/tfentities.h"
#include "stub/strings.h"
#include "stub/usermessages_sv.h"
#include "util/iterate.h"


namespace Mod::Debug::Robot_Pose_Parameters
{
	void PrintToChatAll(const char *str)
	{
		int msg_type = usermessages->LookupUserMessage("SayText2");
		if (msg_type == -1) return;
		
		CReliableBroadcastRecipientFilter filter;
		
		bf_write *msg = engine->UserMessageBegin(&filter, msg_type);
		if (msg == nullptr) return;
		
		msg->WriteByte(0x00);
		msg->WriteByte(0x00);
		msg->WriteString(str);
		
		engine->MessageEnd();
	}
	
	
	CON_COMMAND(sig_debug_robot_pose_parameters_recalc, "")
	{
		ForEachTFPlayer([](CTFPlayer *player){
			CMultiplayerAnimState *m_PlayerAnimState = player->m_PlayerAnimState;
			m_PlayerAnimState->OnNewModel();
		});
	}
	
	ConVar cvar_giant("sig_debug_robot_pose_parameters_giant", "0", FCVAR_NONE);
	CON_COMMAND(sig_debug_robot_pose_parameters_change, "")
	{
		CTFPlayer *host = ToTFPlayer(UTIL_PlayerByIndex(1));
		if (host == nullptr) return;
		
		auto l_switch = [=](int klass, bool giant){
			DevMsg("l_switch: klass = %d, giant = %d\n", klass, giant);
			
			const char *model_path = (giant ? g_szBotBossModels[klass] : g_szBotModels[klass]);
			
			bool sentry_buster = false;
			if (klass == 10) {
				sentry_buster = true;
				
				klass      = TF_CLASS_DEMOMAN;
				model_path = g_szBotBossSentryBusterModel;
			}
			
		//	host->HandleCommand_JoinTeam_NoMenus("blue");
			
			DevMsg("l_switch: HandleCommand_JoinClass(\"%s\")\n", g_aRawPlayerClassNames[klass]);
			host->HandleCommand_JoinClass(g_aRawPlayerClassNames[klass]);
			
			DevMsg("l_switch: SetMiniBoss(%d)\n", giant);
			DevMsg("l_switch: SetModelScale(%.2f)\n", (giant ? 1.75f : 1.00f));
			host->SetMiniBoss(giant);
			host->SetModelScale(giant ? 1.75f : 1.00f);
			
			DevMsg("l_switch: SetCustomModel(\"%s\")\n", model_path);
			DevMsg("l_switch: UpdateModel()\n");
			host->GetPlayerClass()->SetCustomModel(model_path);
			host->UpdateModel();
			
		//	DevMsg("l_switch: ForceRespawn()\n");
		//	host->ForceRespawn();
			
			if (sentry_buster) {
				PrintToChatAll(CFmtStr("Switched to: sentrybuster (\"%s\")\n", model_path));
			} else {
				PrintToChatAll(CFmtStr("Switched to: %s %s (\"%s\")\n", (giant ? "giant" : "normal"), g_aRawPlayerClassNames[klass], model_path));
			}
		};
		
		l_switch(atoi(args[1]), cvar_giant.GetBool());
	}
	
	CON_COMMAND(sig_debug_robot_pose_parameters_dump, "")
	{
		CTFPlayer *host = ToTFPlayer(UTIL_PlayerByIndex(1));
		if (host == nullptr) {
			Warning("host == nullptr\n");
			return;
		}
		
	//	host->SetModel(args[1]);
		
		CStudioHdr *pStudioHdr = host->GetModelPtr();
		if (pStudioHdr == nullptr) {
			Warning("pStudioHdr == nullptr\n");
			return;
		}
		
	//	MDLHandle_t handle = mdlcache->FindMDL(args[1]);
	//	if (handle == MDLHANDLE_INVALID) {
	//		
	//	}
	//	Msg("handle = %04x\n", handle);
	//	return;
		
		
		int nParams = pStudioHdr->GetNumPoseParameters();
		Msg("\n%-3s %-12s  %7s %7s %7s  %8s\n",
			"IDX", "NAME________", "__START", "____END", "___LOOP", "___FLAGS");
		for (int i = 0; i < nParams; ++i) {
			const auto& param = pStudioHdr->pPoseParameter(i);
			
			Msg("%3s %12s  %7s %7s %7s  %8s\n",
				CFmtStr("[%d]",   i)              .Get(),
				CFmtStr("\"%s\"", param.pszName()).Get(),
				CFmtStr("% 7.2f", param.start)    .Get(),
				CFmtStr("% 7.2f", param.end)      .Get(),
				CFmtStr("% 7.2f", param.loop)     .Get(),
				CFmtStr("%08x",   param.flags)    .Get());
		}
		Msg("\n");
	}
	
	
	// TODO: need a list of the various robot models' pose parameter orders
	// TODO: generate list of all interesting permutations
	
	// TODO: might need a debug overlay to show pose parameters live, if the built-in one doesn't do what we need
	
	
	// turn off cosmetics because they don't all look good...
	DETOUR_DECL_MEMBER(bool, CTFWearable_CanEquip, CBaseEntity *pEntity)
	{
		return false;
	}
	
	// don't drop ammo packs on death
	DETOUR_DECL_MEMBER(bool, CTFPlayer_ShouldDropAmmoPack)
	{
		return false;
	}
	
	
	void DrawHitboxes(CBaseAnimating *anim)
	{
		if (!anim->IsPlayer() && !anim->IsBaseObject()) return;
		if (anim->GetTeamNumber() == TEAM_SPECTATOR)    return;
		if (!anim->IsAlive())                           return;
		
		anim->DrawServerHitboxes(3600.0f, false);
	}
	
	void DrawPoseParameterOverlay()
	{
		CTFPlayer *host = ToTFPlayer(UTIL_PlayerByIndex(1));
		if (host == nullptr) {
			Warning("DrawPoseParameterOverlay: host == nullptr\n");
			return;
		}
		
		CStudioHdr *pStudioHdr = host->GetModelPtr();
		if (pStudioHdr == nullptr) {
			Warning("DrawPoseParameterOverlay: pStudioHdr == nullptr\n");
			return;
		}
		
		int line = 0;
		auto l_PrintText = [&](const char *str){
			NDebugOverlay::ScreenText(0.03f, 0.03f + (line * 0.03f), str, 0xff, 0xff, 0xff, 0xff, 3600.0f);
			++line;
		};
		
//		l_PrintText("ACTUAL:");
//		l_PrintText("");
		l_PrintText("IDX  NAME_________  __VALUE  __START  ____END  ___LOOP  ___FLAGS");
		int nParams = pStudioHdr->GetNumPoseParameters();
		for (int i = 0; i < nParams; ++i) {
			const auto& param = pStudioHdr->pPoseParameter(i);
			float val = host->GetPoseParameter(i);
			
			l_PrintText(CFmtStr("[%d]  %-13s  % 7.2f  % 7.2f  % 7.2f  % 7.2f  %08x\n", i, CFmtStr("\"%s\"", param.pszName()).Get(), val, param.start, param.end, param.loop, param.flags));
		}
		
		l_PrintText("");
		
//		l_PrintText("CACHED:");
//		l_PrintText("");
//		l_PrintText("IDX  NAME_________  __VALUE  __START  ____END  ___LOOP  ___FLAGS");
//		int nParams = pStudioHdr->GetNumPoseParameters();
//		for (int i = 0; i < nParams; ++i) {
//		//	const auto& param = pStudioHdr->pPoseParameter(i);
//		//	float val = host->GetPoseParameter(i);
//		//	
//		//	l_PrintText(CFmtStr("[%d]  %-13s  % 7.2f  % 7.2f  % 7.2f  % 7.2f  %08x\n", i, CFmtStr("\"%s\"", param.pszName()).Get(), val, param.start, param.end, param.loop, param.flags));
//		}
	}
	
	
	class CMod : public IMod, public IFrameUpdatePostEntityThinkListener
	{
	public:
		CMod() : IMod("Debug:Robot_Pose_Parameters")
		{
			MOD_ADD_DETOUR_MEMBER(CTFWearable_CanEquip,         "CTFWearable::CanEquip");
			MOD_ADD_DETOUR_MEMBER(CTFPlayer_ShouldDropAmmoPack, "CTFPlayer::ShouldDropAmmoPack");
		}
		
		virtual bool ShouldReceiveCallbacks() const override { return this->IsEnabled(); }
		
		virtual void FrameUpdatePostEntityThink() override
		{
			// get rid of ragdolls
			ForEachEntityByRTTI<CTFRagdoll>([](CTFRagdoll *ragdoll){
				ragdoll->Remove();
			});
			
			NDebugOverlay::Clear();
			
			ForEachEntityByRTTI<CBaseAnimating>([](CBaseAnimating *anim){
				DrawHitboxes(anim);
			});
			
			DrawPoseParameterOverlay();
		}
	};
	CMod s_Mod;
	
	
	ConVar cvar_enable("sig_debug_robot_pose_parameters", "0", FCVAR_NOTIFY,
		"Debug: demonstrate screwed-up robot player model pose parameters' effect on hitboxes",
		[](IConVar *pConVar, const char *pOldValue, float flOldValue){
			s_Mod.Toggle(static_cast<ConVar *>(pConVar)->GetBool());
		});
}
