#include "mod.h"
#include "stub/gamerules.h"
#include "stub/tfbot.h"
#include "stub/tfentities.h"
//#include "util/backtrace.h"


namespace Mod::AI::MvM_Defender_Bots
{
	std::vector<bool> stack_m_bPlayingMannVsMachine;
	void Quirk_MvM_Pre(bool red)
	{
		stack_m_bPlayingMannVsMachine.push_back(TFGameRules()->IsMannVsMachineMode());
		
		if (red) {
			TFGameRules()->Set_m_bPlayingMannVsMachine(false);
		} else {
			TFGameRules()->Set_m_bPlayingMannVsMachine(true);
		}
	}
	void Quirk_MvM_Post()
	{
		assert(!stack_m_bPlayingMannVsMachine.empty());
		
		TFGameRules()->Set_m_bPlayingMannVsMachine(stack_m_bPlayingMannVsMachine.back());
		stack_m_bPlayingMannVsMachine.pop_back();
	}
	
	
	DETOUR_DECL_MEMBER(CTFPlayer *, CTFBotMedicHeal_SelectPatient, CTFBot *actor, CTFPlayer *player)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(actor, player);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(Action<CTFBot> *, CTFBotEngineerBuild_InitialContainedAction, CTFBot *actor)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(actor);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_STATIC(bool, CTFBotGetAmmo_IsPossible, CTFBot *actor)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_STATIC_CALL(actor);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_STATIC(bool, CTFBotGetHealth_IsPossible, CTFBot *actor)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_STATIC_CALL(actor);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(const CKnownEntity *, CTFBotSniperLurk_SelectMoreDangerousThreat, const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2)
	{
		Quirk_MvM_Pre(nextbot->GetEntity()->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(nextbot, them, threat1, threat2);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotVision_Update)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		
		Quirk_MvM_Pre(vision->GetBot()->GetEntity()->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL();
		Quirk_MvM_Post();
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsIgnored, CBaseEntity *ent)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		
		Quirk_MvM_Pre(vision->GetBot()->GetEntity()->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(ent);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBotVision_IsVisibleEntityNoticed, CBaseEntity *ent)
	{
		auto vision = reinterpret_cast<IVision *>(this);
		
		Quirk_MvM_Pre(vision->GetBot()->GetEntity()->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(ent);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotLocomotion_Approach, const Vector& v1, float f1)
	{
		auto loco = reinterpret_cast<ILocomotion *>(this);
		
		Quirk_MvM_Pre(loco->GetBot()->GetEntity()->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL(v1, f1);
		Quirk_MvM_Post();
	}
	
	DETOUR_DECL_MEMBER(float, CTFBotBody_GetHeadAimTrackingInterval)
	{
		auto body = reinterpret_cast<IBody *>(this);
		
		Quirk_MvM_Pre(body->GetBot()->GetEntity()->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL();
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_Touch, CBaseEntity *ent)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		Quirk_MvM_Pre(bot->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL(ent);
		Quirk_MvM_Post();
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBot_ShouldFireCompressionBlast)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		Quirk_MvM_Pre(bot->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL();
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(bool, CTFBot_EquipLongRangeWeapon)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		Quirk_MvM_Pre(bot->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL();
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(float, CTFBot_GetDesiredAttackRange)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		Quirk_MvM_Pre(bot->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL();
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_AvoidPlayers, CUserCmd *usercmd)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		Quirk_MvM_Pre(bot->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL(usercmd);
		Quirk_MvM_Post();
	}
	
	DETOUR_DECL_MEMBER(void, CTFBot_Event_Killed, const CTakeDamageInfo& info)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		Quirk_MvM_Pre(bot->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL(info);
		Quirk_MvM_Post();
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMainAction_OnStuck, CTFBot *actor)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(actor);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotMedicHeal_Update, CTFBot *actor, float dt)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotTacticalMonitor_Update, CTFBot *actor, float dt)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(void, CTFBotMainAction_FireWeaponAtEnemy, CTFBot *actor)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		DETOUR_MEMBER_CALL(actor);
		Quirk_MvM_Post();
	}
	
	DETOUR_DECL_MEMBER(CCaptureFlag *, CTFBot_GetFlagToFetch)
	{
		auto bot = reinterpret_cast<CTFBot *>(this);
		
		Quirk_MvM_Pre(bot->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL();
		Quirk_MvM_Post();
		return result;
	}
	
	#warning need to separate quirk from enhancement without breaking enhancement
	DETOUR_DECL_MEMBER(const CKnownEntity *, CTFBotMainAction_SelectMoreDangerousThreatInternal, const INextBot *nextbot, const CBaseCombatCharacter *them, const CKnownEntity *threat1, const CKnownEntity *threat2)
	{
		/* CTFBotMainAction::SelectMoreDangerousThreat already does ToTFBot for us */
		if (nextbot == nullptr) return nullptr;
		auto actor = reinterpret_cast<CTFBot *>(nextbot->GetEntity());
		
		if (TFGameRules()->IsMannVsMachineMode() && actor->GetTeamNumber() == TF_TEAM_RED) {
			CTFPlayer *player1 = ToTFPlayer(threat1->GetEntity());
			CTFPlayer *player2 = ToTFPlayer(threat2->GetEntity());
			
			if (player1 != nullptr && player2 != nullptr &&
				player1->GetTeamNumber() == TF_TEAM_BLUE &&
				player2->GetTeamNumber() == TF_TEAM_BLUE) {
				
				bool carrier1 = player1->HasTheFlag();
				bool carrier2 = player2->HasTheFlag();
				
				if (carrier1 && !carrier2) return threat1;
				if (carrier2 && !carrier1) return threat2;
			}
		}
		
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(nextbot, them, threat1, threat2);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotSpyAttack_Update, CTFBot *actor, float dt)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		Quirk_MvM_Post();
		return result;
	}
	
	DETOUR_DECL_MEMBER(ActionResult<CTFBot>, CTFBotStickybombSentrygun_Update, CTFBot *actor, float dt)
	{
		Quirk_MvM_Pre(actor->GetTeamNumber() == TF_TEAM_RED);
		auto result = DETOUR_MEMBER_CALL(actor, dt);
		Quirk_MvM_Post();
		return result;
	}
	
	
	#define AddDetour_Member(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_MEMBER_CALLBACK(detour), GET_MEMBER_INNERPTR(detour)))
	#define AddDetour_Static(detour, addr) \
		mod->AddDetour(new CDetour(addr, GET_STATIC_CALLBACK(detour), GET_STATIC_INNERPTR(detour)))
	
	void AddQuirks_MannVsMachineMode(IMod *mod)
	{
		/* QUIRK: red bots use robot patient selection logic */
		AddDetour_Member(CTFBotMedicHeal_SelectPatient, "CTFBotMedicHeal::SelectPatient");
		
		/* QUIRK: initial engie action isn't EngineerBuildTeleportEntrance */
	//	AddDetour_Member(CTFBotEngineerBuild_InitialContainedAction, "CTFBotEngineerBuild::InitialContainedAction");
		
		/* QUIRK: bots won't try to pick up ammo */
		AddDetour_Static(CTFBotGetAmmo_IsPossible, "CTFBotGetAmmo::IsPossible");
		
		/* QUIRK: bots won't try to pick up health */
		AddDetour_Static(CTFBotGetHealth_IsPossible, "CTFBotGetHealth::IsPossible");
		
		/* QUIRK: red snipers will obey tf_mvm_bot_sniper_target_by_dps */
		AddDetour_Member(CTFBotSniperLurk_SelectMoreDangerousThreat, "CTFBotSniperLurk::SelectMoreDangerousThreat");
		
		/* QUIRK: bot vision update interval is 0.9-1.1 seconds */
		AddDetour_Member(CTFBotVision_Update, "CTFBotVision::Update");
		
		/* QUIRK: bots don't ignore sapped buildings */
		AddDetour_Member(CTFBotVision_IsIgnored, "CTFBotVision::IsIgnored");
		
		/* QUIRK: bot vision obeys MvM specific rules for spies */
		AddDetour_Member(CTFBotVision_IsVisibleEntityNoticed, "CTFBotVision::IsVisibleEntityNoticed");
		
		/* QUIRK: bot locomotion won't approach while in the air */
		AddDetour_Member(CTFBotLocomotion_Approach, "CTFBotLocomotion::Approach");
		
		/* QUIRK: bot vision has fixed tracking interval for spies */
		AddDetour_Member(CTFBotBody_GetHeadAimTrackingInterval, "CTFBotBody::GetHeadAimTrackingInterval");
		
		/* QUIRK: bots won't immediately realize spies they bump into */
		AddDetour_Member(CTFBot_Touch, "CTFBot::Touch");
		
		/* QUIRK: pyro bots won't use airblast much */
		AddDetour_Member(CTFBot_ShouldFireCompressionBlast, "CTFBot::ShouldFireCompressionBlast");
		
		/* QUIRK: bots won't switch to long range weapons */
		AddDetour_Member(CTFBot_EquipLongRangeWeapon, "CTFBot::EquipLongRangeWeapon");
		
		/* QUIRK: bots use MvM-specific rocket launcher attack range */
		AddDetour_Member(CTFBot_GetDesiredAttackRange, "CTFBot::GetDesiredAttackRange");
		
		/* QUIRK: bots use 150 HU pushaway range instead of 50 HU */
		AddDetour_Member(CTFBot_AvoidPlayers, "CTFBot::AvoidPlayers");
		
		/* QUIRK: bots do robot related stuff (spies left, engineers left) upon dying */
		AddDetour_Member(CTFBot_Event_Killed, "CTFBot::Event_Killed");
		
		/* QUIRK: bots can stuck-destroy buildings */
		AddDetour_Member(CTFBotMainAction_OnStuck, "CTFBotMainAction::OnStuck");
		
		/* QUIRK: medic bots do a bunch of squad related nonsense */
		/* QUIRK: medic bots use more simplistic uber threshold logic */
		/* QUIRK: medic bots change to FetchFlag behavior in some cases */
		AddDetour_Member(CTFBotMedicHeal_Update, "CTFBotMedicHeal::Update");
		
		/* QUIRK: bots won't ever try to retreat */
		/* QUIRK: bots won't ever switch to DestroyEnemySentry */
		AddDetour_Member(CTFBotTacticalMonitor_Update, "CTFBotTacticalMonitor::Update");
		#warning PROBLEM: CTFBotTacticalMonitor::Update detour will break some stuff!
		// bots won't use teleporters
		// bots won't monitor armed stickybombs
		// spies won't avoid bumping enemies
		// bots won't update delayed threat notices
		
		/* QUIRK: bots use MvM specific flag logic */
		AddDetour_Member(CTFBot_GetFlagToFetch, "CTFBot::GetFlagToFetch");
		
		/* QUIRK: bots won't abstain from shooting ubered targets */
		/* QUIRK: bots obey tf_bot_hitscan_range_limit */
		/* QUIRK: sniper bots wait 0.5 seconds instead of 0.1 seconds */
		AddDetour_Member(CTFBotMainAction_FireWeaponAtEnemy, "CTFBotMainAction::FireWeaponAtEnemy");
		
		/* QUIRK: bots ignore sentries */
		/* ENHANCEMENT: make defender bots prioritize bomb carriers */
		AddDetour_Member(CTFBotMainAction_SelectMoreDangerousThreatInternal, "CTFBotMainAction::SelectMoreDangerousThreatInternal");
		
		/* QUIRK: spy bots have fixed backstab dot product threshold */
		/* QUIRK: spy bots chuckle when going for backstabs */
		AddDetour_Member(CTFBotSpyAttack_Update, "CTFBotSpyAttack::Update");
		
		/* QUIRK: ammo requirement for stickytrapping sentries is 5 rather than 3 */
		AddDetour_Member(CTFBotStickybombSentrygun_Update, "CTFBotStickybombSentrygun::Update");
	}
}
